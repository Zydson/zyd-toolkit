#include <lua.h>
#include <lauxlib.h>
#include <sqlite3.h>

static int l_sqlite_open(lua_State* L) {
    const char* filename = luaL_checkstring(L, 1);
    sqlite3* db;
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        lua_pushnil(L);
        lua_pushstring(L, sqlite3_errmsg(db));
        return 2;
    }

    sqlite3** ud = (sqlite3**)lua_newuserdata(L, sizeof(sqlite3*));
    *ud = db;
    luaL_getmetatable(L, "SQLite");
    lua_setmetatable(L, -2);
    return 1;
}

static int l_sqlite_exec(lua_State* L) {
    sqlite3* db = *(sqlite3**)luaL_checkudata(L, 1, "SQLite");
    const char* sql = luaL_checkstring(L, 2);

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        lua_pushnil(L);
        lua_pushstring(L, sqlite3_errmsg(db));
        return 2;
    }

    lua_newtable(L);
    int index = 1;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        lua_newtable(L);
        for (int col = 0; col < sqlite3_column_count(stmt); col++) {
            const char* name = sqlite3_column_name(stmt, col);
            const char* value = (const char*)sqlite3_column_text(stmt, col);
            lua_pushstring(L, name);
            lua_pushstring(L, value ? value : "NULL");
            lua_settable(L, -3);
        }
        lua_seti(L, -2, index++);
    }

    sqlite3_finalize(stmt);
    return 1;
}

static int l_sqlite_close(lua_State* L) {
    sqlite3* db = *(sqlite3**)luaL_checkudata(L, 1, "SQLite");
    sqlite3_close(db);
    return 0;
}

static const struct luaL_Reg sqlite_methods[] = {
    {"open", l_sqlite_open},
    {"exec", l_sqlite_exec},
    {"close", l_sqlite_close},
    {NULL, NULL}
};

int luaopen_z_sqlite(lua_State* L) {
    luaL_newmetatable(L, "SQLite");
    luaL_setfuncs(L, sqlite_methods, 0);

    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2);
    lua_settable(L, -3);

    lua_pushcfunction(L, l_sqlite_open);
    lua_setglobal(L, "sqlite");
    return 1;
}
