#include "MainWindow.h"
#include "ISO.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <filesystem>
#include "resource.h"

void MainWindow::SetupMenu() {
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hEditMenu = CreatePopupMenu();
    HMENU hViewMenu = CreatePopupMenu();
    HMENU hHelpMenu = CreatePopupMenu();

    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, L"&Open");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_SAVE, L"&Save");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, L"&Exit");

    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_UNDO, L"&Undo");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_REDO, L"&Redo");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_COPY, L"&Copy");
    AppendMenu(hEditMenu, MF_STRING, ID_EDIT_PASTE, L"&Paste");

    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"&About");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hEditMenu, L"&Edit");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hViewMenu, L"&View");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

    SetMenu(hwnd_, hMenu);
}

void MainWindow::CreateToolbar() {
    hwndToolbar_ = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP, 0, 0, 0, 0,
        hwnd_, nullptr, hInstance_, nullptr);

    SendMessage(hwndToolbar_, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

    TBBUTTON tbb[6] = { 0 };

    tbb[0].iBitmap = 0;
    tbb[0].idCommand = ID_FILE_OPEN;
    tbb[0].fsState = TBSTATE_ENABLED;
    tbb[0].fsStyle = TBSTYLE_BUTTON;
    tbb[0].iString = (INT_PTR)L"Open";

    tbb[1].iBitmap = 1;
    tbb[1].idCommand = ID_FILE_SAVE;
    tbb[1].fsState = TBSTATE_ENABLED;
    tbb[1].fsStyle = TBSTYLE_BUTTON;
    tbb[1].iString = (INT_PTR)L"Save";

    tbb[2].iBitmap = 2;
    tbb[2].idCommand = ID_EDIT_UNDO;
    tbb[2].fsState = TBSTATE_ENABLED;
    tbb[2].fsStyle = TBSTYLE_BUTTON;
    tbb[2].iString = (INT_PTR)L"Undo";

    tbb[3].iBitmap = 3;
    tbb[3].idCommand = ID_EDIT_REDO;
    tbb[3].fsState = TBSTATE_ENABLED;
    tbb[3].fsStyle = TBSTYLE_BUTTON;
    tbb[3].iString = (INT_PTR)L"Redo";

    tbb[4].iBitmap = 4;
    tbb[4].idCommand = ID_EDIT_COPY;
    tbb[4].fsState = TBSTATE_ENABLED;
    tbb[4].fsStyle = TBSTYLE_BUTTON;
    tbb[4].iString = (INT_PTR)L"Copy";

    tbb[5].iBitmap = 5;
    tbb[5].idCommand = ID_EDIT_PASTE;
    tbb[5].fsState = TBSTATE_ENABLED;
    tbb[5].fsStyle = TBSTYLE_BUTTON;
    tbb[5].iString = (INT_PTR)L"Paste";

    TBADDBITMAP tbab;
    tbab.hInst = HINST_COMMCTRL;
    tbab.nID = IDB_STD_SMALL_COLOR;
    SendMessage(hwndToolbar_, TB_ADDBITMAP, 0, (LPARAM)&tbab);
    SendMessage(hwndToolbar_, TB_ADDBUTTONS, sizeof(tbb) / sizeof(TBBUTTON), (LPARAM)&tbb);

    // Finally, tell the toolbar to size itself based on the added buttons:
    SendMessage(hwndToolbar_, TB_AUTOSIZE, 0, 0);
}

void MainWindow::CreateTreeView() {
    hwndTreeView_ = CreateWindowEx(0, WC_TREEVIEW, L"Tree View",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
        0, 0, 0, 0,
        hwnd_, nullptr, hInstance_, nullptr);
}

void MainWindow::CreateListView() {
    hwndListView_ = CreateWindowEx(0, WC_LISTVIEW, L"List View",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_EDITLABELS,
        0, 0, 0, 0,
        hwnd_, nullptr, hInstance_, nullptr);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.cx = 100;

    lvc.pszText = const_cast<LPWSTR>(L"File Name");
    ListView_InsertColumn(hwndListView_, 0, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"File Path");
    ListView_InsertColumn(hwndListView_, 1, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"File Size");
    ListView_InsertColumn(hwndListView_, 2, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"Date Created");
    ListView_InsertColumn(hwndListView_, 3, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"Date Modified");
    ListView_InsertColumn(hwndListView_, 4, &lvc);
}

void MainWindow::PopulateTreeView() {
    TreeView_DeleteAllItems(hwndTreeView_);

    std::wstring isoName = stringToWstring(iso_->GetFileName());
    TVINSERTSTRUCT tvis = { 0 };
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;
    tvis.item.pszText = const_cast<LPWSTR>(isoName.c_str());
    HTREEITEM hRoot = TreeView_InsertItem(hwndTreeView_, &tvis);

    std::unordered_map<std::wstring, HTREEITEM> treeItems;
    treeItems[isoName] = hRoot;

    for (const auto& recordPair : iso_->GetDirectoryRecords()) {
        const auto& recordPathStr = recordPair.first;
        const auto& record = recordPair.second;

        if (record.IsFile()) continue;

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
                childTvis.item.pszText = const_cast<LPWSTR>(folder.c_str());
                HTREEITEM hItem = TreeView_InsertItem(hwndTreeView_, &childTvis);
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

void MainWindow::PopulateListView(const std::wstring& directoryPath) {
    ListView_DeleteAllItems(hwndListView_);

    LVITEM lvi;
    lvi.mask = LVIF_TEXT;
    lvi.iItem = 0;

    for (const auto& recordPair : iso_->GetDirectoryRecords()) {
        const auto& recordPath = recordPair.first;
        const auto& record = recordPair.second;

        std::wstring parentPath = std::filesystem::path(std::wstring(recordPath.begin(), recordPath.end())).parent_path().wstring();
        if (parentPath == directoryPath && record.IsFile()) {
            lvi.iSubItem = 0;
            lvi.pszText = const_cast<LPWSTR>(std::filesystem::path(std::wstring(recordPath.begin(), recordPath.end())).filename().c_str());
            ListView_InsertItem(hwndListView_, &lvi);

            lvi.iSubItem = 1;
            lvi.pszText = const_cast<LPWSTR>(std::wstring(recordPath.begin(), recordPath.end()).c_str());
            ListView_SetItem(hwndListView_, &lvi);

            lvi.iSubItem = 2;
            lvi.pszText = const_cast<LPWSTR>(std::to_wstring(record.GetSize()).c_str());
            ListView_SetItem(hwndListView_, &lvi);

            std::wstring dateTime = stringToWstring(record.GetFormattedDateTime());
            lvi.iSubItem = 3;
            lvi.pszText = const_cast<LPWSTR>(dateTime.c_str());
            ListView_SetItem(hwndListView_, &lvi);

            lvi.iSubItem = 4;
            lvi.pszText = const_cast<LPWSTR>(dateTime.c_str());
            ListView_SetItem(hwndListView_, &lvi);

            lvi.iItem++;
        }
    }
}