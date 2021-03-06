#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "comctl32")
#pragma comment(lib, "d2d1")
#pragma comment(lib, "dwrite")

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <list>
#include <vector>
#include <map>
#include <string>
#include "util.h"
#include <d2d1_3.h>
#include <dwrite.h>
#include <wincodec.h>
#include "graphic.h"
#include "object.h"
#include "node.h"
#include "common.h"
#include "connectpoint.h"
#include "objectlist.h"
#include "arrow.h"
#include "app.h"
#include "node_output.h"
#include "node_start.h"
#include "node_end.h"
#include "node_timer.h"
#include "node_if.h"
#include "node_sound.h"
#include "node_beep.h"
#include "variable.h"
#include "resource.h"

#if _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#define SCALEX(X) MulDiv(X, uDpiX, DEFAULT_DPI)
#define SCALEY(Y) MulDiv(Y, uDpiY, DEFAULT_DPI)
#define FONT_SIZE 14

WCHAR szClassName[] = L"nocode";
HHOOK g_hHook;
common g_c;

LRESULT CALLBACK RichEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CHAR:
		if (wParam == '[')
		{
			int nTextHeight = 0;
			{
				HDC hdc = GetDC(hWnd);
				SIZE size = {};
				HFONT hOldFont = (HFONT)SelectObject(hdc, g_c.hUIFont);
				GetTextExtentPoint32(hdc, L"[", 1, &size);
				SelectObject(hdc, hOldFont);
				nTextHeight = size.cy;
				ReleaseDC(hWnd, hdc);
			}
			if (nTextHeight)
			{
				POINT point;
				GetCaretPos(&point);
				ClientToScreen(hWnd, &point);
				SetWindowLongPtr(g_c.hList, GWLP_USERDATA, (LONG_PTR)hWnd);
				{
					const int nItemHeight = (int)SendMessage(g_c.hList, LB_GETITEMHEIGHT, 0, 0);
					const int nItemCount = min((int)SendMessage(g_c.hList, LB_GETCOUNT, 0, 0), 16);
					SetWindowPos(g_c.hList, 0, point.x, point.y + nTextHeight, POINT2PIXEL(256), nItemHeight * nItemCount + GetSystemMetrics(SM_CYEDGE) * 2, SWP_NOZORDER | SWP_NOACTIVATE);
				}
				SendMessage(g_c.hList, LB_SETCURSEL, 0, 0);
				ShowWindow(g_c.hList, SW_SHOW);
			}
		}
		else {
			ShowWindow(g_c.hList, SW_HIDE);
		}
		{
			const LONG_PTR ret = CallWindowProc(util::DefaultRichEditProc, hWnd, msg, wParam, lParam);
			HWND hParent = GetParent(hWnd);
			if (hParent) {
				int id = GetWindowLong(hWnd, GWL_ID);
				SendMessage(hParent, WM_APP, MAKEWPARAM(id, EN_UPDATE), (LPARAM)hWnd);
			}
			return ret;
		}
		break;
	case WM_KILLFOCUS:
		if ((HWND)wParam != g_c.hList) {
			ShowWindow(g_c.hList, SW_HIDE);
		}
		break;
	case WM_MOUSEWHEEL:
		if (GET_KEYSTATE_WPARAM(wParam) & MK_CONTROL) {
			return 0;
		}
		break;
	case WM_PAINT:
		{
			HideCaret(hWnd);
			CallWindowProc(util::DefaultRichEditProc, hWnd, msg, wParam, lParam);
			RECT rect;
			SendMessage(hWnd, EM_GETRECT, 0, (LPARAM)&rect);
			LONG_PTR eax = SendMessage(hWnd, EM_CHARFROMPOS, 0, (LPARAM)&rect);
			eax = SendMessage(hWnd, EM_LINEFROMCHAR, eax, 0);
			TEXTRANGE txtrange = {};
			txtrange.chrg.cpMin = (LONG)SendMessage(hWnd, EM_LINEINDEX, eax, 0);
			txtrange.chrg.cpMax = (LONG)SendMessage(hWnd, EM_CHARFROMPOS, 0, (LPARAM)&rect.right);
			LONG_PTR FirstChar = txtrange.chrg.cpMin;
			HRGN hRgn = CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
			HDC hdc = GetDC(hWnd);
			SetBkMode(hdc, TRANSPARENT);
			HRGN hOldRgn = (HRGN)SelectObject(hdc, hRgn);
			DWORD dwTextSize = txtrange.chrg.cpMax - txtrange.chrg.cpMin;
			if (dwTextSize > 0) {
				LPWSTR pBuffer = (LPWSTR)GlobalAlloc(0, (dwTextSize + 1) * sizeof(WCHAR));
				if (pBuffer) {
					txtrange.lpstrText = pBuffer;
					if (SendMessage(hWnd, EM_GETTEXTRANGE, 0, (LPARAM)&txtrange) > 0) {
						LPWSTR lpszStart = wcsstr(txtrange.lpstrText, L"[");
						while (lpszStart) {
							while (*(lpszStart + 1) == L'[') {
								lpszStart++;
							}
							LPWSTR lpszEnd = wcsstr(lpszStart, L"]");
							if (lpszEnd) {
								const int nTextLen = (int)(lpszEnd - lpszStart);
								LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, nTextLen * sizeof(WCHAR));
								if (lpszText) {
									(void)lstrcpyn(lpszText, lpszStart + 1, nTextLen);
									if (g_c.app && ((app*)g_c.app)->IsHighlightText(lpszText)) {
										SendMessage(hWnd, EM_POSFROMCHAR, (WPARAM)&rect, lpszStart - txtrange.lpstrText + FirstChar);
										COLORREF colOld = SetTextColor(hdc, RGB(128, 128, 255));
										HFONT hOldFont = (HFONT)SelectObject(hdc, g_c.hUIFont);
										DrawText(hdc, lpszStart, nTextLen + 1, &rect, 0);
										SelectObject(hdc, hOldFont);
									}
									GlobalFree(lpszText);
								}
							}
							else {
								break;
							}
							lpszStart = wcsstr(lpszEnd, L"[");
						}
					}
					GlobalFree(pBuffer);
				}
			}
			SelectObject(hdc, hOldRgn);
			DeleteObject(hRgn);
			ReleaseDC(hWnd, hdc);
			ShowCaret(hWnd);
			return 0;
		}
		break;
	default:
		break;
	}
	return CallWindowProc(util::DefaultRichEditProc, hWnd, msg, wParam, lParam);
}

WNDPROC DefaultListWndProc;
LRESULT CALLBACK ListProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_LBUTTONUP:
		{
			CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
			POINT point;
			GetCursorPos(&point);
			if (WindowFromPoint(point) == hWnd) {
				SendMessage(hWnd, WM_CHAR, VK_RETURN, 0);
			}
		}
		return 0;
	case WM_CHAR:
		if (wParam == VK_RETURN) {
			const int nIndex = (int)SendMessage(hWnd, LB_GETCURSEL, 0, 0);
			if (nIndex != LB_ERR) {
				const int nTextLength = (int)SendMessage(hWnd, LB_GETTEXTLEN, nIndex, 0);
				if (nTextLength != LB_ERR) {
					LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, (nTextLength + 1) * sizeof(WCHAR));
					SendMessage(hWnd, LB_GETTEXT, nIndex, (LPARAM)lpszText);
					HWND hEdit = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);
					if (hEdit) {
						SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)lpszText);
						SendMessage(hEdit, EM_REPLACESEL, TRUE, (LPARAM)L"]");
					}
					GlobalFree(lpszText);
				}
			}
			ShowWindow(hWnd, SW_HIDE);
		}
		else if (wParam == VK_ESCAPE) {
			ShowWindow(hWnd, SW_HIDE);
		}
		else {
			return CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
		}
		break;
	case WM_KILLFOCUS:
		ShowWindow(hWnd, SW_HIDE);
		{
			HWND hEdit = (HWND)GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (hEdit && IsWindow(hEdit) && IsWindowVisible(hEdit)) {
				SetFocus(hEdit);
			}
		}
		break;
	default:
		break;
	}
	return CallWindowProc(DefaultListWndProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	switch (nCode) {
		case HCBT_ACTIVATE:
		{
			UnhookWindowsHookEx(g_hHook);
			HWND hMes = (HWND)wParam;
			HWND hWnd = GetParent(hMes);
			RECT rectMessageBox, rectParentWindow;
			GetWindowRect(hMes, &rectMessageBox);
			GetWindowRect(hWnd, &rectParentWindow);
			RECT rect = {
				(rectParentWindow.right + rectParentWindow.left - rectMessageBox.right + rectMessageBox.left) / 2,
				(rectParentWindow.bottom + rectParentWindow.top - rectMessageBox.bottom + rectMessageBox.top) / 2,
				(rectParentWindow.right + rectParentWindow.left - rectMessageBox.right + rectMessageBox.left) / 2 + rectMessageBox.right - rectMessageBox.left,
				(rectParentWindow.bottom + rectParentWindow.top - rectMessageBox.bottom + rectMessageBox.top) / 2 + rectMessageBox.bottom - rectMessageBox.top
			};
			util::ClipOrCenterRectToMonitor(&rect, MONITOR_CLIP | MONITOR_WORKAREA);
			SetWindowPos(hMes, hWnd, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
		break;
	}
	return 0;
}

INT_PTR CALLBACK PropContainerDialogProc(HWND hDlg, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_SIZE:
	{
		HWND hWnd = GetTopWindow(hDlg);
		if (hWnd) {
			MoveWindow(hWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
	}
	break;
	}
	return FALSE;
}

INT_PTR CALLBACK NodeBoxDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static app* pNoCodeApp;
	static HWND hTree;
	static int flag;
	static HIMAGELIST hImageList;
	static HWND hEdit;
	switch (msg)
	{
	case WM_INITDIALOG:
		pNoCodeApp = (app*)lParam;
		{
			hEdit = CreateWindow(L"EDIT", 0, WS_VISIBLE | WS_CHILD, 0, 0, 0, 0, hWnd, 0, GetModuleHandle(0), 0);
			SendMessage(hEdit, EM_LIMITTEXT, 0, 0);

			hTree = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, 0, WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_LINESATROOT, 0, 0, 0, 0, hWnd, 0, GetModuleHandle(0), 0);

			hImageList = ImageList_Create(32, 32, ILC_COLOR16, 4, 10);
			HBITMAP hBitMap = LoadBitmap(GetModuleHandle(0), MAKEINTRESOURCE(IDB_BITMAP2));
			ImageList_Add(hImageList, hBitMap, NULL);
			DeleteObject(hBitMap);
			TreeView_SetImageList(hTree, hImageList, TVSIL_NORMAL); 

			HTREEITEM hRootItem = 0;
			{
				SetFocus(hTree);
				TV_INSERTSTRUCT tv = { 0 };
				tv.hParent = TVI_ROOT;
				tv.hInsertAfter = TVI_LAST;
				tv.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_EXPANDEDIMAGE;
				tv.item.pszText = TEXT("Root");
				tv.item.iImage = 0;
				tv.itemex.iExpandedImage = 1;
				hRootItem = TreeView_InsertItem(hTree, &tv);
			}
			// 子ノード1
			if (hRootItem) {
				for (int i = 0; ; i++) {
					node* n = 0;
					switch (i) {
					case 0:
						n = new node_start(0);
						break;
					case 1:
						n = new node_end(0);
						break;
					case 2:
						n = new node_timer(0);
						break;
					case 3:
						n = new node_if(0);
						break;
					case 4:
						n = new node_sound(0);
						break;
					case 5:
						n = new node_output(0);
						break;
					case 6:
						n = new node_beep(0);
						break;
					}
					if (n) {
						TV_INSERTSTRUCT tv = { 0 };
						tv.hParent = hRootItem;
						tv.hInsertAfter = TVI_LAST;
						tv.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
						tv.item.pszText = n->name;
						tv.item.iImage = 2;
						tv.item.iSelectedImage = 3;
						tv.item.lParam = (LPARAM)n;
						HTREEITEM hItem = TreeView_InsertItem(hTree, &tv);
					}
					else {
						break;
					}
				}
				TreeView_Expand(hTree, hRootItem, TVE_EXPAND);
			}
		}
		EnumChildWindows(hWnd, util::EnumChildSetFontProc, 0);
		return TRUE;
	case WM_NOTIFY:
		{
			LPNMHDR lpNmhdr = (LPNMHDR)lParam;
			if (lpNmhdr->code == TVN_BEGINDRAG) {
				NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)lParam;
				HTREEITEM item = pnmtv->itemNew.hItem;
				TreeView_SelectItem(hTree, item);
				if (pNoCodeApp) {
					node* n = (node*)pnmtv->itemNew.lParam;
					if (n) {
						node* nn = n->copy(0);
						nn->setselect(true, 0);
						pNoCodeApp->OnBeginDrag(nn);
						flag = 1;
						SetCapture(hWnd);
					}
				}
			} else if (lpNmhdr->code == TVN_DELETEITEM) {
				LPTVITEM lp = &((LPNMTREEVIEW)lParam)->itemOld;
				node* n = (node*)lp->lParam;
				delete n;
			}
		}
		break;
	case WM_MOUSEMOVE:
		if (flag) {
			if (pNoCodeApp) {
				POINT point;
				GetCursorPos(&point);
				ScreenToClient(g_c.hMainWnd, &point);
				pNoCodeApp->OnDragging(point.x, point.y);
			}
		}
		break;
	case WM_LBUTTONUP:
		if (flag == 1) {
			ReleaseCapture();
			flag = 0;
			if (pNoCodeApp) {
				POINT point;
				GetCursorPos(&point);
				if (WindowFromPoint(point) == g_c.hMainWnd) {
					pNoCodeApp->OnDropped();
				}
				else {
					pNoCodeApp->OnCancelDrag();
				}
			}
		}
		break;
	case WM_SIZE:
		if (hTree) {
			MoveWindow(hTree, POINT2PIXEL(4), POINT2PIXEL(4), LOWORD(lParam) - POINT2PIXEL(8), HIWORD(lParam) - POINT2PIXEL(8), TRUE);
		}
		break;
	case WM_DESTROY:
		if (hTree) {
			TreeView_DeleteAllItems(hTree);
		}
		ImageList_Destroy(hImageList);
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK OutputDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static app* pNoCodeApp;
	static HWND hEdit;
	switch (msg)
	{
	case WM_INITDIALOG:
		pNoCodeApp = (app*)lParam;
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 0, 0, 0, 0, hWnd, 0, GetModuleHandle(0), 0);
		SendMessage(hEdit, EM_LIMITTEXT, 0, 0);
		EnumChildWindows(hWnd, util::EnumChildSetFontProc, 0);
		return TRUE;
	case WM_SIZE:
		MoveWindow(hEdit, POINT2PIXEL(4), POINT2PIXEL(4), LOWORD(lParam) - POINT2PIXEL(8), HIWORD(lParam) - POINT2PIXEL(8), TRUE);
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK AboutDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK SettingsDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK CreateVariableDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static variable* v;
	switch (msg)
	{
	case WM_INITDIALOG:
		v = (variable*)lParam;

		{
			RECT rect;
			GetWindowRect(GetParent(hWnd), &rect);
			SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
		}

		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");
		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_ADDSTRING, 0, (LPARAM)L"値が設定されていない");

		SendDlgItemMessage(hWnd, IDC_COMBO1, CB_SETCURSEL, 0, 0);

		return TRUE;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			DWORD dwSize = GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT1));
			LPWSTR lpszName = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * (dwSize + 1));
			GetDlgItemText(hWnd, IDC_EDIT1, lpszName, dwSize + 1);
			v->name = lpszName;
			GlobalFree(lpszName);

			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hWnd, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK VariableListDialogProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
	static app* pNoCodeApp;
	static HWND hButtonCreate;
	static HWND hButtonDelete;
	static HWND hList;
	static HWND hEdit;
	static int m_nRow;
	static int m_nCol;

	switch (msg)
	{
	case WM_INITDIALOG:
		pNoCodeApp = (app*)lParam;
		if (pNoCodeApp) {
			hButtonCreate = CreateWindow(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)1000, GetModuleHandle(0), NULL);
			hButtonDelete = CreateWindow(L"BUTTON", L"-", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hWnd, (HMENU)1001, GetModuleHandle(0), NULL);
			hList = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, 0, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_EDITLABELS, 0, 0, 0, 0, hWnd, (HMENU)1002, GetModuleHandle(0), NULL);
			if (hList) {
				LV_COLUMN lvcol = {};
				lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
				lvcol.fmt = LVCFMT_LEFT;
				lvcol.cx = 128;
				lvcol.pszText = TEXT("変数名");
				lvcol.iSubItem = 0;
				SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&lvcol);
				lvcol.fmt = LVCFMT_LEFT;
				lvcol.cx = 128;
				lvcol.pszText = TEXT("値");
				lvcol.iSubItem = 1;
				SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&lvcol);
				SendMessage(hWnd, WM_APP, 0, 0);
				ListView_SetExtendedListViewStyle(hList, LVS_EX_TWOCLICKACTIVATE);
			}
		}
		EnumChildWindows(hWnd, util::EnumChildSetFontProc, 0);
		return TRUE;
	case WM_NOTIFY:
		if (LOWORD(wParam) == 1002)
		{
			switch (((LV_DISPINFO*)lParam)->hdr.code)
			{
			case NM_CLICK:
				{
					auto lpnmia = (LPNMITEMACTIVATE)lParam;
					m_nRow = lpnmia->iItem;
					m_nCol = lpnmia->iSubItem;
				}
				break;
			case NM_DBLCLK:
				{
					//ListView_EditLabel(hList, m_nRow);
					ListView_EditLabel(hList, ListView_GetNextItem(hList, -1, LVNI_FOCUSED));
				}
				break;
			case LVN_KEYDOWN:
				if (((NMLVKEYDOWN*)lParam)->wVKey == VK_F2)
				{
					ListView_EditLabel(hList, ListView_GetNextItem(hList, -1, LVNI_FOCUSED));
				}
				break;
			//case LVN_ITEMACTIVATE:
			//	{
			//		auto lpnmia = (LPNMITEMACTIVATE)lParam;


			//		RECT rect;
			//		if (ListView_GetSubItemRect(hList, m_nRow, m_nCol, LVIR_LABEL, &rect))
			//		{
			//			hEdit = ListView_GetEditControl(hList);
			//			MoveWindow(hEdit, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
			//		}
			//	}
			//	ListView_EditLabel(hList, ListView_GetNextItem(hList, -1, LVNI_FOCUSED));
			//	break;
			case LVN_BEGINLABELEDIT:    //アイテム編集を開始
				{
					hEdit = ListView_GetEditControl(hList);

					RECT rect;
					if (ListView_GetSubItemRect(hList, m_nRow, m_nCol, LVIR_LABEL, &rect))
					{
						MoveWindow(hEdit, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);
					}
				}
				break;
			case LVN_ENDLABELEDIT:
				{
					int nTextLength = GetWindowTextLength(hEdit);
					if (nTextLength) {
						LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * (nTextLength + 1));
						GetWindowText(hEdit, lpszText, nTextLength + 1);
						ListView_SetItemText(hList, ((LV_DISPINFO*)lParam)->item.iItem, 0, lpszText);
						GlobalFree(lpszText);
					}
				}
				break;
			}
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == 1000) {
			variable* v = new variable(0);
			if (DialogBoxParam(GetModuleHandle(0), MAKEINTRESOURCE(IDD_CREATE_VARIABLE), hWnd, CreateVariableDialogProc, (LPARAM)v) == IDOK) {
				pNoCodeApp->OnCreateVariable(v);
			}
			else {
				delete v;
			}
		}
		break;
	case WM_APP://updata
		{
			SendMessage(hList, LVM_DELETEALLITEMS, 0, 0);
			LV_ITEM item = {};
			item.mask = LVIF_TEXT;
			int index = 0;
			for (auto i : pNoCodeApp->nl.l) {
				if (!i->isalive(pNoCodeApp->generation)) continue;
				if (i->getobjectkind() != OBJECT_VARIABLE) continue;
				auto v = (variable*)i;
				item.pszText = (LPWSTR)v->name.c_str();
				item.iItem = index;
				item.iSubItem = 0;
				SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&item);
				item.pszText = v->displayValue();
				item.iItem = index;
				item.iSubItem = 1;
				SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&item);
				index++;
			}
		}
		break;
	case WM_SIZE:
		MoveWindow(hButtonCreate, POINT2PIXEL(2), POINT2PIXEL(2), POINT2PIXEL(50), POINT2PIXEL(20), TRUE);
		MoveWindow(hButtonDelete, POINT2PIXEL(54), POINT2PIXEL(2), POINT2PIXEL(50), POINT2PIXEL(20), TRUE);
		MoveWindow(hList, POINT2PIXEL(2), POINT2PIXEL(24), LOWORD(lParam) - POINT2PIXEL(4), HIWORD(lParam) - POINT2PIXEL(26), TRUE);
		break;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static app* pNoCodeApp;
	switch (msg)
	{
	case WM_CREATE:
		InitCommonControls();
		LoadLibrary(L"riched20");

		pNoCodeApp = new app();
		g_c.app = (void*)pNoCodeApp;

		SendMessage(hWnd, WM_APP, 0, 0);

		g_c.hList = CreateWindowEx(WS_EX_TOPMOST | WS_EX_CLIENTEDGE | WS_EX_NOACTIVATE, L"LISTBOX", NULL, WS_POPUP | WS_VSCROLL, 0, 0, 0, 0, 0, 0, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		SendMessage(g_c.hList, WM_SETFONT, (WPARAM)g_c.hUIFont, 0);
		DefaultListWndProc = (WNDPROC)SetWindowLongPtr(g_c.hList, GWLP_WNDPROC, (LONG_PTR)ListProc1);
		{
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"abc");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"def");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"ghi");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"jkl");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"mno");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"pqr");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"stu");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"vwx");
			SendMessage(g_c.hList, LB_ADDSTRING, 0, (LPARAM)L"yz");
		}

		g_c.hNodeBox = CreateDialogParam(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_NODEBOX), hWnd, NodeBoxDialogProc, (LPARAM)pNoCodeApp);
		g_c.hOutput = CreateDialogParam(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_OUTPUT), hWnd, OutputDialogProc, (LPARAM)pNoCodeApp);
		g_c.hVariableList = CreateDialogParam(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_VARIABLE_LIST), hWnd, VariableListDialogProc, (LPARAM)pNoCodeApp);

		{
			TBBUTTON tbb[] = {
				{0,ID_NEW,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{1,ID_OPEN,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{2,ID_SAVE,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{3,ID_EXIT,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{4,ID_UNDO,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{5,ID_REDO,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{6,ID_COPY,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{7,ID_CUT,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{8,ID_PASTE,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{9,ID_DELETE,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{10,ID_RUN,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{11,ID_SUSPEND,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{12,ID_STOP,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{13,ID_ZOOM,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{14,ID_SHRINK,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{15,ID_HOME,TBSTATE_ENABLED,TBSTYLE_BUTTON},
				{ 0 , 0 , TBSTATE_ENABLED , TBSTYLE_SEP , 0 , 0 , 0 } ,
				{16,ID_HELP,TBSTATE_ENABLED,TBSTYLE_BUTTON},
			};
			g_c.hTool = CreateToolbarEx(hWnd, WS_CHILD | WS_VISIBLE | WS_BORDER | TBSTYLE_FLAT, 0, _countof(tbb), ((LPCREATESTRUCT)lParam)->hInstance, IDR_TOOLBAR1, tbb, _countof(tbb), 0, 0, 64, 64, sizeof(TBBUTTON));
			//LONG_PTR lStyle = GetWindowLongPtr(g_c.hTool, GWL_STYLE);
			//lStyle = (lStyle | TBSTYLE_FLAT) & ~TBSTYLE_TRANSPARENT;
			//SetWindowLongPtr(g_c.hTool, GWL_STYLE, lStyle);
		}

		g_c.hPropContainer = CreateDialog(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDD_PROPERTY), hWnd, PropContainerDialogProc);

		{
			// メニューを設定する
			HMENU hMenu = GetMenu(hWnd);
			if (hMenu) {
				if (g_c.bShowGrid) {
					CheckMenuItem(hMenu, ID_VIEW_GRID, MF_BYCOMMAND | MFS_CHECKED);
				}
			}
		}

		if (pNoCodeApp) {
			pNoCodeApp->OnCreate(hWnd); // ツールバーなどが作成された後に呼び出す
		}

		break;
	case WM_APP: // 画面DPIが変わった時、ウィンドウ作成時にフォントの生成を行う
		util::GetScaling(hWnd, &g_c.uDpiX, &g_c.uDpiY);
		DeleteObject(g_c.hUIFont);
		g_c.hUIFont = CreateFontW(-POINT2PIXEL(FONT_SIZE), 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, FONT_NAME);
		break;
	case  WM_APP + 1:
		if (pNoCodeApp) {
			pNoCodeApp->OnEnd();
		}
		break;
	case WM_RBUTTONDOWN:
		if (pNoCodeApp) {
			pNoCodeApp->OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_RBUTTONUP:
		if (pNoCodeApp) {
			pNoCodeApp->OnRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_LBUTTONDOWN:
		if (pNoCodeApp) {
			pNoCodeApp->OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_LBUTTONUP:
		if (pNoCodeApp) {
			pNoCodeApp->OnLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MBUTTONDOWN:
		if (pNoCodeApp) {
			pNoCodeApp->OnMButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MBUTTONUP:
		if (pNoCodeApp) {
			pNoCodeApp->OnMButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MOUSEMOVE:
		if (pNoCodeApp) {
			pNoCodeApp->OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		}
		break;
	case WM_MOUSEWHEEL:
		if (pNoCodeApp) {
			pNoCodeApp->OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		}
		break;
	case WM_PAINT:
		if (pNoCodeApp) {
			pNoCodeApp->OnPaint();
		}
		break;
	case WM_SIZE:
		{
			SendMessage(g_c.hTool, TB_AUTOSIZE, 0, 0);
			RECT rect;
			GetWindowRect(g_c.hTool, &rect);
			MoveWindow(g_c.hNodeBox, 0, rect.bottom - rect.top, POINT2PIXEL(256), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			MoveWindow(g_c.hOutput, POINT2PIXEL(256), HIWORD(lParam) - POINT2PIXEL(96), LOWORD(lParam) - POINT2PIXEL(256) - POINT2PIXEL(256) - POINT2PIXEL(200), POINT2PIXEL(96), 1);
			MoveWindow(g_c.hPropContainer, LOWORD(lParam) - POINT2PIXEL(256), rect.bottom - rect.top, POINT2PIXEL(256), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			MoveWindow(g_c.hVariableList, LOWORD(lParam) - POINT2PIXEL(256)- POINT2PIXEL(200), rect.bottom - rect.top, POINT2PIXEL(200), HIWORD(lParam) - (rect.bottom - rect.top), 1);
			if (pNoCodeApp) {
				pNoCodeApp->OnSize(POINT2PIXEL(256), rect.bottom - rect.top, LOWORD(lParam) - POINT2PIXEL(256) - POINT2PIXEL(256) - POINT2PIXEL(200), HIWORD(lParam) - (rect.bottom - rect.top) - POINT2PIXEL(96));
			}
		}
		break;
	case WM_DISPLAYCHANGE:
		if (pNoCodeApp) {
			pNoCodeApp->OnDisplayChange();
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_NEW:
			if (pNoCodeApp) {
				pNoCodeApp->OnNew();
			}
			break;
		case ID_ALLSELECT:
			if (pNoCodeApp) {
				pNoCodeApp->OnAllselect();
			}
			break;
		case ID_UNSELECT:
			if (pNoCodeApp) {
				pNoCodeApp->OnUnselect();
			}
			break;
		case ID_DELETE:
			if (pNoCodeApp) {
				pNoCodeApp->OnDelete();
			}
			break;
		case ID_HOME:
			if (pNoCodeApp) {
				pNoCodeApp->OnHome();
			}
			break;
		case ID_EXIT:
			PostMessage(hWnd, WM_CLOSE, 0, 0);
			break;
		case ID_ZOOM:
			if (pNoCodeApp) {
				pNoCodeApp->OnZoom();
			}
			break;
		case ID_SHRINK:
			if (pNoCodeApp) {
				pNoCodeApp->OnShrink();
			}
			break;
		case ID_UNDO:
			if (pNoCodeApp) {
				pNoCodeApp->OnUndo();
			}
			break;
		case ID_REDO:
			if (pNoCodeApp) {
				pNoCodeApp->OnRedo();
			}
			break;
		case ID_PROPERTY:
			if (pNoCodeApp) {
				pNoCodeApp->OnProperty();
			}
			break;
		case ID_RUN:
			if (pNoCodeApp) {
				pNoCodeApp->OnRun();
			}
			break;
		case ID_SUSPEND:
			if (pNoCodeApp) {
				pNoCodeApp->OnSuspend();
			}
			break;
		case ID_STOP:
			if (pNoCodeApp) {
				pNoCodeApp->OnStop();
			}
			break;
		case ID_OUTPUT_CLEAR:
			if (g_c.hOutput) {
				HWND hEdit = GetTopWindow(g_c.hOutput);
				if (hEdit) {
					SetWindowText(hEdit, 0);
				}
			}
			break;
		case ID_ABOUT:
			DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_ABOUT), hWnd, AboutDialogProc);
			break;
		case ID_SETTINGS:
			DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_SETTINGS), hWnd, SettingsDialogProc);
			break;
		case ID_FEEDBACK:
			ShellExecute(NULL, L"open", L"https://forms.gle/vAFimNY6nWnx6DCDA", NULL, NULL, SW_SHOWNORMAL);
			break;
		case ID_OPEN:
			if (pNoCodeApp) {
				pNoCodeApp->OnOpen();
			}
			break;
		case ID_SAVE:
			if (pNoCodeApp) {
				pNoCodeApp->OnSave();
			}
			break;
		case ID_VIEW_GRID:
			{
				HMENU hMenu = GetMenu(hWnd);
				if (hMenu) {
					g_c.bShowGrid = !g_c.bShowGrid;
					if (g_c.bShowGrid) {
						CheckMenuItem(hMenu, ID_VIEW_GRID, MF_BYCOMMAND | MFS_CHECKED);
					}
					else {
						CheckMenuItem(hMenu, ID_VIEW_GRID, MF_BYCOMMAND | MFS_UNCHECKED);
					}
					if (pNoCodeApp) {
						pNoCodeApp->OnViewGrid(g_c.bShowGrid);
					}
				}
			}
			break;
		}
		break;
	case WM_CLOSE:
		g_hHook = SetWindowsHookEx(WH_CBT, CBTProc, NULL, ::GetCurrentThreadId());
		if (MessageBox(hWnd, L"終了しますか？", L"確認", MB_YESNO) == IDYES) {
			DestroyWindow(hWnd);
		}
		break;
	case WM_NCCREATE:
		{
			const HMODULE hModUser32 = GetModuleHandleW(L"user32");
			if (hModUser32)
			{
				typedef BOOL(WINAPI* fnTypeEnableNCScaling)(HWND);
				const fnTypeEnableNCScaling fnEnableNCScaling = (fnTypeEnableNCScaling)GetProcAddress(hModUser32, "EnableNonClientDpiScaling");
				if (fnEnableNCScaling)
				{
					fnEnableNCScaling(hWnd);
				}
			}
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	case WM_DPICHANGED:
		SendMessage(hWnd, WM_APP, 0, 0);
		break;
	case WM_DESTROY:
		if (pNoCodeApp) {
			pNoCodeApp->OnDestroy();
		}
		DeleteObject(g_c.hUIFont);
		delete pNoCodeApp;
		pNoCodeApp = nullptr;
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
#if _DEBUG
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
#endif
	HeapSetInformation(0, HeapEnableTerminationOnCorruption, 0, 0);
	if (FAILED(CoInitialize(0))) return 0;
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		LoadIcon(hInstance,(LPCTSTR)IDI_ICON1),
		LoadCursor(0,IDC_ARROW),
		0,
		MAKEINTRESOURCE(IDR_MAIN),
		szClassName
	};
	RegisterClass(&wndclass);
	g_c.hMainWnd = CreateWindowW(
		szClassName,
		L"nocode",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(g_c.hMainWnd, SW_SHOWDEFAULT);
	UpdateWindow(g_c.hMainWnd);
	ACCEL Accel[] = {
		{FVIRTKEY | FCONTROL, 'A', ID_ALLSELECT},
		{FVIRTKEY, VK_ESCAPE, ID_UNSELECT},
		{FVIRTKEY, VK_DELETE, ID_DELETE},
		{FVIRTKEY, VK_HOME, ID_HOME},
		{FVIRTKEY | FCONTROL, 'W', ID_EXIT},
		{FVIRTKEY | FCONTROL, VK_OEM_PLUS, ID_ZOOM},
		{FVIRTKEY | FCONTROL, VK_OEM_MINUS, ID_SHRINK},
		{FVIRTKEY | FCONTROL, VK_ADD, ID_ZOOM},
		{FVIRTKEY | FCONTROL, VK_SUBTRACT, ID_SHRINK},
		{FVIRTKEY | FCONTROL, 'Z', ID_UNDO},
		{FVIRTKEY | FCONTROL, 'Y', ID_REDO},
		{FVIRTKEY | FALT, VK_RETURN, ID_PROPERTY},
		{FVIRTKEY, VK_F5, ID_RUN},
	};
	HACCEL hAccel = CreateAcceleratorTable(Accel, _countof(Accel));
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!((GetFocus() == g_c.hOutput || IsChild(g_c.hOutput, msg.hwnd)) && IsDialogMessage(g_c.hOutput, &msg)) &&
			!((GetFocus() == g_c.hNodeBox || IsChild(g_c.hNodeBox, msg.hwnd)) && IsDialogMessage(g_c.hNodeBox, &msg))&&
			!((GetFocus() == g_c.hPropContainer || IsChild(g_c.hPropContainer, msg.hwnd)) && IsDialogMessage(g_c.hPropContainer, &msg))&&
			!((GetFocus() == g_c.hVariableList || IsChild(g_c.hVariableList, msg.hwnd)) && IsDialogMessage(g_c.hVariableList, &msg))&&
			!TranslateAccelerator(g_c.hMainWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	CoUninitialize();
	return (int)msg.wParam;
}
