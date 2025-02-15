#ifndef MAINWINDOWUTILITIES_H
#define MAINWINDOWUTILITIES_H

#include <windows.h>
#include <string>
#include <memory>
#include "ISO.h"
#include <commctrl.h>

class MainWindowUtilities {
public:
    static void LoadIsoAndDisplayTree(HWND hwnd, HWND hwndTreeView, HWND hwndListView, std::unique_ptr<ISO>& iso, const std::wstring& isoPath);
    static void PopulateTreeView(HWND hwndTreeView, const std::unique_ptr<ISO>& iso, const std::wstring& isoName);
    static void PopulateListView(HWND hwndListView, const std::unique_ptr<ISO>& iso, const std::wstring& folderPath);
    static std::wstring GetFullPathFromTreeViewItem(HWND hwndTreeView, HTREEITEM hItem);
    static std::wstring stringToWstring(const std::string& str);
    static std::string wstringToString(const std::wstring& wstr);
};

#endif // MAINWINDOWUTILITIES_H