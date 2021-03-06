/****************************** Module Header ******************************\
* Module Name: winbez.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Window Bezier Demo
*
* History:
* 05-20-91 PaulB        Created.
\***************************************************************************/

#include <windows.h>
#include <stdarg.h>
#include <commdlg.h>
#include "winbez.h"

// window globals

HANDLE   ghModule;
HWND     ghwndMain;
BOOL     bNoTitle = FALSE;

// drawing globals

HDC      ghdc;
HPEN     ghpenDraw  = (HPEN) 0;
HPEN     ghpenThin  = (HPEN) 0;
HBRUSH   ghbrushBez = (HBRUSH) 0;

LONG     glSpeed = 110;
ULONG    gfl = BEZ_XOR;
WORD     gwOldClipMode = MM_CLIP_NONE;

ULONG    giEndCap = MM_ENDCAP_ROUND;
ULONG    giStyle  = MM_STYLE_SOLID;
ULONG    giColorMode = 0;

COLORREF gcrDraw = RGB(255, 0, 0  );
COLORREF gcrClip = RGB(0,   0, 255);

LOGFONT  glf;

/*
 * Forward declarations.
 */
BOOL InitializeApp(void);
LONG MainWndProc(HWND hwnd, UINT message, DWORD wParam, LONG lParam);
LONG About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void NEAR PASCAL SetMenuBar( HWND hWnd );

/***************************************************************************\
* main
*
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

int _CRTAPI1 main(
    int argc,
    char *argv[])
{
    MSG msg;
    HANDLE haccel;

    ghModule = GetModuleHandle(NULL);

    if (!InitializeApp())
        return 0;

    haccel = LoadAccelerators(ghModule, MAKEINTRESOURCE(1));

    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, haccel, &msg)) {
             TranslateMessage(&msg);
             DispatchMessage(&msg);
        }
    }

    return 1;

    argc;
    argv;
}


/***************************************************************************\
* InitializeApp
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

BOOL InitializeApp(void)
{
    WNDCLASS wc;

    wc.style            = CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc      = MainWndProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = ghModule;
    wc.hIcon            = LoadIcon(ghModule, (LPTSTR)1);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName     = "MainMenu";
    wc.lpszClassName    = "WINBezClass";

    RegisterClass(&wc);

    ghwndMain = CreateWindowEx(0L, "WINBezClass", "Win Bez",
            WS_OVERLAPPED | WS_CAPTION | WS_BORDER | WS_THICKFRAME |
            WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN |
            WS_VISIBLE | WS_SYSMENU,
            80, 70, 400, 300,
            NULL, NULL, ghModule, NULL);

    if (ghwndMain == NULL)
        return FALSE;

    if( bNoTitle )
    {
        SetMenuBar( ghwndMain );
    }

    SetFocus(ghwndMain);    /* set initial focus */

// Set up default clip font and text.

    memset(&glf, 0, sizeof(LOGFONT));
    lstrcpy(glf.lfFaceName, "Arial");
    glf.lfHeight  = -24;
    glf.lfWeight  = 700;
    glf.lfCharSet = ANSI_CHARSET;
    glf.lfPitchAndFamily = FF_SWISS | VARIABLE_PITCH ;

    return TRUE;
}

/***************************************************************************\
* vNewPen(hdc)
*
* History:
* 02-03-92 AndrewGo      Created.
\***************************************************************************/

VOID vNewPen(HDC hdc)
{
    ULONG    iStyle;
    LOGBRUSH lb;
    HPEN     hpenOld;
    ULONG    ulStyleCount = 0;
    ULONG*   pulStyle = (LPDWORD) NULL;

    static ULONG aulOdd[] = { 40 };     // Style array for 'Odd Style'

    iStyle =  (gfl & BEZ_WIDE) ? PS_GEOMETRIC : PS_COSMETIC;

    switch(giStyle)
    {
    case MM_STYLE_SOLID:        iStyle |= PS_SOLID;      break;
    case MM_STYLE_DOT:          iStyle |= PS_DOT;        break;
    case MM_STYLE_DASH:         iStyle |= PS_DASH;       break;
    case MM_STYLE_DASH_DOT:     iStyle |= PS_DASHDOT;    break;
    case MM_STYLE_DASH_DOT_DOT: iStyle |= PS_DASHDOTDOT; break;
    case MM_STYLE_ALTERNATE:    iStyle |= PS_ALTERNATE;  break;
    case MM_STYLE_ODD:
        iStyle      |= PS_USERSTYLE;
        pulStyle     = &aulOdd[0];
        ulStyleCount = sizeof(aulOdd) / sizeof(aulOdd[0]);
        break;
    }

    if (gfl & BEZ_WIDE)
    {
        switch(giEndCap)
        {
        case MM_ENDCAP_ROUND:  iStyle |= PS_ENDCAP_ROUND;  break;
        case MM_ENDCAP_FLAT:   iStyle |= PS_ENDCAP_FLAT;   break;
        case MM_ENDCAP_SQUARE: iStyle |= PS_ENDCAP_SQUARE; break;
        }
    }

    lb.lbStyle = BS_SOLID;
    lb.lbColor = gcrDraw;
    lb.lbHatch = 0;

    hpenOld = ghpenDraw;
    ghpenDraw = ExtCreatePen(iStyle,
                             (gfl & BEZ_WIDE) ? 21 : 1,
                             &lb,
                             ulStyleCount,
                             pulStyle);

    DeleteObject(ghpenThin);
    ghpenThin = ExtCreatePen(PS_COSMETIC, 1, &lb, 0, NULL);

    SelectObject(hdc, ghpenDraw);
    DeleteObject(hpenOld);

    DeleteObject(ghbrushBez);
    ghbrushBez = CreateSolidBrush(gcrDraw);
}

/***************************************************************************\
* MainWndProc
*
* History:
* 04-07-91 DarrinM      Created.
\***************************************************************************/

long MainWndProc(
    HWND hwnd,
    UINT message,
    DWORD wParam,
    LONG lParam)
{
    HMENU       hmenu = GetMenu(hwnd);
    WORD        wCmd;
    CHOOSECOLOR cc;
    DWORD       adwCust[16] = { RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255),
                                RGB(255, 255, 255), RGB(255, 255, 255) };

    cc.lStructSize    = sizeof(CHOOSECOLOR);
    cc.hwndOwner      = hwnd;
    cc.hInstance      = ghModule;
    cc.lpCustColors   = adwCust;
    cc.Flags          = CC_RGBINIT | CC_SHOWHELP;
    cc.lCustData      = 0;
    cc.lpfnHook       = NULL;
    cc.lpTemplateName = NULL;

    switch (message)
    {
    case WM_SIZE:
        if (wParam == SIZEICONIC)
            gfl |= BEZ_ICONIC;
        else
        {
            gfl &= ~BEZ_ICONIC;
            gcxScreen = LOWORD(lParam);
            gcyScreen = HIWORD(lParam);
            vSetClipMode(giClip);
        }
        break;

    case WM_MOVE:
        vRedraw();
        break;

    case WM_TIMER:
        if (!(gfl & (BEZ_PAUSE | BEZ_ICONIC)))
            vNextBez();

        break;

    case WM_CREATE:
        vInitPoints();

        SetTimer(hwnd,1,glSpeed,NULL);

        ghdc = GetDC(hwnd);

        ghbrushClip = CreateSolidBrush(RGB(0,0,255));
        ghbrushBlob = CreateSolidBrush(RGB(0,0,255));
        ghbrushBack = GetStockObject(BLACK_BRUSH);

        vNewPen(ghdc);
        SelectObject(ghdc,ghbrushBlob);
        SetROP2(ghdc, R2_XORPEN);
        SetPolyFillMode(ghdc,WINDING);

        break;

    case WM_DESTROY:
        DeleteObject(ghbrushClip);
        DeleteObject(ghrgnClip);
        DeleteObject(ghrgnInvert);
        DeleteObject(ghpenDraw);
        DeleteObject(ghpenThin);
        DeleteObject(ghbrushBez);
        DeleteObject(ghrgnWideOld);

        PostQuitMessage(0);
        break;


    case WM_NCLBUTTONDBLCLK:
        if( !bNoTitle )
            /* if we have title bars etc. let the normal sutff take place */
            goto defproc;
        /* else: no title bars, then this is actually a request to bring
         * the title bars back...
         */
        /* fall through */

    case WM_LBUTTONDBLCLK:
toggle_title:
        bNoTitle = (bNoTitle ? FALSE : TRUE );
        SetMenuBar( hwnd );
        break;

    case WM_NCHITTEST:
        /* if we have no title/menu bar, clicking and dragging the client
         * area moves the window. To do this, return HTCAPTION.
         * Note dragging not allowed if window maximized, or if caption
         * bar is present.
         */
        wParam = DefWindowProc(hwnd, message, wParam, lParam);
        if( bNoTitle && (wParam == HTCLIENT) && !IsZoomed(hwnd) )
            return HTCAPTION;
        else
            return wParam;

    case WM_COMMAND:
        wCmd = LOWORD(wParam);
        switch (wCmd)
        {

        case MM_ABOUT:
            DialogBox(ghModule, "AboutBox", ghwndMain, About);
            break;

        case MM_PAUSE:
            gfl ^= BEZ_PAUSE;
            CheckMenuItem(hmenu, MM_PAUSE, (gfl & BEZ_PAUSE) ? MF_CHECKED : MF_UNCHECKED);
            break;

        case MM_BLOB:
            gfl ^= BEZ_BLOB;
            CheckMenuItem(hmenu, MM_BLOB, (gfl & BEZ_BLOB) ? MF_CHECKED : MF_UNCHECKED);

            gfl ^= BEZ_XOR;
            SetROP2(ghdc, (gfl & BEZ_XOR) ? R2_XORPEN : R2_COPYPEN);

            vNewPen(ghdc);
            vRedraw();
            break;



        case MM_WIDE:
            gfl ^= BEZ_WIDE;

            CheckMenuItem(hmenu, MM_WIDE, (gfl & BEZ_WIDE) ? MF_CHECKED : MF_UNCHECKED);

            gfl ^= BEZ_XOR;
            SetROP2(ghdc, (gfl & BEZ_XOR) ? R2_XORPEN : R2_COPYPEN);

            if (gfl & BEZ_WIDE)
            {
                EnableMenuItem(hmenu, MM_ENDCAP_ROUND,  MF_ENABLED);
                EnableMenuItem(hmenu, MM_ENDCAP_FLAT,   MF_ENABLED);
                EnableMenuItem(hmenu, MM_ENDCAP_SQUARE, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hmenu, MM_ENDCAP_ROUND,  MF_GRAYED);
                EnableMenuItem(hmenu, MM_ENDCAP_FLAT,   MF_GRAYED);
                EnableMenuItem(hmenu, MM_ENDCAP_SQUARE, MF_GRAYED);
            }

            vNewPen(ghdc);
            vRedraw();
            break;

        case MM_STYLE_SOLID:
        case MM_STYLE_DOT:
        case MM_STYLE_DASH:
        case MM_STYLE_DASH_DOT:
        case MM_STYLE_DASH_DOT_DOT:
        case MM_STYLE_ALTERNATE:
        case MM_STYLE_ODD:
            CheckMenuItem(hmenu, giStyle, MF_UNCHECKED);
            giStyle = wCmd;
            CheckMenuItem(hmenu, giStyle, MF_CHECKED);

            vNewPen(ghdc);
            vRedraw();
            break;

        case MM_ENDCAP_ROUND:
        case MM_ENDCAP_FLAT:
        case MM_ENDCAP_SQUARE:
            CheckMenuItem(hmenu, giEndCap, MF_UNCHECKED);
            giEndCap = wCmd;
            CheckMenuItem(hmenu, giEndCap, MF_CHECKED);

            vNewPen(ghdc);
            vRedraw();
            break;

        case MM_ADD:
            if (gcBez < (MAXBEZ - 1))
            {
                gcBez++;
                vRedraw();
            }
            break;

        case MM_SUB:
            if (gcBez > 1)
            {
                gcBez--;
                vRedraw();
            }
            break;

        case MM_SLOWER:
            glSpeed += 20;
            SetTimer(hwnd,1,glSpeed,NULL);
            break;

        case MM_FASTER:
            if (glSpeed > 20)
            {
                glSpeed -= 20;
                SetTimer(hwnd,1,glSpeed,NULL);
            }
            break;

        case MM_PLUS:
            if (gcBand < (MAXBANDS - 1))
            {
                gcBand++;
                vRedraw();
            }
            break;

        case MM_MINUS:
            if (gcBand > 1)
            {
                gcBand--;
                vRedraw();
            }

        case MM_INCREASE:
            giVelMax++;
            break;

        case MM_DECREASE:
            if (giVelMax > 4)
            {
                giVelMax--;
            }
            break;

        case MM_DEBUG:
            gfl ^= BEZ_DEBUG;
            break;

        case MM_CLIP_LARGESTRIPES:
            gulStripe = 35;
            vSetClipMode(giClip);
            CheckMenuItem(hmenu, MM_CLIP_SMALLSTRIPES,  MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_LARGESTRIPES,  MF_CHECKED);
            break;

        case MM_CLIP_MEDIUMSTRIPES:
            gulStripe = 20;
            vSetClipMode(giClip);
            CheckMenuItem(hmenu, MM_CLIP_SMALLSTRIPES,  MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_CHECKED);
            CheckMenuItem(hmenu, MM_CLIP_LARGESTRIPES,  MF_UNCHECKED);
            break;

        case MM_CLIP_SMALLSTRIPES:
            gulStripe = 10;
            vSetClipMode(giClip);
            CheckMenuItem(hmenu, MM_CLIP_SMALLSTRIPES,  MF_CHECKED);
            CheckMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_UNCHECKED);
            CheckMenuItem(hmenu, MM_CLIP_LARGESTRIPES,  MF_UNCHECKED);
            break;

        case MM_NO_TITLEBAR:
            goto toggle_title;

        case MM_CLIP_FONT:

            vSelectClipFont(hwnd);
            vSetClipMode(gwOldClipMode);
            break;

        case MM_CLIP_NONE:
        case MM_CLIP_BOX:
        case MM_CLIP_CIRCLE:
        case MM_CLIP_BOXCIRCLE:
        case MM_CLIP_BOXCIRCLE_INVERT:
        case MM_CLIP_HORIZONTAL:
        case MM_CLIP_VERTICLE:
        case MM_CLIP_GRID:
        case MM_CLIP_TEXTPATH:
        case MM_CLIP_BOXTEXTPATH:

            CheckMenuItem(hmenu, gwOldClipMode, MF_UNCHECKED);
            CheckMenuItem(hmenu, wCmd, MF_CHECKED);

            gwOldClipMode = wCmd;

            if (wCmd == MM_CLIP_HORIZONTAL || wCmd == MM_CLIP_VERTICLE ||
                wCmd == MM_CLIP_GRID)
            {
                EnableMenuItem(hmenu, MM_CLIP_LARGESTRIPES, MF_ENABLED);
                EnableMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_ENABLED);
                EnableMenuItem(hmenu, MM_CLIP_SMALLSTRIPES, MF_ENABLED);
            }
            else
            {
                EnableMenuItem(hmenu, MM_CLIP_LARGESTRIPES, MF_GRAYED);
                EnableMenuItem(hmenu, MM_CLIP_MEDIUMSTRIPES, MF_GRAYED);
                EnableMenuItem(hmenu, MM_CLIP_SMALLSTRIPES, MF_GRAYED);
            }

            if ((wCmd == MM_CLIP_TEXTPATH) || (wCmd == MM_CLIP_BOXTEXTPATH))
                EnableMenuItem(hmenu, MM_CLIP_FONT, MF_ENABLED);
            else
                EnableMenuItem(hmenu, MM_CLIP_FONT, MF_GRAYED);

            if (wCmd == MM_CLIP_NONE)
            {
                EnableMenuItem(hmenu, MM_CLIP_COLOR, MF_GRAYED);
                EnableMenuItem(hmenu, MM_CLIP_COLOR_WASH, MF_GRAYED);
            }
            else
            {
                EnableMenuItem(hmenu, MM_CLIP_COLOR, MF_ENABLED);
                EnableMenuItem(hmenu, MM_CLIP_COLOR_WASH, MF_ENABLED);
            }

            vSetClipMode(wCmd);
            break;

        case MM_COLOR:
            cc.rgbResult = gcrDraw;
            if (ChooseColor(&cc))
            {
                gcrDraw = cc.rgbResult;
                vNewPen(ghdc);
            }

            vRedraw();
            break;

        case MM_CLIP_COLOR_WASH:

        // Toggle the color wash.

            if (giColorMode & COLOR_MODE_CLIPWASH)
            {
                giColorMode &= ~COLOR_MODE_CLIPWASH;
                CheckMenuItem(hmenu, MM_CLIP_COLOR_WASH, MF_UNCHECKED);
            }
            else
            {
                giColorMode |= COLOR_MODE_CLIPWASH;
                CheckMenuItem(hmenu, MM_CLIP_COLOR_WASH, MF_CHECKED);
            }

            vRedraw();
            break;

        case MM_CLIP_COLOR:
            cc.rgbResult = gcrClip;
            if (ChooseColor(&cc))
            {
                gcrClip = cc.rgbResult;
                DeleteObject(ghbrushClip);
                ghbrushClip = CreateSolidBrush(gcrClip);
            }

            vRedraw();
            break;
        }
        break;

    case WM_ERASEBKGND:
        break;

    case WM_PAINT:
        vRedraw();

    default:
defproc:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0L;
}

/***************************************************************************\
* About
*
* About dialog proc.
*
* History:
* 04-13-91 ScottLu      Created.
\***************************************************************************/

LONG About(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        if (wParam == IDOK)
            EndDialog(hDlg, wParam);
        break;
    }

    return FALSE;

    lParam;
    hDlg;
}


/*
 *  SetMenuBar() - places or removes the menu bar, etc.
 *
 *  Based on the flags in ClockDisp structure (ie: do we want a menu/title
 *  bar or not?), adds or removes the window title and menu bar:
 *    Gets current style, toggles the bits, and re-sets the style.
 *    Must then resize the window frame and show it.
 */

void NEAR PASCAL SetMenuBar( HWND hWnd )
{
    static DWORD wID;
    DWORD   dwStyle;

    dwStyle = GetWindowLong( hWnd, GWL_STYLE );
    if( bNoTitle ) {
        /* remove caption & menu bar, etc. */
        dwStyle &= ~(WS_DLGFRAME | WS_SYSMENU |
                   WS_MINIMIZEBOX | WS_MAXIMIZEBOX );
        wID = SetWindowLong( hWnd, GWL_ID, 0 );
    } else {
        /* put menu bar & caption back in */
        dwStyle = WS_TILEDWINDOW | dwStyle;
        SetWindowLong( hWnd, GWL_ID, wID );
    }
    SetWindowLong( hWnd, GWL_STYLE, dwStyle );
    SetWindowPos( hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE |
        SWP_NOZORDER | SWP_FRAMECHANGED );
    ShowWindow( hWnd, SW_SHOW );
}
