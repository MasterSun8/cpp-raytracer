#include "display.h"
#include <windows.h>

LRESULT CALLBACK display::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_DESTROY)
        PostQuitMessage(0);
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

display::display(int w, int h) : width(w), height(h)
{
    initWindow();
}

void display::initWindow()
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "displayClass";
    RegisterClass(&wc);

    hwnd = CreateWindowA(wc.lpszClassName, "display", WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                         nullptr, nullptr, wc.hInstance, nullptr);

    hdc = GetDC(hwnd);

    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    ShowWindow(hwnd, SW_SHOW);
}

void display::update(const std::vector<unsigned char> &rgb)
{
    if (rgb.size() != width * height * 3)
        return;
    SetDIBitsToDevice(hdc,
                      0,
                      0,
                      width,
                      height,
                      0,
                      0,
                      0,
                      height,
                      rgb.data(),
                      &bmi,
                      DIB_RGB_COLORS);
}

bool display::processMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            return false;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}
