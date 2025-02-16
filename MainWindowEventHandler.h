#ifndef MAINWINDOWEVENTHANDLER_H
#define MAINWINDOWEVENTHANDLER_H

#include <windows.h>
#include <memory>
#include "ISO.h"

class MainWindowEventHandler {
public:
    static LRESULT HandleCommand(HWND hwnd, WPARAM wParam, LPARAM lParam, HWND hwndTreeView, HWND hwndListView, std::unique_ptr<ISO>& iso);
    static LRESULT HandleNotify(HWND hwnd, WPARAM wParam, LPARAM lParam, HWND hwndTreeView, HWND hwndListView, const std::unique_ptr<ISO>& iso);
};

#endif // MAINWINDOWEVENTHANDLER_H