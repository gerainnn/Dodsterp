#include "overlay.hpp"
#include <dwmapi.h>

static const wchar_t* WND_CLASS = L"cs2_external_overlay_v1";

LRESULT CALLBACK Overlay::wnd_proc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_NCHITTEST:
            // на всякий случай — даже без WS_EX_TRANSPARENT не цепляем мышь
            return HTTRANSPARENT;
    }
    return DefWindowProcW(h, msg, w, l);
}

bool Overlay::create(HWND target_window) {
    target_ = target_window;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = &Overlay::wnd_proc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = WND_CLASS;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    RegisterClassExW(&wc);

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST |
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        WND_CLASS, L"",
        WS_POPUP,
        0, 0, 800, 600,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd_) return false;

    // чёрный — color key — становится прозрачным
    SetLayeredWindowAttributes(hwnd_, RGB(0, 0, 0), 0, LWA_COLORKEY);

    if (!init_d2d_()) return false;

    sync_to_target();
    ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd_);
    return true;
}

bool Overlay::init_d2d_() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2d_factory_);
    if (FAILED(hr)) return false;

    RECT rc{};
    GetClientRect(hwnd_, &rc);
    width_  = rc.right  - rc.left;
    height_ = rc.bottom - rc.top;
    if (width_  < 1) width_  = 1;
    if (height_ < 1) height_ = 1;

    D2D1_RENDER_TARGET_PROPERTIES props =
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_HARDWARE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96, 96);

    D2D1_HWND_RENDER_TARGET_PROPERTIES hwnd_props =
        D2D1::HwndRenderTargetProperties(
            hwnd_,
            D2D1::SizeU(width_, height_),
            D2D1_PRESENT_OPTIONS_IMMEDIATELY);

    hr = d2d_factory_->CreateHwndRenderTarget(props, hwnd_props, &rt_);
    return SUCCEEDED(hr);
}

void Overlay::resize_rt_(int w, int h) {
    if (!rt_) return;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    rt_->Resize(D2D1::SizeU(w, h));
    width_  = w;
    height_ = h;
}

void Overlay::sync_to_target() {
    if (!target_ || !IsWindow(target_)) return;

    RECT rc{};
    if (!GetClientRect(target_, &rc)) return;

    POINT tl{ rc.left, rc.top };
    ClientToScreen(target_, &tl);

    int new_w = rc.right - rc.left;
    int new_h = rc.bottom - rc.top;

    SetWindowPos(hwnd_, HWND_TOPMOST,
                 tl.x, tl.y, new_w, new_h,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    if (new_w != width_ || new_h != height_)
        resize_rt_(new_w, new_h);
}

void Overlay::frame(std::function<void(ID2D1HwndRenderTarget*)> draw) {
    if (!rt_) return;
    rt_->BeginDraw();
    rt_->Clear(D2D1::ColorF(0, 0, 0, 0));   // полностью прозрачно
    if (draw) draw(rt_);
    HRESULT hr = rt_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        if (rt_) { rt_->Release(); rt_ = nullptr; }
        init_d2d_();
    }
}

bool Overlay::pump() {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

void Overlay::destroy() {
    if (rt_) { rt_->Release(); rt_ = nullptr; }
    if (d2d_factory_) { d2d_factory_->Release(); d2d_factory_ = nullptr; }
    if (hwnd_) { DestroyWindow(hwnd_); hwnd_ = nullptr; }
}

Overlay::~Overlay() { destroy(); }
