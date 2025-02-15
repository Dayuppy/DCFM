#include "MainWindow.h"
#include "resource.h"
#include "MainWindowLayout.h"
#include "MainWindowUtilities.h"
#include "MainWindowEventHandler.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <shlobj.h>

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
    hwndTreeView_ = CreateTreeView();
    hwndListView_ = CreateListView();

    return true;
}

void MainWindow::Show(int nCmdShow) {
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);
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

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_COMMAND:
        return HandleCommand(wParam, lParam);
    case WM_NOTIFY:
        return HandleNotify(lParam);
    case WM_SIZE:
        return HandleResize();
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }

    return 0;
}

void MainWindow::SetupMenu() {
    MainWindowLayout::SetupMenu(hwnd_);
}

void MainWindow::CreateToolbar() {
    hwndToolbar_ = MainWindowLayout::CreateToolbar(hwnd_, hInstance_);
}

HWND MainWindow::CreateTreeView() {
    return MainWindowLayout::CreateTreeView(hwnd_, hInstance_);
}

HWND MainWindow::CreateListView() {
    return MainWindowLayout::CreateListView(hwnd_, hInstance_);
}

LRESULT MainWindow::HandleCommand(WPARAM wParam, LPARAM lParam) {
    return MainWindowEventHandler::HandleCommand(hwnd_, wParam, lParam, hwndTreeView_, hwndListView_, iso_);
}

LRESULT MainWindow::HandleNotify(LPARAM lParam) {
    LPNMHDR pnmh = (LPNMHDR)lParam;
    if (pnmh->hwndFrom == hwndTreeView_ && pnmh->code == TVN_SELCHANGED) {
        LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)lParam;
        HTREEITEM hSelectedItem = pnmtv->itemNew.hItem;

        // Get the full path of the selected folder
        std::wstring folderPath = MainWindowUtilities::GetFullPathFromTreeViewItem(hwndTreeView_, hSelectedItem);

        // Populate the ListView with the contents of the selected folder
        MainWindowUtilities::PopulateListView(hwndListView_, iso_, folderPath);
    }
    return 0;
}

LRESULT MainWindow::HandleResize() {
    return MainWindowLayout::HandleResize(hwnd_, hwndToolbar_, hwndTreeView_, hwndListView_);
}

void MainWindow::LoadIsoAndDisplayTree(const std::wstring& isoPath) {
    MainWindowUtilities::LoadIsoAndDisplayTree(hwnd_, hwndTreeView_, hwndListView_, iso_, isoPath);
}

std::wstring MainWindow::stringToWstring(const std::string& str) {
    return MainWindowUtilities::stringToWstring(str);
}

std::string MainWindow::wstringToString(const std::wstring& wstr) {
    return MainWindowUtilities::wstringToString(wstr);
}