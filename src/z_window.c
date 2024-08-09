#include <lua.h>
#include <lauxlib.h>
#include <windows.h>

static int alert(lua_State *L) {
    const char *text = luaL_optstring(L,1,"Unknown");
    const char *title = luaL_optstring(L,2,"");
    MessageBox(NULL,text,title, MB_OK | MB_ICONINFORMATION);
    return 1;
};

static int confirm(lua_State *L) {
    const char *text = luaL_optstring(L,1,"Unknown");
    const char *title = luaL_optstring(L,2,"");
    int a = MessageBox(NULL,text,title, MB_YESNO | MB_ICONQUESTION);
    if (a == IDYES) {
        lua_pushboolean(L, 1);
    } else if (a == IDNO) {
        lua_pushboolean(L, 0);
    }
    return 1;
};

static const struct luaL_Reg z_window[] = {
    {"alert",alert},
    {"confirm",confirm},
    {NULL, NULL}
};

__declspec(dllexport) int luaopen_z_window(lua_State *L) {
    luaL_newlib(L, z_window);
    return 1;
}
