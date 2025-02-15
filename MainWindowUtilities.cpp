#include "MainWindow.h"
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

void MainWindow::LoadIsoAndDisplayTree(const std::wstring& isoPath) {
    try {
        iso_ = std::make_unique<ISO>(std::string(isoPath.begin(), isoPath.end()));
        iso_->LoadISO();
        std::wcout << L"Loaded ISO: " << isoPath << std::endl;
        std::wcout << L"Directory Records count: " << iso_->GetDirectoryRecords().size() << std::endl;

        // Clear the TreeView and ListView
        TreeView_DeleteAllItems(hwndTreeView_);
        ListView_DeleteAllItems(hwndListView_);

        // Get the ISO name from the path
        std::wstring isoName = std::filesystem::path(isoPath).stem().wstring();

        PopulateTreeView();
    }
    catch (const std::exception& ex) {
        std::cerr << "Error loading ISO file: " << ex.what() << std::endl;
    }
}

void MainWindow::BuildDirectoryTree(const std::unordered_map<std::string, DirectoryRecord>& directoryRecords, const std::wstring& parentPath, HTREEITEM parentNode, std::unordered_set<std::wstring>& visitedDirectories, const std::wstring& isoName) {
    std::vector<std::pair<std::string, DirectoryRecord>> filesAndDirs(directoryRecords.begin(), directoryRecords.end());
    std::sort(filesAndDirs.begin(), filesAndDirs.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
        });

    std::vector<std::pair<std::string, DirectoryRecord>> directories;
    std::vector<std::pair<std::string, DirectoryRecord>> files;

    for (const auto& kvp : filesAndDirs) {
        if (!kvp.second.IsFile()) {
            directories.push_back(kvp);
        }
        else {
            files.push_back(kvp);
        }
    }

    for (const auto& dir : directories) {
        std::wstring directoryPath = stringToWstring(dir.first);
        directoryPath.erase(directoryPath.find_last_not_of(L" \t\n\r\f\v\\") + 1);

        if (directoryPath.empty() || visitedDirectories.find(directoryPath) != visitedDirectories.end()) {
            continue;
        }

        bool isRealDirectory = std::any_of(directoryRecords.begin(), directoryRecords.end(), [&directoryPath](const auto& record) {
            return record.first.find(wstringToString(directoryPath) + '\\') == 0;
            });

        if (!isRealDirectory) {
            continue;
        }

        visitedDirectories.insert(directoryPath);

        TVINSERTSTRUCT tvis = { 0 };
        tvis.hParent = parentNode;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT;
        tvis.item.pszText = const_cast<LPWSTR>(std::filesystem::path(directoryPath).filename().c_str());
        HTREEITEM hDirNode = TreeView_InsertItem(hwndTreeView_, &tvis);

        std::wcout << L"Adding directory to TreeView: " << directoryPath << std::endl;

        BuildDirectoryTree(directoryRecords, directoryPath, hDirNode, visitedDirectories, isoName);
    }

    for (const auto& file : files) {
        std::wstring filePath = stringToWstring(file.first);
        filePath.erase(filePath.find_last_not_of(L" \t\n\r\f\v") + 1);

        if (filePath.empty()) {
            continue;
        }

        LVITEM lvi = { 0 };
        lvi.mask = LVIF_TEXT;
        lvi.iItem = ListView_GetItemCount(hwndListView_);
        lvi.pszText = const_cast<LPWSTR>(std::filesystem::path(filePath).filename().c_str());
        ListView_InsertItem(hwndListView_, &lvi);

        lvi.iSubItem = 1;
        lvi.pszText = const_cast<LPWSTR>(filePath.c_str());
        ListView_SetItem(hwndListView_, &lvi);

        std::wcout << L"Adding file to ListView: " << filePath << std::endl;
    }

    if (parentNode == TVI_ROOT || parentPath.empty()) {
        TVITEM tvi = { 0 };
        tvi.mask = TVIF_TEXT;
        tvi.hItem = parentNode;
        tvi.pszText = const_cast<LPWSTR>(isoName.c_str());
        TreeView_SetItem(hwndTreeView_, &tvi);
    }
}

void MainWindow::ExtractAll() {
    if (!iso_) {
        MessageBox(hwnd_, L"No ISO loaded to extract.", L"Error", MB_ICONERROR);
        return;
    }

    // Use SHBrowseForFolder to get a folder selection dialog
    BROWSEINFO bi = { 0 };
    bi.lpszTitle = L"Select Destination Folder for Extraction";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (!pidl) {
        MessageBox(hwnd_, L"Extraction canceled.", L"Info", MB_ICONINFORMATION);
        return;
    }

    wchar_t destinationPath[MAX_PATH];
    if (!SHGetPathFromIDList(pidl, destinationPath)) {
        MessageBox(hwnd_, L"Failed to get selected folder path.", L"Error", MB_ICONERROR);
        CoTaskMemFree(pidl);
        return;
    }
    CoTaskMemFree(pidl);

    // Remove trailing slashes
    std::wstring destPath(destinationPath);
    if (!destPath.empty() && (destPath.back() == L' ' || destPath.back() == L'\\')) {
        destPath.pop_back();
    }

    try {
        const auto& directoryRecords = iso_->GetDirectoryRecords();
        for (const auto& recordPair : directoryRecords) {
            const auto& recordPath = recordPair.first;
            const auto& record = recordPair.second;

            std::wstring fullPath = destPath + L"\\" + stringToWstring(recordPath);
            std::replace(fullPath.begin(), fullPath.end(), L'/', L'\\');

            if (record.IsFile()) {
                // Ensure directories exist
                std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());

                std::ofstream outFile(fullPath, std::ios::binary);
                if (!outFile) {
                    std::wcerr << L"Failed to create file: " << fullPath << std::endl;
                    continue;
                }

                auto fileData = iso_->ReadFileData(record);
                outFile.write(reinterpret_cast<const char*>(fileData.data()), fileData.size());
                outFile.close();
            }
            else {
                // Create directory
                std::filesystem::create_directories(fullPath);
            }
        }

        MessageBox(hwnd_, L"Extraction completed successfully.", L"Info", MB_ICONINFORMATION);
    }
    catch (const std::exception& ex) {
        std::string errorMsg = "Error during extraction: ";
        errorMsg += ex.what();
        MessageBoxA(hwnd_, errorMsg.c_str(), "Error", MB_ICONERROR);
    }
}

std::wstring MainWindow::FormatFileTime(const std::time_t& time) {
    struct tm timeInfo;
    localtime_s(&timeInfo, &time);

    std::wstringstream ss;
    ss << std::put_time(&timeInfo, L"%Y-%m-%d %H:%M:%S");
    return ss.str();
}


std::wstring MainWindow::stringToWstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string MainWindow::wstringToString(const std::wstring& wstr) {
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}