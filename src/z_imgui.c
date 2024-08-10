#include <d3d10.h>
#include <d3dcompiler.h>
#include <windows.h>
#include "imgui.h"
#include "imgui_impl_dx10.h"
#include "imgui_impl_win32.h"
#include "lua.hpp"

ID3D10Device* g_pd3dDevice = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D10RenderTargetView* g_mainRenderTargetView = NULL;
HWND g_hWnd = NULL;
const char* g_windowTitle = "Default";
int g_windowWidth = 250;
int g_windowHeight = 250;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void CreateHiddenWindow() {
    WNDCLASS wc = { sizeof(WNDCLASS) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "z";
    RegisterClass(&wc);
    g_hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW, wc.lpszClassName, g_windowTitle, WS_POPUP | WS_VISIBLE, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), NULL, NULL, wc.hInstance, NULL);
    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 255, LWA_COLORKEY);
    ShowWindow(g_hWnd, SW_SHOW);
}

void CreateDeviceD3D() {
    CreateHiddenWindow();
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = GetSystemMetrics(SM_CXSCREEN);
    sd.BufferDesc.Height = GetSystemMetrics(SM_CYSCREEN);
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    HRESULT hr = D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice);
    if (FAILED(hr)) { MessageBox(NULL, "swap chain", "Error", MB_OK); return; }
    ID3D10Texture2D* pBackBuffer;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
    if (FAILED(hr)) { MessageBox(NULL, "back buffer", "Error", MB_OK); return; }
    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    if (FAILED(hr)) { MessageBox(NULL, "rtv", "Error", MB_OK); return; }
    pBackBuffer->Release();
}

void CleanupDeviceD3D() {
    if (g_pd3dDevice) g_pd3dDevice->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_mainRenderTargetView) g_mainRenderTargetView->Release();
    if (g_hWnd) DestroyWindow(g_hWnd);
    g_hWnd = NULL;
}

void Render() {
    if (g_pd3dDevice == NULL) return;
    g_pd3dDevice->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    static float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_pd3dDevice->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
    ImGui::Render();
    ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
}

static int InitializeLua(lua_State* L) {
    const char* title = luaL_optstring(L, 1, "Default");
    g_windowTitle = title;
    g_windowWidth = luaL_optinteger(L, 2, 250);
    g_windowHeight = luaL_optinteger(L, 3, 250);
    CreateDeviceD3D();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)GetSystemMetrics(SM_CXSCREEN), (float)GetSystemMetrics(SM_CYSCREEN));
    io.IniFilename = nullptr;
    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX10_Init(g_pd3dDevice);

    luaL_Reg imgui_funcs[] = {
        {"Text", [](lua_State* L) -> int {
            const char* text = luaL_checkstring(L, 1);
            ImGui::Text("%s", text);
            return 0;
        }},
        {"Button", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            if (ImGui::Button(label)) {
                lua_pushboolean(L, true);
                return 1;
            }
            lua_pushboolean(L, false);
            return 1;
        }},
        {"Combo", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            luaL_checktype(L, 2, LUA_TTABLE);

            int current_item = luaL_checkinteger(L, 3);
            int count = luaL_len(L, 2);
            const char* items[100];

            for (int i = 1; i <= count; ++i) {
                lua_pushinteger(L, i);
                lua_gettable(L, 2);
                items[i - 1] = lua_tostring(L, -1);
                lua_pop(L, 1);
            }

            ImGui::Combo(label, &current_item, items, count);
            lua_pushinteger(L, current_item);
            return 1;
        }},
        {"SliderInt", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            int value = luaL_checkinteger(L, 2);
            int min = luaL_checkinteger(L, 3);
            int max = luaL_checkinteger(L, 4);
            if (ImGui::SliderInt(label, &value, min, max)) {
                lua_pushinteger(L, value);
                return 1;
            }
            lua_pushinteger(L, value);
            return 1;
        }},
        {"SliderFloat", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            float value = luaL_checknumber(L, 2);
            float min = luaL_checknumber(L, 3);
            float max = luaL_checknumber(L, 4);
            if (ImGui::SliderFloat(label, &value, min, max)) {
                lua_pushnumber(L, value);
                return 1;
            }
            lua_pushnumber(L, value);
            return 1;
        }},
        {"InputText", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            char buffer[256];
            strcpy(buffer, luaL_checkstring(L, 2));
            if (ImGui::InputText(label, buffer, sizeof(buffer))) {
                lua_pushstring(L, buffer);
                return 1;
            }
            lua_pushstring(L, buffer);
            return 1;
        }},
        {"Checkbox", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            bool checked = lua_toboolean(L, 2);
            if (ImGui::Checkbox(label, &checked)) {
                lua_pushboolean(L, checked);
                return 1;
            }
            lua_pushboolean(L, checked);
            return 1;
        }},
        {"ColorButton", [](lua_State* L) -> int {
            const char* label = luaL_checkstring(L, 1);
            ImVec4 color = ImVec4(luaL_checknumber(L, 2), luaL_checknumber(L, 3), luaL_checknumber(L, 4), luaL_checknumber(L, 5));
            if (ImGui::ColorButton(label, color)) {
                ImGui::OpenPopup("ColorPopup");
            }

            if (ImGui::BeginPopup("ColorPopup")) {
                ImGui::ColorPicker4("Color Picker", (float*)&color);
                ImGui::EndPopup();
            }

            lua_pushnumber(L, color.x);
            lua_pushnumber(L, color.y);
            lua_pushnumber(L, color.z);
            lua_pushnumber(L, color.w);
            return 4;
        }},
        {NULL, NULL}
    };

    luaL_newlib(L, imgui_funcs);
    lua_setglobal(L, "ImGuiC");

    return 0;
}

static int RenderFrameLua(lua_State* L) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) return 0;
    }

    ImGui_ImplDX10_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(g_windowTitle, NULL);
    
    int width = luaL_optinteger(L, 1, g_windowWidth);
    int height = luaL_optinteger(L, 2, g_windowHeight);
    ImGui::SetWindowSize(g_windowTitle, ImVec2((float)width, (float)height), ImGuiCond_Once);

    lua_getglobal(L, "lua_render");
    if (lua_isfunction(L, -1)) {
        lua_call(L, 0, 0);
    }

    ImGui::End();
    Render();
    g_pSwapChain->Present(1, 0);
    return 0;
}

static int CleanupLua(lua_State* L) {
    CleanupDeviceD3D();
    ImGui_ImplDX10_Shutdown();
    ImGui::DestroyContext();
    return 0;
}

static const struct luaL_Reg z_imgui[] = {
    {"initialize", InitializeLua},
    {"renderFrame", RenderFrameLua},
    {"cleanup", CleanupLua},
    {NULL, NULL}
};

extern "C" int luaopen_z_imgui(lua_State* L) {
    luaL_newlib(L, z_imgui);
    return 1;
}