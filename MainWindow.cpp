#include "MainWindow.h"
#include "resource.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <shlobj.h> // Required for SHBrowseForFolder, SHGetPathFromIDList, etc.

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Shlwapi.lib")

MainWindow::MainWindow(HINSTANCE hInstance)
    : hInstance_(hInstance), hwnd_(nullptr), hwndToolbar_(nullptr), hwndTreeView_(nullptr), hwndListView_(nullptr) {
}

MainWindow::~MainWindow() {
    if (iso_) {
        iso_.reset();
    }
}

bool MainWindow::Create(LPCWSTR windowName, int width, int height) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInstance_;
    wc.lpszClassName = L"MainWindowClass";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(hInstance_, MAKEINTRESOURCE(IDI_APP_ICON));

    RegisterClass(&wc);

    hwnd_ = CreateWindowEx(
        0,
        L"MainWindowClass",
        windowName,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr,
        nullptr,
        hInstance_,
        this
    );

    if (!hwnd_) {
        return false;
    }

    SetupMenu();
    CreateToolbar();
    CreateTreeView();
    CreateListView();

    return true;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(hwnd_, nCmdShow);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

        pThis->hwnd_ = hwnd;
    }
    else {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }
    else {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}