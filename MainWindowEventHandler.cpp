#include "MainWindowEventHandler.h"
#include "MainWindowUtilities.h"
#include <CommCtrl.h>
#include <shlobj.h>
#include <fstream>
#include "resource.h"

LRESULT MainWindowEventHandler::HandleCommand(HWND hwnd, WPARAM wParam, LPARAM lParam, HWND hwndTreeView, HWND hwndListView, std::unique_ptr<ISO>& iso) {
    switch (LOWORD(wParam)) {
    case ID_FILE_OPEN: {
        OPENFILENAME ofn;
        wchar_t szFile[260];

        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
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
            MainWindowUtilities::LoadIsoAndDisplayTree(hwnd, hwndTreeView, hwndListView, iso, szFile);
        }
        break;
    }
    case ID_FILE_EXIT:
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        break;
    case ID_HELP_ABOUT:
        MessageBox(hwnd, L"Dark Cloud File Manager\nVersion 1.0", L"About", MB_OK);
        break;
    default:
        return DefWindowProc(hwnd, WM_COMMAND, wParam, lParam);
    }
    return 0;
}

LRESULT MainWindowEventHandler::HandleNotify(HWND hwnd, WPARAM wParam, LPARAM lParam, HWND hwndTreeView, HWND hwndListView, const std::unique_ptr<ISO>& iso) {
    LPNMHDR lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
    if (lpnmhdr->hwndFrom == hwndTreeView && lpnmhdr->code == TVN_SELCHANGED) {
        MainWindowUtilities::OnTreeViewItemSelectionChanged(hwndTreeView, hwndListView, iso);
    }
    return 0;
}