#include "MainWindowLayout.h"
#include "resource.h"
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <shlobj.h>

void MainWindowLayout::SetupMenu(HWND hwnd) {
    HMENU hMenu = CreateMenu();
    HMENU hFileMenu = CreatePopupMenu();
    HMENU hHelpMenu = CreatePopupMenu();

    AppendMenu(hFileMenu, MF_STRING, ID_FILE_OPEN, L"&Open");
    AppendMenu(hFileMenu, MF_STRING, ID_FILE_EXIT, L"E&xit");

    AppendMenu(hHelpMenu, MF_STRING, ID_HELP_ABOUT, L"&About");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, L"&File");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");

    SetMenu(hwnd, hMenu);
}

HWND MainWindowLayout::CreateToolbar(HWND hwnd, HINSTANCE hInstance) {
    HWND hwndToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP, 0, 0, 0, 0,
        hwnd, nullptr, hInstance, nullptr);

    SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

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
    SendMessage(hwndToolbar, TB_ADDBITMAP, 0, (LPARAM)&tbab);
    SendMessage(hwndToolbar, TB_ADDBUTTONS, sizeof(tbb) / sizeof(TBBUTTON), (LPARAM)&tbb);

    // Finally, tell the toolbar to size itself based on the added buttons:
    SendMessage(hwndToolbar, TB_AUTOSIZE, 0, 0);

    return hwndToolbar;
}

HWND MainWindowLayout::CreateTreeView(HWND hwnd, HINSTANCE hInstance) {
    return CreateWindowEx(0, WC_TREEVIEW, L"Tree View",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS,
        0, 0, 0, 0,
        hwnd, (HMENU)IDC_TREEVIEW, hInstance, nullptr);
}

HWND MainWindowLayout::CreateListView(HWND hwnd, HINSTANCE hInstance) {
    HWND hwndListView = CreateWindowEx(0, WC_LISTVIEW, L"List View",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_EDITLABELS,
        0, 0, 0, 0,
        hwnd, (HMENU)IDC_LISTVIEW, hInstance, nullptr);

    LVCOLUMN lvc;
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.cx = 100;

    lvc.pszText = const_cast<LPWSTR>(L"File Name");
    ListView_InsertColumn(hwndListView, 0, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"File Path");
    ListView_InsertColumn(hwndListView, 1, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"File Size");
    ListView_InsertColumn(hwndListView, 2, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"Date Created");
    ListView_InsertColumn(hwndListView, 3, &lvc);

    lvc.pszText = const_cast<LPWSTR>(L"Date Modified");
    ListView_InsertColumn(hwndListView, 4, &lvc);

    return hwndListView;
}

LRESULT MainWindowLayout::HandleResize(HWND hwnd, HWND hwndToolbar, HWND hwndTreeView, HWND hwndListView) {
    if (hwndToolbar && hwndTreeView && hwndListView) {
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        RECT rcToolbar;
        GetWindowRect(hwndToolbar, &rcToolbar);
        int toolbarHeight = rcToolbar.bottom - rcToolbar.top;

        MoveWindow(hwndToolbar, 0, 0, rcClient.right, toolbarHeight, TRUE);
        MoveWindow(hwndTreeView, 0, toolbarHeight, rcClient.right / 3, rcClient.bottom - toolbarHeight, TRUE);
        MoveWindow(hwndListView, rcClient.right / 3, toolbarHeight, rcClient.right - rcClient.right / 3, rcClient.bottom - toolbarHeight, TRUE);
    }
    return 0;
}