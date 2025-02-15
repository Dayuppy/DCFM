#ifndef MAINWINDOWLAYOUT_H
#define MAINWINDOWLAYOUT_H

#include <windows.h>

class MainWindowLayout {
public:
    static void SetupMenu(HWND hwnd);
    static HWND CreateToolbar(HWND hwnd, HINSTANCE hInstance);
    static HWND CreateTreeView(HWND hwnd, HINSTANCE hInstance);
    static HWND CreateListView(HWND hwnd, HINSTANCE hInstance);
    static LRESULT HandleResize(HWND hwnd, HWND hwndToolbar, HWND hwndTreeView, HWND hwndListView);
};

#endif // MAINWINDOWLAYOUT_H