#include <lua.h>
#include <lauxlib.h>
#include <windows.h>

static int clickMouse(lua_State *L) {
    const char *button = lua_tostring(L, lua_upvalueindex(1));
    int x, y;
    if (lua_gettop(L) >= 2) {
        x = luaL_checkinteger(L, 1);
        y = luaL_checkinteger(L, 2);
    } else {
        POINT cursorPos;
        GetCursorPos(&cursorPos);
        x = cursorPos.x;
        y = cursorPos.y;
    }

    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dx = x * (65535 / GetSystemMetrics(SM_CXSCREEN));
    input.mi.dy = y * (65535 / GetSystemMetrics(SM_CYSCREEN));

    if (strcmp(button, "l") == 0) {
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;
    } else if (strcmp(button, "r") == 0) {
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_RIGHTDOWN | MOUSEEVENTF_RIGHTUP;
    } else if (strcmp(button, "m") == 0) {
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_MIDDLEDOWN | MOUSEEVENTF_MIDDLEUP;
    }
    SendInput(1, &input, sizeof(INPUT));

    return 0;
}

static const struct luaL_Reg z_mouse[] = {
    {NULL, NULL}
};

__declspec(dllexport) int luaopen_z_mouse(lua_State *L) {
    luaL_newlib(L, z_mouse);

    lua_pushstring(L, "l");
    lua_pushcclosure(L, clickMouse, 1);
    lua_setfield(L, -2, "left");

    lua_pushstring(L, "r");
    lua_pushcclosure(L, clickMouse, 1);
    lua_setfield(L, -2, "right");

    lua_pushstring(L, "m");
    lua_pushcclosure(L, clickMouse, 1);
    lua_setfield(L, -2, "middle");

    return 1;
}
