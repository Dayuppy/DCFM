#include "MainWindowUtilities.h"
#include "ISO.h"
#include "DirectoryRecord.h"
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
#include <Windows.h>
#include <unordered_map>
#include <algorithm>

void MainWindowUtilities::LoadIsoAndDisplayTree(HWND hwnd, HWND hwndTreeView, HWND hwndListView, std::unique_ptr<ISO>& iso, const std::wstring& isoPath) {
    try {
        // Convert the ISO path (wstring) to a std::string using our updated conversion function.
        iso = std::make_unique<ISO>(wstringToString(isoPath));
        iso->LoadISO();
        std::wcout << L"Loaded ISO: " << isoPath << std::endl;
        std::wcout << L"Directory Records count: " << iso->GetDirectoryRecords().size() << std::endl;
        std::wcout << L"File Records count: " << iso->GetFileRecords().size() << std::endl;

        // Clear the TreeView and ListView
        TreeView_DeleteAllItems(hwndTreeView);
        ListView_DeleteAllItems(hwndListView);

        // Get the ISO name from the path (the stem)
        std::wstring isoName = std::filesystem::path(isoPath).stem().wstring();

        // Populate the TreeView and ListView
        MainWindowUtilities::PopulateTreeView(hwndTreeView, iso, isoName);
        PopulateListView(hwndListView, iso, isoName);
    }
    catch (const std::exception& ex) {
        std::cerr << "Error loading ISO file: " << ex.what() << std::endl;
    }
}

void MainWindowUtilities::PopulateTreeView(HWND hwndTreeView, const std::unique_ptr<ISO>& iso, const std::wstring& isoName) {
    TreeView_DeleteAllItems(hwndTreeView);
    std::unordered_map<std::wstring, HTREEITEM> treeItems;

    // Insert the root item using the ISO name.
    TVINSERTSTRUCT tvisRoot = { 0 };
    tvisRoot.hParent = TVI_ROOT;
    tvisRoot.hInsertAfter = TVI_SORT;
    tvisRoot.item.mask = TVIF_TEXT;
    tvisRoot.item.pszText = const_cast<LPWSTR>(isoName.c_str());
    HTREEITEM hRoot = TreeView_InsertItem(hwndTreeView, &tvisRoot);
    treeItems[isoName] = hRoot;

    // Loop through each directory record from the ISO.
    for (const auto& recordPair : iso->GetDirectoryRecords()) {
        const auto& recordPathStr = recordPair.first; // Expected format: e.g. "MODULES" or "CONF\NET"
        const auto& record = recordPair.second;
        if (record.IsDirectory() && !recordPathStr.empty()) {

            // Determine the full tree path.
            // If the key does not already begin with the root name, prepend it.
            std::wstring fullPath = stringToWstring(recordPathStr);;
            
            if (fullPath.compare(0, isoName.size(), isoName) != 0)
                fullPath = isoName + L"\\" + fullPath;

            // Tokenize the fullPath by backslashes.
            std::wistringstream iss(fullPath);
            std::wstring token;
            HTREEITEM hParent = hRoot;
            std::wstring accum;
            while (std::getline(iss, token, L'\\')) {
                if (token.empty()) continue;
                if (accum.empty())
                    accum = token;
                else
                    accum += L"\\" + token;
                // Insert a new node if this branch hasn't been inserted yet.
                if (treeItems.find(accum) == treeItems.end()) {
                    TVINSERTSTRUCT tvis = { 0 };
                    tvis.hParent = hParent;
                    tvis.hInsertAfter = TVI_SORT;
                    tvis.item.mask = TVIF_TEXT;
                    tvis.item.pszText = const_cast<LPWSTR>(token.c_str());
                    HTREEITEM hItem = TreeView_InsertItem(hwndTreeView, &tvis);
                    treeItems[accum] = hItem;
                    hParent = hItem;
                }
                else {
                    hParent = treeItems[accum];
                }
            }
        }
    }
}

void MainWindowUtilities::PopulateListView(HWND hwndListView, const std::unique_ptr<ISO>& iso, const std::wstring& folderPath) {
    ListView_DeleteAllItems(hwndListView);
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;
    std::wcout << L"Populating ListView for folder: " << folderPath << std::endl;

    auto populateList = [&](const std::unordered_map<std::string, DirectoryRecord>& records) {
        for (const auto& recordPair : records) {
            const auto& recordPath = recordPair.first; // std::string
            const auto& record = recordPair.second;

            // Convert parent path from the record's path.
            std::wstring parentPath = stringToWstring(std::filesystem::path(recordPath).parent_path().string());
            if (parentPath.empty() || parentPath == L"\\" || parentPath == L"\0") {
                parentPath = stringToWstring(iso->GetRootFolderName());
            }

            // Normalize folderPath by removing a trailing backslash if present.
            std::wstring normalizedFolderPath = folderPath;

            if (!normalizedFolderPath.empty() && normalizedFolderPath.back() == L'\\') {
                normalizedFolderPath.pop_back();
            }

            // Normalize parentPath by removing a trailing backslash if present.
            std::wstring normalizedparentPath = parentPath;

            if (!normalizedparentPath.empty() && normalizedparentPath.back() == L'\\') {
                normalizedparentPath.pop_back();
            }

            std::wcout << L"Checking record: " << normalizedFolderPath
                << L" (Parent: " << normalizedparentPath << L")" << std::endl;

            if (normalizedparentPath == normalizedFolderPath) {
                std::wcout << L"Inserting record: " << stringToWstring(recordPath) << std::endl;

                // Insert file name
                std::wstring fileName = stringToWstring(std::filesystem::path(recordPath).filename().string());
                lvi.iSubItem = 0;
                lvi.pszText = const_cast<LPWSTR>(fileName.c_str());
                ListView_InsertItem(hwndListView, &lvi);

                // Insert full path
                std::wstring recordPathCopy = stringToWstring(recordPath);
                lvi.iSubItem = 1;
                lvi.pszText = const_cast<LPWSTR>(recordPathCopy.c_str());
                ListView_SetItem(hwndListView, &lvi);

                // Insert file size
                std::wstring fileSize = std::to_wstring(record.DataLength.Value());
                lvi.iSubItem = 2;
                lvi.pszText = const_cast<LPWSTR>(fileSize.c_str());
                ListView_SetItem(hwndListView, &lvi);

                // Insert LBA (logical block address)
                std::wstringstream lbaStream;
                lbaStream << record.ExtentLocation.Value();
                std::wstring lba = lbaStream.str();
                lvi.iSubItem = 3;
                lvi.pszText = const_cast<LPWSTR>(lba.c_str());
                ListView_SetItem(hwndListView, &lvi);

                // Insert sector length (here, reusing DataLength)
                std::wstringstream sectorLengthStream;
                sectorLengthStream << record.DataLength.Value();
                std::wstring sectorLength = sectorLengthStream.str();
                lvi.iSubItem = 4;
                lvi.pszText = const_cast<LPWSTR>(sectorLength.c_str());
                ListView_SetItem(hwndListView, &lvi);

                lvi.iItem++;
            }
        }
        };

    populateList(iso->GetDirectoryRecords());
    populateList(iso->GetFileRecords());
}

void MainWindowUtilities::OnTreeViewItemSelectionChanged(HWND hwndTreeView, HWND hwndListView, const std::unique_ptr<ISO>& iso) {
    HTREEITEM hSelectedItem = TreeView_GetSelection(hwndTreeView);
    if (hSelectedItem) {
        std::wstring selectedPath = GetFullPathFromTreeViewItem(hwndTreeView, hSelectedItem);
        std::wcout << L"Selected TreeView item path: " << selectedPath << std::endl;
        // Update ListView with the contents of the selected folder
        PopulateListView(hwndListView, iso, selectedPath);
    }
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
        text[sizeof(text) / sizeof(text[0]) - 1] = '\0'; // ensure null-termination
        fullPath = item.pszText + std::wstring(L"\\") + fullPath;
        hItem = TreeView_GetParent(hwndTreeView, hItem);
    }
    if (!fullPath.empty() && fullPath.back() == L'\\') {
        fullPath.pop_back();
    }
    return fullPath;
}

// Updated conversion functions using the corrected logic
std::wstring MainWindowUtilities::stringToWstring(const std::string& str) {
    if (str.empty())
        return std::wstring();

    // Determine the required size (including the null terminator)
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (size_needed <= 0) {
        throw std::runtime_error("MultiByteToWideChar failed to determine size.");
    }
    // Allocate a temporary buffer.
    std::vector<wchar_t> buffer(size_needed);
    int ret = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buffer.data(), size_needed);
    if (ret == 0) {
        throw std::runtime_error("MultiByteToWideChar conversion failed.");
    }
    // ret includes the null terminator; construct wstring without it.
    return std::wstring(buffer.data(), ret - 1);
}

std::string MainWindowUtilities::wstringToString(const std::wstring& wstr) {
    if (wstr.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) {
        throw std::runtime_error("WideCharToMultiByte failed to determine size.");
    }
    std::vector<char> buffer(size_needed);
    int ret = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buffer.data(), size_needed, nullptr, nullptr);
    if (ret == 0) {
        throw std::runtime_error("WideCharToMultiByte conversion failed.");
    }
    return std::string(buffer.data(), ret - 1);
}