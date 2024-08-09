#include <lua.h>
#include <lauxlib.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <stdio.h>

static int list(lua_State *L) {
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    unsigned int i;

    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        return 1;
    }

    cProcesses = cbNeeded / sizeof(DWORD);
    lua_newtable(L);
    for (i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            lua_pushinteger(L, aProcesses[i]);
            lua_rawseti(L, -2, i);
        }
    }

    return 1;
};

static int get_path(lua_State *L) {
    DWORD pid = luaL_checkinteger(L,1);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (hProcess == NULL) {
        lua_pushstring(L,"none");
        return 1;
    }

    TCHAR pPath[MAX_PATH] = TEXT("none");
    HMODULE hMod;
    DWORD cbNeeded;

    if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
        if (GetModuleFileNameEx(hProcess, hMod, pPath, sizeof(pPath) / sizeof(TCHAR))) {
            lua_pushstring(L,pPath);
        } else {
            lua_pushstring(L,"none");
        }
    }

    return 1;
};

static int get_name(lua_State *L) {
    DWORD pid = luaL_checkinteger(L,1);
    TCHAR procName[MAX_PATH] = TEXT("unknown");
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess != NULL) {
        HMODULE hMod;
        DWORD cbNeededMod;

        if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeededMod)) {
            GetModuleBaseName(hProcess, hMod, procName, sizeof(procName) / sizeof(TCHAR));
        }
    }
    CloseHandle(hProcess);
    lua_pushstring(L,procName);
    return 1;
};

static int get_memory_usage(lua_State *L) {
    DWORD pid = luaL_checkinteger(L,1);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

    if (hProcess == NULL) {
        lua_pushinteger(L,0);
        return 1;
    }

    PROCESS_MEMORY_COUNTERS pmc;

    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
        lua_pushinteger(L,pmc.WorkingSetSize);
    } else {
        lua_pushinteger(L,0);
    }

    return 1;
};

static int kill(lua_State *L) {
    DWORD pid = luaL_checkinteger(L,1);
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL) {
        lua_pushboolean(L,0);
    } else {
        if (!TerminateProcess(hProcess,0)) {
            lua_pushboolean(L,0);
        } else {
            lua_pushboolean(L,1);
        }
    }
    CloseHandle(hProcess);
    return 1;
};

static int freeze(lua_State *L) {
    DWORD pid = luaL_checkinteger(L,1);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        lua_pushboolean(L,0);
        return 1;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread != NULL) {
                    SuspendThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    } else {
        lua_pushboolean(L,0);
        return 1;
    }
    lua_pushboolean(L,1);
    CloseHandle(hSnapshot);
    return 1;
};

static int unfreeze(lua_State *L) {
    DWORD pid = luaL_checkinteger(L,1);
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        lua_pushboolean(L,0);
        return 1;
    }

    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hSnapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
                if (hThread != NULL) {
                    ResumeThread(hThread);
                    CloseHandle(hThread);
                }
            }
        } while (Thread32Next(hSnapshot, &te));
    } else {
        lua_pushboolean(L,0);
        return 1;
    }
    lua_pushboolean(L,1);
    CloseHandle(hSnapshot);
    return 1;
};

static const struct luaL_Reg z_process[] = {
    {"list",list},
    {"get_name",get_name},
    {"kill",kill},
    {"get_path",get_path},
    {"get_memory_usage",get_memory_usage},
    {"freeze",freeze},
    {"unfreeze",unfreeze},
    {NULL, NULL}
};

__declspec(dllexport) int luaopen_z_process(lua_State *L) {
    luaL_newlib(L, z_process);
    return 1;
}
