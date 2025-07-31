#include <windows.h>

class RGBDisplay
{
private:
    HWND hwnd;
    HDC hdc;
    BITMAPINFO bmi;
    int width, height, imageWidth, imageHeight;

public:
    bool windowOpen = true;
    RGBDisplay(int w, int h) : width(w), height(h), imageWidth(w), imageHeight(h)
    {
        // Create window
        WNDCLASSA wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = "RGBDisplay";
        RegisterClassA(&wc);

        hwnd = CreateWindowA("RGBDisplay", "Not So Postcard Pathtracer", WS_OVERLAPPEDWINDOW,
                             100, 100, width, height, nullptr, nullptr,
                             GetModuleHandle(nullptr), this);

        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        hdc = GetDC(hwnd);

        // Setup bitmap info
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = -height; // negative for top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 24;
        bmi.bmiHeader.biCompression = BI_RGB;

        ShowWindow(hwnd, SW_SHOW);
    }

    void update(unsigned char *rgbData)
    {
        // if (!windowOpen)
        // return;
        StretchDIBits(hdc,
                      0, 0, width, height,           // Draw to full window
                      0, 0, imageWidth, imageHeight, // From full RGB data
                      rgbData, &bmi, DIB_RGB_COLORS, SRCCOPY);
    }

    bool processMessages()
    {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                windowOpen = false;
                return false;
            }
            if (msg.message == WM_CLOSE)
                return false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        return true;
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        RGBDisplay *display = (RGBDisplay *)GetWindowLongPtrA(hwnd, GWLP_USERDATA);

        switch (uMsg)
        {
        case WM_SIZE:
        {
            if (display)
            {
                // Update window dimensions when resized
                display->width = LOWORD(lParam);
                display->height = HIWORD(lParam);

                // Force a repaint
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_CLOSE:
            if (display)
            {
                ShowWindow(hwnd, SW_HIDE);
                display->windowOpen = false;
            }
            return 0;
        case WM_DESTROY:
            if (display)
            {
                display->windowOpen = false;
            }
            return 0;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
        }
    }

    bool isWindowVisible() const
    {
        return windowOpen;
    }
};