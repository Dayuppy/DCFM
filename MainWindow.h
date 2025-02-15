#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Windows.h>
#include <CommCtrl.h>
#include <memory>
#include <unordered_set>
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
    void CreateTreeView();
    void CreateListView();
    void LoadIsoAndDisplayTree(const std::wstring& isoPath);
    void BuildDirectoryTreeIterative(const std::unordered_map<std::string, DirectoryRecord>& directoryRecords, const std::wstring& parentPath, HTREEITEM parentNode, std::unordered_set<std::wstring>& visitedDirectories, const std::wstring& isoName);
    void BuildDirectoryTree(const std::unordered_map<std::string, DirectoryRecord>& directoryRecords, const std::wstring& parentPath, HTREEITEM parentNode, std::unordered_set<std::wstring>& visitedDirectories, const std::wstring& isoName);
    void ExtractAll();
    void PopulateTreeView();
    void PopulateListView(const std::wstring& directoryPath);

    static std::wstring stringToWstring(const std::string& str);
    static std::string wstringToString(const std::wstring& wstr);
    static std::wstring FormatFileTime(const std::time_t& time);

    HINSTANCE hInstance_;
    HWND hwnd_;
    HWND hwndToolbar_;
    HWND hwndTreeView_;
    HWND hwndListView_;
    std::unique_ptr<ISO> iso_;
};

#endif // MAINWINDOW_H