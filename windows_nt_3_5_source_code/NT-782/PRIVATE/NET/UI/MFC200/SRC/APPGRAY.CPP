// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#include "stdafx.h"

#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Support for Gray background message boxes

#ifndef _USRDLL // can't hook message boxes in a DLL

// default Windows dialog window procedure (for message boxes as well)
static WNDPROC lpfnDlgWndProc = NULL;

// old hook handle -- global so we can unhook it in AfxWinTerm
HHOOK _afxHHookOldCbtFilter;

/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK AFX_EXPORT
_AfxGrayBackgroundWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	if (afxDlgBkBrush != NULL &&
		(nMsg == WM_CTLCOLORBTN || nMsg == WM_CTLCOLORDLG ||
		 nMsg == WM_CTLCOLORSTATIC || nMsg == WM_CTLCOLORSCROLLBAR ||
		 nMsg == WM_CTLCOLORLISTBOX) &&
		CWnd::GrayCtlColor((HDC)wParam, (HWND)lParam,
			(UINT)(nMsg - WM_CTLCOLORMSGBOX), afxDlgBkBrush, afxDlgTextClr))
	{
		return (LRESULT)afxDlgBkBrush;
	}
#ifdef STRICT
	return CallWindowProc(lpfnDlgWndProc, hWnd, nMsg, wParam, lParam);
#else
	return CallWindowProc((FARPROC)lpfnDlgWndProc, hWnd, nMsg, wParam, lParam);
#endif
}


LRESULT CALLBACK AFX_EXPORT
_AfxCbtFilterHook(int code, WPARAM wParam, LPARAM lParam)
	// filter for WH_CBT - to dynamically subclass MessageBoxes
	//   and dialogs created outside of the C++ domain for grey backgrounds
{
	static HWND hWndHookParentDlg = NULL;

	if (code == HCBT_CREATEWND)
	{
		ASSERT(lParam != NULL);
		LPCREATESTRUCT lpcs = ((LPCBT_CREATEWND)lParam)->lpcs;
		ASSERT(lpcs != NULL);
		if (lpcs->lpszClass == WC_DIALOG)
		{
			hWndHookParentDlg = (HWND)wParam;       // hook it
		}
		else if (hWndHookParentDlg != NULL)
		{
			// first control in a dialog
			// dynamically subclass the parent is still a WC_DIALOG
			if ((WNDPROC)GetWindowLong(hWndHookParentDlg,
				GWL_WNDPROC) == lpfnDlgWndProc)
			{
				(WNDPROC)SetWindowLong(hWndHookParentDlg,
					GWL_WNDPROC, (DWORD)_AfxGrayBackgroundWndProc);
			}
			hWndHookParentDlg = NULL;
		}
	}

	// just call default handler and return the result
	return CallNextHookEx(_afxHHookOldCbtFilter, code, wParam, lParam);
}

#endif //!_USRDLL

/////////////////////////////////////////////////////////////////////////////

void CWinApp::SetDialogBkColor(COLORREF clrCtlBk, COLORREF clrCtlText)
{
	ASSERT((clrCtlBk & 0xFF000000L) == 0);  // must be valid RGB value
	ASSERT((clrCtlText & 0xFF000000L) == 0);    // must be valid RGB value
					// no palette indices

	// set up for grey backgrounds for dialogs
	if (afxDlgBkBrush != NULL)
	{
		::DeleteObject(afxDlgBkBrush);      // get rid of old one
		afxDlgBkBrush = NULL;
	}

	afxDlgBkBrush = ::CreateSolidBrush(clrCtlBk);
	afxDlgTextClr = clrCtlText;
	if (afxDlgBkBrush == NULL)
		AfxThrowResourceException();

#ifndef _USRDLL
	// Hook default background colors for message boxes
	if (lpfnDlgWndProc == NULL) // only set once
	{
		// get info about standard dialog wnd proc
		WNDCLASS wndcls;
		VERIFY(::GetClassInfo(NULL, WC_DIALOG, &wndcls));
		lpfnDlgWndProc = wndcls.lpfnWndProc;
	}

	// Win31 supports CBT window create hook
	if (_afxHHookOldCbtFilter == NULL)
		_afxHHookOldCbtFilter = SetWindowsHookEx(WH_CBT,
			(HOOKPROC)_AfxCbtFilterHook, _AfxGetHookHandle(),
			::GetCurrentThreadId());
#endif //!_USRDLL
}

/////////////////////////////////////////////////////////////////////////////
