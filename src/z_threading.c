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
    int arg_count;
} ThreadData;

DWORD WINAPI thread_function(LPVOID arg) {
    ThreadData *data = (ThreadData *)arg;

    lua_rawgeti(data->L, LUA_REGISTRYINDEX, data->function_ref); // Push the function on the stack

    // Push the arguments onto the stack
    for (int i = 0; i < data->arg_count; i++) {
        lua_pushvalue(data->L, -(data->arg_count + 1)); // Push each argument onto the stack
    }

    if (lua_pcall(data->L, data->arg_count, 0, 0) != LUA_OK) {
        const char *error_msg = lua_tostring(data->L, -1);
        fprintf(stderr, "Error in Lua function: %s\n", error_msg);
        lua_pop(data->L, 1); // Pop the error message from the stack
    }

    luaL_unref(data->L, LUA_REGISTRYINDEX, data->function_ref);
    free(data);

    return 0;
}

static int l_thread(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    int arg_count = lua_gettop(L) - 1; // Number of arguments passed after the function

    ThreadData *data = malloc(sizeof(ThreadData));
    if (!data) {
        return luaL_error(L, "Memory allocation failed");
    }

    data->L = L;
    data->arg_count = arg_count;
    lua_pushvalue(L, 1);
    data->function_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    // Save arguments on the stack
    for (int i = 1; i <= arg_count; i++) {
        lua_pushvalue(L, i + 1);
    }

    HANDLE thread = CreateThread(NULL, 0, thread_function, data, 0, NULL);
    if (thread == NULL) {
        luaL_unref(L, LUA_REGISTRYINDEX, data->function_ref);
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
