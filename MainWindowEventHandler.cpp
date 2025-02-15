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
#include "Resource.h"

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_FILE_OPEN: {
            OPENFILENAME ofn;
            wchar_t szFile[260];

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwnd_;
            ofn.lpstrFile = szFile;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = L"ISO Files\0*.iso\0All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = nullptr;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = nullptr;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn) == TRUE) {
                LoadIsoAndDisplayTree(szFile);
            }
            break;
        }
        case ID_FILE_EXIT:
            PostMessage(hwnd_, WM_CLOSE, 0, 0);
            break;
        case ID_EDIT_UNDO:
            // Handle undo
            break;
        case ID_EDIT_REDO:
            // Handle redo
            break;
        case ID_EDIT_COPY:
            // Handle copy
            break;
        case ID_EDIT_PASTE:
            // Handle paste
            break;
        case ID_HELP_ABOUT:
            MessageBox(hwnd_, L"ISO Extractor\nVersion 1.0", L"About", MB_OK);
            break;
        }
        break;
    case WM_SIZE:
        if (hwndToolbar_ && hwndTreeView_ && hwndListView_) {
            RECT rcClient;
            GetClientRect(hwnd_, &rcClient);

            RECT rcToolbar;
            GetWindowRect(hwndToolbar_, &rcToolbar);
            int toolbarHeight = rcToolbar.bottom - rcToolbar.top;

            MoveWindow(hwndToolbar_, 0, 0, rcClient.right, toolbarHeight, TRUE);
            MoveWindow(hwndTreeView_, 0, toolbarHeight, rcClient.right / 3, rcClient.bottom - toolbarHeight, TRUE);
            MoveWindow(hwndListView_, rcClient.right / 3, toolbarHeight, rcClient.right - rcClient.right / 3, rcClient.bottom - toolbarHeight, TRUE);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd_, uMsg, wParam, lParam);
    }

    return 0;
}