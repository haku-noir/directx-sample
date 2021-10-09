#include <Windows.h>
#include <tchar.h>
#ifdef _DEBUG
    #include <iostream>
#endif // _DEBUG

#define WINDOW_CLASS _T("DX12Sample")
#define WINDOW_TITLE _T("DX12ƒeƒXƒg")
#define	WINDOW_STYLE WS_OVERLAPPEDWINDOW
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

using namespace std;

LRESULT WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

#ifdef _DEBUG
int main() {
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif // _DEBUG
    WNDCLASSEX w = {};
    w.cbSize = sizeof(WNDCLASSEX);
    w.lpfnWndProc = (WNDPROC)WindowProcedure;
    w.lpszClassName = WINDOW_CLASS;
    w.hInstance = GetModuleHandle(nullptr);

    RegisterClassEx(&w);

    RECT wrc = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wrc, WINDOW_STYLE, false);

    HWND hwnd = CreateWindow(
        w.lpszClassName,
        WINDOW_TITLE,
        WINDOW_STYLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wrc.right - wrc.left,
        wrc.bottom - wrc.top,
        nullptr,
        nullptr,
        w.hInstance,
        nullptr
    );
    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};

    while (true) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) {
            break;
        }
    }
    UnregisterClass(w.lpszClassName, w.hInstance);

    return 0;
}