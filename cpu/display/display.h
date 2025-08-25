#pragma once
#include <windows.h>
#include <vector>

class display {
public:
    display(int width, int height);
    void update(const std::vector<unsigned char>& rgb);
    bool processMessages();

private:
    HWND hwnd;
    HDC hdc;
    BITMAPINFO bmi;
    int width, height;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void initWindow();
};
