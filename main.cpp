#include "MainWindow.h"
#include <windows.h>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    MainWindow mainWindow(hInstance);

    if (!mainWindow.Create(L"Dark Cloud File Manager", 800, 600)) {
        return 0;
    }

    mainWindow.Show(nCmdShow);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) 
{
    wWinMain(hInstance, hPrevInstance, lpCmdLine, 1);
    return 0;
}