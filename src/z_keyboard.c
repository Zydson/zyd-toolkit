#include <windows.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>

static int send_keys(lua_State *L) {
    const char *keys = luaL_optstring(L, 1, "");
    if (*keys == '\0') {
        return 0;
    }
    INPUT input[2] = {0};

    for (const char *ch = keys; *ch; ch++) {
        input[0].type = INPUT_KEYBOARD;
        input[0].ki.wVk = VkKeyScanA(*ch);
        input[0].ki.dwFlags = 0;

        input[1].type = INPUT_KEYBOARD;
        input[1].ki.wVk = VkKeyScanA(*ch);
        input[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, input, sizeof(INPUT));
    }
    return 1;
}

static const struct luaL_Reg z_keyboard[] = {
    {"send_keys",send_keys},
    {NULL, NULL}
};

__declspec(dllexport) int luaopen_z_keyboard(lua_State *L) {
    luaL_newlib(L, z_keyboard);
    return 1;
}