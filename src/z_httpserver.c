#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

typedef struct {
    char *path;
    int function_ref;
} Route;

typedef struct {
    Route *routes;
    int route_count;
    SOCKET server_socket;
    lua_State *L;
    CRITICAL_SECTION cs;
} Server;

static Server *server = NULL;

static void log_error(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
}

static char *table_to_json(lua_State *L, int index) {
    if (!lua_istable(L, index)) {
        return NULL;
    }

    lua_pushvalue(L, index);

    lua_getglobal(L, "json");
    lua_getfield(L, -1, "encode");

    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return NULL;
    }

    lua_pushvalue(L, -3);
    lua_call(L, 1, 1);

    const char *json = lua_tostring(L, -1);
    lua_pop(L, 3);
    return strdup(json);
}

static void handle_client(SOCKET client_socket) {
    char buffer[1024];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        log_error("Failed to receive data from client");
        closesocket(client_socket);
        return;
    }

    buffer[bytes_received] = '\0';

    char *request_line = strtok(buffer, "\r\n");
    if (!request_line) {
        log_error("Failed to parse request line");
        closesocket(client_socket);
        return;
    }

    char method[16], path[256];
    sscanf(request_line, "%s %s", method, path);

    EnterCriticalSection(&server->cs);

    lua_pushlightuserdata(server->L, (void*)(uintptr_t)client_socket);
    lua_newtable(server->L);

    lua_pushstring(server->L, path);
    lua_setfield(server->L, -2, "url");

    lua_pushstring(server->L, method);
    lua_setfield(server->L, -2, "method");

    lua_newtable(server->L);
    char *header;
    while ((header = strtok(NULL, "\r\n")) != NULL) {
        if (strlen(header) > 0) {
            char key[256], value[256];
            sscanf(header, "%[^:]: %[^\r\n]", key, value);
            lua_pushstring(server->L, value);
            lua_setfield(server->L, -2, key);
        }
    }
    lua_setfield(server->L, -2, "headers");

    int response_code = 200;
    const char *response_body = NULL;
    int isJson = 0;
    int isHtml = 0;

    for (int i = 0; i < server->route_count; i++) {
        if (strcmp(path, server->routes[i].path) == 0) {
            lua_rawgeti(server->L, LUA_REGISTRYINDEX, server->routes[i].function_ref);
            lua_pushvalue(server->L, -2);
            lua_call(server->L, 1, 2);

            if (lua_isnumber(server->L, -2)) {
                response_code = (int)lua_tointeger(server->L, -2);
                lua_pop(server->L, 1);
                response_body = (char*)luaL_optstring(server->L, 0, "Error");
            } else if (lua_istable(server->L, -2)) {
                isJson = 1;
                char *json = table_to_json(server->L, -2);
                response_body = json;
            } else {
                response_body = lua_tostring(server->L, -2);
                if (strncmp(response_body, "<!DOCTYPE html>", 15) == 0 || strncmp(response_body, "<html>", 6) == 0) {
                    isHtml = 1;
                }
            }
            break;
        }
    }

    char http_response[1024];
    if (response_body) {
        const char *content_type = isJson ? "application/json" : (isHtml ? "text/html" : "text/plain");
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n%s",
            response_code,
            response_code == 200 ? "OK" : "Not Found",
            content_type,
            strlen(response_body),
            response_body);
    } else {
        const char *error_msg = "Internal Server Error";
        snprintf(http_response, sizeof(http_response),
            "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: %zu\r\n\r\n%s",
            strlen(error_msg), error_msg);
    }

    send(client_socket, http_response, strlen(http_response), 0);
    closesocket(client_socket);

    lua_settop(server->L, 0);
    if (isJson && response_body) {
        free((void*)response_body);
    }

    LeaveCriticalSection(&server->cs);
}



static int l_httpserver_start(lua_State *L) {
    int port = luaL_checkinteger(L, 1);
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        luaL_error(L, "Failed to initialize Winsock");
    }

    server->server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_socket == INVALID_SOCKET) {
        WSACleanup();
        luaL_error(L, "Failed to create socket");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server->server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(server->server_socket);
        WSACleanup();
        luaL_error(L, "Failed to bind socket");
    }

    if (listen(server->server_socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(server->server_socket);
        WSACleanup();
        luaL_error(L, "Failed to listen on socket");
    }

    printf("Server is listening on port %d...\n", port);

    while (1) {
        SOCKET client_socket = accept(server->server_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            log_error("Failed to accept client connection.");
            continue;
        }

        _beginthread((void(*)(void*))handle_client, 0, (void*)client_socket);
    }

    closesocket(server->server_socket);
    WSACleanup();
    return 0;
}

static int l_httpserver_route(lua_State *L) {
    luaL_checktype(L, 2, LUA_TFUNCTION);
    const char *path = luaL_checkstring(L, 1);

    EnterCriticalSection(&server->cs);

    int function_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    server->routes = realloc(server->routes, sizeof(Route) * (server->route_count + 1));
    if (!server->routes) {
        LeaveCriticalSection(&server->cs);
        luaL_error(L, "mem");
    }

    server->routes[server->route_count].path = strdup(path);
    server->routes[server->route_count].function_ref = function_ref;
    server->route_count++;

    LeaveCriticalSection(&server->cs);

    lua_pushboolean(L, 1);
    return 1;
}

static char *load_file(const char *file_path, size_t *file_size) {
    FILE *file = fopen(file_path, "rb");
    if (!file) {
        fprintf(stderr, "can't open file: %s\n", file_path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    *file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc(*file_size + 1);
    if (!buffer) {
        fprintf(stderr, "mem\n");
        fclose(file);
        return NULL;
    }

    fread(buffer, 1, *file_size, file);
    buffer[*file_size] = '\0';
    fclose(file);
    return buffer;
}

static int l_render(lua_State *L) {
    const char *file_path = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    size_t file_size;
    char *file_content = load_file(file_path, &file_size);
    if (!file_content) {
        return luaL_error(L, "can't load file");
    }

    size_t output_capacity = file_size * 2;
    char *output = malloc(output_capacity);
    if (!output) {
        free(file_content);
        return luaL_error(L, "mem");
    }
    strcpy(output, file_content);
    free(file_content);

    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
        const char *key = lua_tostring(L, -2);
        const char *value = lua_tostring(L, -1);

        size_t key_len = strlen(key);
        size_t value_len = strlen(value);

        char placeholder_space[256];
        char placeholder_no_space[256];

        snprintf(placeholder_space, sizeof(placeholder_space), "{{ %s }}", key);
        snprintf(placeholder_no_space, sizeof(placeholder_no_space), "{{%s}}", key);

        char *pos;
        while ((pos = strstr(output, placeholder_space)) != NULL || (pos = strstr(output, placeholder_no_space)) != NULL) {
            size_t placeholder_len = (pos == strstr(output, placeholder_space)) ? strlen(placeholder_space) : strlen(placeholder_no_space);
            size_t output_len = strlen(output);
            size_t new_output_size = output_len - placeholder_len + value_len;

            if (new_output_size > output_capacity) {
                output_capacity = new_output_size * 2;
                output = realloc(output, output_capacity);
                if (!output) {
                    return luaL_error(L, "mem");
                }
            }

            memmove(pos + value_len, pos + placeholder_len, output_len - (pos - output) - placeholder_len + 1);
            memcpy(pos, value, value_len);
        }

        lua_pop(L, 1);
    }

    lua_pushstring(L, output);
    free(output);
    return 1;
}


__declspec(dllexport) int luaopen_z_httpserver(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_httpserver_route);
    lua_setfield(L, -2, "route");

    lua_pushcfunction(L, l_httpserver_start);
    lua_setfield(L, -2, "start");

    lua_pushcfunction(L, l_render);
    lua_setglobal(L, "Render");

    server = malloc(sizeof(Server));
    server->routes = NULL;
    server->route_count = 0;
    server->L = L;
    InitializeCriticalSection(&server->cs);

    return 1;
}
