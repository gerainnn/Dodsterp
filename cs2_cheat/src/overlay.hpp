#pragma once
#include <windows.h>
#include <d2d1.h>
#include <string>
#include <functional>

// Прозрачное topmost-окно с Direct2D рендером.
// Привязывается размером к client area внешнего окна (CS2).
class Overlay {
public:
    Overlay() = default;
    ~Overlay();

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    bool create(HWND target_window);
    void destroy();

    // Пересинхронизировать положение/размер с окном CS2.
    void sync_to_target();

    // Один кадр рендера. draw — колбэк, в нём ID2D1RenderTarget уже готов
    // (BeginDraw сделан, EndDraw вызовется после).
    void frame(std::function<void(ID2D1HwndRenderTarget*)> draw);

    int width()  const { return width_; }
    int height() const { return height_; }

    // Прокачать сообщения чтоб окно не зависло; возвращает false если WM_QUIT.
    bool pump();

    HWND hwnd() const { return hwnd_; }

private:
    HWND   hwnd_   = nullptr;
    HWND   target_ = nullptr;
    int    width_  = 0;
    int    height_ = 0;

    ID2D1Factory*          d2d_factory_ = nullptr;
    ID2D1HwndRenderTarget* rt_          = nullptr;

    bool init_d2d_();
    void resize_rt_(int w, int h);

    static LRESULT CALLBACK wnd_proc(HWND, UINT, WPARAM, LPARAM);
};
