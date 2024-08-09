#include <lua.h>
#include <lauxlib.h>
#include <Windows.h>
#include <wingdi.h>
#include <stdio.h>
#include <stdbool.h>
#include <process.h>
#include "lodepng.h"
#include <windows.h>
#include <windows.h>
#include <gdiplus.h>

#pragma comment(lib, "Gdiplus.lib")

static int getResolution(lua_State *L) {
    HMONITOR hMonitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEX monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFOEX);

    if (!GetMonitorInfo(hMonitor, (LPMONITORINFO)&monitorInfo)) {
        return luaL_error(L, "Error - GetMonitorInfo");
    }

    lua_newtable(L);
    lua_pushinteger(L, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top);
    lua_setfield(L, -2, "height");

    return 1;
}

int getScreenshot(lua_State* L) {
    HDC hdc = GetDC(NULL);
    HDC m_hdc = CreateCompatibleDC(hdc);

    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    BITMAPINFO bi;
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = width;
    bi.bmiHeader.biHeight = -height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* ptr = NULL;
    HBITMAP m_hBmp = CreateDIBSection(m_hdc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
    HGDIOBJ obj = SelectObject(m_hdc, m_hBmp);
    BitBlt(m_hdc, 0, 0, width, height, hdc, 0, 0, SRCCOPY);

    unsigned char* pixels = (unsigned char*)ptr;
    for (int i = 0; i < width * height; ++i) {
        unsigned char temp = pixels[i * 4];
        pixels[i * 4] = pixels[i * 4 + 2];
        pixels[i * 4 + 2] = temp;
    }

    unsigned char* pngBuffer;
    size_t pngSize;
    unsigned error = lodepng_encode32(&pngBuffer, &pngSize, pixels, width, height);

    ReleaseDC(NULL, hdc);
    DeleteDC(m_hdc);
    DeleteObject(m_hBmp);
    DeleteObject(obj);

    if (!error) {
        lua_pushlstring(L, (const char*)pngBuffer, pngSize);
        free(pngBuffer);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

static const struct luaL_Reg z_screen[] = {
    {"getResolution", getResolution},
    {"getScreenshot", getScreenshot},
    {NULL, NULL}
};

__declspec(dllexport) int luaopen_z_screen(lua_State *L) {
    luaL_newlib(L, z_screen);
    return 1;
}
