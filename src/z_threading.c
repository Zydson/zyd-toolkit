#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <windows.h>

typedef struct {
    lua_State *L;
    int function_ref;
    int *arg_refs;
    int num_args;
} ThreadData;

DWORD WINAPI thread_function(LPVOID arg) {
    ThreadData *data = (ThreadData *)arg;
    lua_State *L = lua_newthread(data->L);

    lua_rawgeti(L, LUA_REGISTRYINDEX, data->function_ref);

    for (int i = 0; i < data->num_args; i++) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, data->arg_refs[i]);
    }

    if (lua_pcall(L, data->num_args, 0, 0) != LUA_OK) {
        const char *error_msg = lua_tostring(L, -1);
        fprintf(stderr, "Error in Lua function: %s\n", error_msg);
        lua_pop(L, 1);
    }

    luaL_unref(L, LUA_REGISTRYINDEX, data->function_ref);
    for (int i = 0; i < data->num_args; i++) {
        luaL_unref(L, LUA_REGISTRYINDEX, data->arg_refs[i]);
    }
    free(data->arg_refs);
    free(data);

    return 0;
}

static int l_thread(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int num_args = lua_gettop(L) - 1;

    ThreadData *data = malloc(sizeof(ThreadData));
    if (!data) {
        return luaL_error(L, "Memory allocation failed");
    }

    data->L = L;
    data->num_args = num_args;
    data->arg_refs = malloc(num_args * sizeof(int));

    lua_pushvalue(L, 1);
    data->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    for (int i = 0; i < num_args; i++) {
        lua_pushvalue(L, i + 2);
        data->arg_refs[i] = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    HANDLE thread = CreateThread(NULL, 0, thread_function, data, 0, NULL);
    if (thread == NULL) {
        luaL_unref(L, LUA_REGISTRYINDEX, data->function_ref);
        for (int i = 0; i < num_args; i++) {
            luaL_unref(L, LUA_REGISTRYINDEX, data->arg_refs[i]);
        }
        free(data->arg_refs);
        free(data);
        return luaL_error(L, "Failed to create thread");
    }

    CloseHandle(thread);

    return 0;
}

__declspec(dllexport) int luaopen_z_threading(lua_State *L) {
    lua_newtable(L);

    lua_pushcfunction(L, l_thread);
    lua_setfield(L, -2, "Thread");

    return 1;
}
