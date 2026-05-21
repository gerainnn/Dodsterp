#include "hook.hpp"
#include "renderer.hpp"
#include "menu.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <MinHook.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND, UINT, WPARAM, LPARAM);

namespace overlay {

using PresentFn = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain*, UINT, UINT);
using ResizeBuffersFn = HRESULT(STDMETHODCALLTYPE*)(
    IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

namespace {

PresentFn       g_orig_present       = nullptr;
ResizeBuffersFn g_orig_resize        = nullptr;

WNDPROC         g_orig_wndproc       = nullptr;
HWND            g_game_hwnd          = nullptr;
ID3D11Device*   g_device             = nullptr;
ID3D11DeviceContext* g_context       = nullptr;
ID3D11RenderTargetView* g_rtv        = nullptr;
bool            g_initialized        = false;
bool            g_menu_visible       = true;

// --- получение vtable Present из временного swapchain ----------------------

bool acquire_present_vtable(void** out_present, void** out_resize) {
    HMODULE h = GetModuleHandleW(L"d3d11.dll");
    if (!h) {
        h = LoadLibraryW(L"d3d11.dll");
        if (!h) return false;
    }

    using D3D11CreateDeviceAndSwapChainFn = HRESULT(WINAPI*)(
        IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
        const D3D_FEATURE_LEVEL*, UINT, UINT, const DXGI_SWAP_CHAIN_DESC*,
        IDXGISwapChain**, ID3D11Device**, D3D_FEATURE_LEVEL*,
        ID3D11DeviceContext**);

    auto p_create = reinterpret_cast<D3D11CreateDeviceAndSwapChainFn>(
        GetProcAddress(h, "D3D11CreateDeviceAndSwapChain"));
    if (!p_create) return false;

    HWND tmp_hwnd = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW,
                                    0, 0, 100, 100, nullptr, nullptr, nullptr, nullptr);
    if (!tmp_hwnd) return false;

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = tmp_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL fl[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1 };
    IDXGISwapChain* sc = nullptr;
    ID3D11Device*   dev = nullptr;
    D3D_FEATURE_LEVEL got;
    ID3D11DeviceContext* ctx = nullptr;

    HRESULT hr = p_create(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                          fl, 2, D3D11_SDK_VERSION, &sd,
                          &sc, &dev, &got, &ctx);
    if (FAILED(hr) || !sc) {
        DestroyWindow(tmp_hwnd);
        return false;
    }

    void** vtable = *reinterpret_cast<void***>(sc);
    // IDXGISwapChain vtable: 8 = Present, 13 = ResizeBuffers
    *out_present = vtable[8];
    *out_resize  = vtable[13];

    sc->Release();
    ctx->Release();
    dev->Release();
    DestroyWindow(tmp_hwnd);
    return true;
}

// --- кастомная WndProc -----------------------------------------------------

LRESULT CALLBACK hk_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // toggle меню на INSERT (только rising edge)
    static bool insert_was_down = false;
    if (msg == WM_KEYDOWN && wp == VK_INSERT && !insert_was_down) {
        g_menu_visible = !g_menu_visible;
        insert_was_down = true;
    } else if (msg == WM_KEYUP && wp == VK_INSERT) {
        insert_was_down = false;
    }

    if (g_initialized && g_menu_visible) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        // когда меню открыто — съедаем мышь/клавиатуру чтоб не уходило в игру
        const ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse &&
            (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP ||
             msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP ||
             msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP ||
             msg == WM_MOUSEWHEEL  || msg == WM_MOUSEMOVE)) {
            return TRUE;
        }
        if (io.WantCaptureKeyboard &&
            (msg == WM_KEYDOWN || msg == WM_KEYUP ||
             msg == WM_CHAR    || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP)) {
            return TRUE;
        }
    }
    return CallWindowProcW(g_orig_wndproc, hwnd, msg, wp, lp);
}

// --- Present hook ----------------------------------------------------------

HRESULT STDMETHODCALLTYPE hk_present(IDXGISwapChain* sc, UINT sync, UINT flags) {
    if (!g_initialized) {
        if (SUCCEEDED(sc->GetDevice(__uuidof(ID3D11Device),
                                    reinterpret_cast<void**>(&g_device)))) {
            g_device->GetImmediateContext(&g_context);

            DXGI_SWAP_CHAIN_DESC sd{};
            sc->GetDesc(&sd);
            g_game_hwnd = sd.OutputWindow;

            ID3D11Texture2D* back = nullptr;
            sc->GetBuffer(0, __uuidof(ID3D11Texture2D),
                          reinterpret_cast<void**>(&back));
            if (back) {
                g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
                back->Release();
            }

            g_orig_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
                g_game_hwnd, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(hk_wndproc)));

            renderer::init(g_device, g_context, g_game_hwnd);
            g_initialized = true;
        }
    }

    if (g_initialized && g_rtv) {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (g_menu_visible) {
            menu::draw();
        }

        ImGui::Render();
        g_context->OMSetRenderTargets(1, &g_rtv, nullptr);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return g_orig_present(sc, sync, flags);
}

HRESULT STDMETHODCALLTYPE hk_resize(IDXGISwapChain* sc, UINT count,
                                    UINT w, UINT h, DXGI_FORMAT fmt,
                                    UINT flags) {
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }

    HRESULT r = g_orig_resize(sc, count, w, h, fmt, flags);

    if (SUCCEEDED(r) && g_device) {
        ID3D11Texture2D* back = nullptr;
        sc->GetBuffer(0, __uuidof(ID3D11Texture2D),
                      reinterpret_cast<void**>(&back));
        if (back) {
            g_device->CreateRenderTargetView(back, nullptr, &g_rtv);
            back->Release();
        }
    }
    return r;
}

} // namespace

// ---------------------------------------------------------------- public API

bool install_hook() {
    void* p_present = nullptr;
    void* p_resize  = nullptr;
    if (!acquire_present_vtable(&p_present, &p_resize)) return false;

    if (MH_Initialize() != MH_OK) return false;

    if (MH_CreateHook(p_present, &hk_present,
                      reinterpret_cast<void**>(&g_orig_present)) != MH_OK) {
        return false;
    }
    if (MH_CreateHook(p_resize, &hk_resize,
                      reinterpret_cast<void**>(&g_orig_resize)) != MH_OK) {
        return false;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) return false;
    return true;
}

void remove_hook() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();

    if (g_game_hwnd && g_orig_wndproc) {
        SetWindowLongPtrW(g_game_hwnd, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(g_orig_wndproc));
    }
    renderer::shutdown();
    if (g_rtv)     { g_rtv->Release();     g_rtv = nullptr; }
    if (g_context) { g_context->Release(); g_context = nullptr; }
    if (g_device)  { g_device->Release();  g_device = nullptr; }
}

} // namespace overlay
