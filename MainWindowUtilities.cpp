#include "MainWindowUtilities.h"
#include "ISO.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <locale>
#include <codecvt>
#include <filesystem>

void MainWindowUtilities::LoadIsoAndDisplayTree(HWND hwnd, HWND hwndTreeView, HWND hwndListView, std::unique_ptr<ISO>& iso, const std::wstring& isoPath) {
    try {
        iso = std::make_unique<ISO>(std::string(isoPath.begin(), isoPath.end()));
        iso->LoadISO();
        std::wcout << L"Loaded ISO: " << isoPath << std::endl;
        std::wcout << L"Directory Records count: " << iso->GetDirectoryRecords().size() << std::endl;
        std::wcout << L"File Records count: " << iso->GetFileRecords().size() << std::endl;

        // Clear the TreeView and ListView
        TreeView_DeleteAllItems(hwndTreeView);
        ListView_DeleteAllItems(hwndListView);

        // Get the ISO name from the path
        std::wstring isoName = std::filesystem::path(isoPath).stem().wstring();

        // Populate TreeView
        PopulateTreeView(hwndTreeView, iso, isoName);

        // Populate ListView with the root directory files
        PopulateListView(hwndListView, iso, isoName);
    }
    catch (const std::exception& ex) {
        std::cerr << "Error loading ISO file: " << ex.what() << std::endl;
    }
}

void MainWindowUtilities::PopulateTreeView(HWND hwndTreeView, const std::unique_ptr<ISO>& iso, const std::wstring& isoName) {
    TreeView_DeleteAllItems(hwndTreeView);

    TVINSERTSTRUCT tvis = { 0 };
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;
    tvis.item.pszText = const_cast<LPWSTR>(isoName.c_str());
    HTREEITEM hRoot = TreeView_InsertItem(hwndTreeView, &tvis);

    std::unordered_map<std::wstring, HTREEITEM> treeItems;
    treeItems[isoName] = hRoot;

    for (const auto& recordPair : iso->GetDirectoryRecords()) {
        const auto& recordPathStr = recordPair.first;
        const auto& record = recordPair.second;

        if (record.IsDirectory()) {
            std::wstring path = stringToWstring(recordPathStr);
            std::replace(path.begin(), path.end(), L'/', L'\\');

            std::wstring fullPath = isoName;
            HTREEITEM hParent = hRoot;

            size_t start = 0;
            while (true) {
                size_t pos = path.find(L'\\', start);
                std::wstring folder = (pos == std::wstring::npos) ? path.substr(start) : path.substr(start, pos - start);

                if (folder.empty()) break;

                fullPath += L"\\" + folder;
                if (treeItems.find(fullPath) == treeItems.end()) {
                    TVINSERTSTRUCT childTvis = { 0 };
                    childTvis.hParent = hParent;
                    childTvis.hInsertAfter = TVI_SORT;
                    childTvis.item.mask = TVIF_TEXT;
                    std::wstring folderCopy = folder; // Create a copy to avoid dangling pointer
                    childTvis.item.pszText = const_cast<LPWSTR>(folderCopy.c_str());
                    HTREEITEM hItem = TreeView_InsertItem(hwndTreeView, &childTvis);
                    treeItems[fullPath] = hItem;
                    hParent = hItem;
                }
                else {
                    hParent = treeItems[fullPath];
                }

                if (pos == std::wstring::npos) break;
                start = pos + 1;
            }
        }
    }
}

void MainWindowUtilities::PopulateListView(HWND hwndListView, const std::unique_ptr<ISO>& iso, const std::wstring& folderPath) {
    ListView_DeleteAllItems(hwndListView);

    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;

    auto populateList = [&](const std::unordered_map<std::string, DirectoryRecord>& records) {
        for (const auto& recordPair : records) {
            const auto& recordPath = recordPair.first;
            const auto& record = recordPair.second;

            std::wstring parentPath = std::filesystem::path(recordPath.begin(), recordPath.end()).parent_path().wstring();
            if (parentPath == folderPath) {
                std::wstring fileName = std::filesystem::path(recordPath.begin(), recordPath.end()).filename().wstring(); // Create a copy to avoid dangling pointer
                lvi.iSubItem = 0;
                lvi.pszText = const_cast<LPWSTR>(fileName.c_str());
                ListView_InsertItem(hwndListView, &lvi);

                std::wstring recordPathCopy = stringToWstring(recordPath); // Create a copy to avoid dangling pointer
                lvi.iSubItem = 1;
                lvi.pszText = const_cast<LPWSTR>(recordPathCopy.c_str());
                ListView_SetItem(hwndListView, &lvi);

                std::wstring fileSize = std::to_wstring(record.DataLength.Value()); // Create a copy to avoid dangling pointer
                lvi.iSubItem = 2;
                lvi.pszText = const_cast<LPWSTR>(fileSize.c_str());
                ListView_SetItem(hwndListView, &lvi);

                std::wstring dateTime = stringToWstring(record.GetFormattedDateTime());
                lvi.iSubItem = 3;
                lvi.pszText = const_cast<LPWSTR>(dateTime.c_str());
                ListView_SetItem(hwndListView, &lvi);

                lvi.iSubItem = 4;
                lvi.pszText = const_cast<LPWSTR>(dateTime.c_str());
                ListView_SetItem(hwndListView, &lvi);

                lvi.iItem++;
            }
        }
        };

    populateList(iso->GetDirectoryRecords());
    populateList(iso->GetFileRecords());
}

std::wstring MainWindowUtilities::GetFullPathFromTreeViewItem(HWND hwndTreeView, HTREEITEM hItem) {
    std::wstring fullPath;
    TVITEM item;
    item.hItem = hItem;
    item.mask = TVIF_TEXT;
    WCHAR text[256];
    item.pszText = text;
    item.cchTextMax = sizeof(text) / sizeof(text[0]);

    while (hItem != nullptr) {
        item.hItem = hItem;
        TreeView_GetItem(hwndTreeView, &item);
        text[sizeof(text) / sizeof(text[0]) - 1] = '\0'; // Ensure null-termination
        fullPath = item.pszText + std::wstring(L"\\") + fullPath;
        hItem = TreeView_GetParent(hwndTreeView, hItem);
    }

    if (!fullPath.empty() && fullPath.back() == L'\\') {
        fullPath.pop_back();
    }

    return fullPath;
}

std::wstring MainWindowUtilities::stringToWstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string MainWindowUtilities::wstringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}