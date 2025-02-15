#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <windows.h>
#include <memory>
#include <string>
#include "ISO.h"

class MainWindow {
public:
    MainWindow(HINSTANCE hInstance);
    ~MainWindow();

    bool Create(LPCWSTR windowName, int width, int height);
    void Show(int nCmdShow);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetupMenu();
    void CreateToolbar();
    HWND CreateTreeView();
    HWND CreateListView();

    LRESULT HandleCommand(WPARAM wParam, LPARAM lParam);
    LRESULT HandleNotify(LPARAM lParam);
    LRESULT HandleResize();

    void LoadIsoAndDisplayTree(const std::wstring& isoPath);

    std::wstring stringToWstring(const std::string& str);
    std::string wstringToString(const std::wstring& wstr);

private:
    HINSTANCE hInstance_;
    HWND hwnd_;
    HWND hwndToolbar_;
    HWND hwndTreeView_;
    HWND hwndListView_;
    std::unique_ptr<ISO> iso_;
};

#endif // MAINWINDOW_H