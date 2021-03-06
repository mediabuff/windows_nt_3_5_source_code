
/*****************************************************************************

                                D I A L O G S

    Name:       dialogs.c
    Date:       21-Jan-1994
    Creator:    Unknown

    Description:
        Dialog handling routines.

*****************************************************************************/



#include <windows.h>
#include <nddeapi.h>

#include "common.h"
#include "clipbook.h"
#include "clipbrd.h"
#include "clpbkdlg.h"
#include "cvutil.h"
#include "dialogs.h"
#include "helpids.h"
#include "debugout.h"
#include "ismember.h"
#include "shares.h"



#define      PERM_READ  (NDDEACCESS_REQUEST|NDDEACCESS_ADVISE)
#define      PERM_WRITE (NDDEACCESS_REQUEST|NDDEACCESS_ADVISE|NDDEACCESS_POKE|NDDEACCESS_EXECUTE)





static BOOL    IsUniqueName (LPTSTR key);
static BOOL    StrHas2BkSlash (LPTSTR  str);




/*
 *      ConnectDlgProc
 */

BOOL CALLBACK ConnectDlgProc (
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{

    switch (message)
        {
        case WM_INITDIALOG:
            szConvPartner[0] = '\0';
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    GetDlgItemText (hwnd, IDC_CONNECTNAME, szConvPartner, 32);
                    EndDialog (hwnd, 1);
                    break;

                case IDCANCEL:
                    szConvPartner[0] = '\0';
                    EndDialog (hwnd, 0);
                    break;

                default:
                    return FALSE;
                }
            break;

        default:
            return FALSE;
        }

    return TRUE;

}






/*
 *      ShareDlgProc
 *
 *  Note: this routine expectes a PNDDESHAREINFO in lParam!
 */

BOOL CALLBACK ShareDlgProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam)
{
static PNDDESHAREINFO   lpDdeS;
DWORD                   dwTrustOptions;
DWORD                   adwTrust[3];

BOOL                    bRet = TRUE;

// These vars are used for determining if I'm owner of the page
PSID                    psidPage;
BOOL                    fDump;
DWORD                   cbSD;
UINT                    uRet;
PSECURITY_DESCRIPTOR    pSD = NULL;




    switch (message)
        {
        case WM_INITDIALOG:

            lpDdeS = (PNDDESHAREINFO)lParam;

            // set share, always static
            SetDlgItemText (hwnd, IDC_STATICSHARENAME, lpDdeS->lpszShareName+1 );

            // If the current user doesn't own the page, we gray out the
            // "start app" and "run minimized" checkboxes.. basically, people
            // who aren't the owner don't get to trust the share.
            EnableWindow(GetDlgItem(hwnd, IDC_STARTAPP), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_MINIMIZED), FALSE);
            EnableWindow(GetDlgItem(hwnd, 207), FALSE);



            // Figure out who owns the page
            psidPage = NULL;
            if (!(pSD = LocalAlloc(LPTR, 50)))
                {
                PERROR(TEXT("Couldn't alloc 50 bytes\r\n"));
                break;
                }


            uRet = NDdeGetShareSecurity (NULL,
                                         lpDdeS->lpszShareName,
                                         OWNER_SECURITY_INFORMATION,
                                         pSD,
                                         50,
                                         &cbSD);

            if (uRet == NDDE_BUF_TOO_SMALL)
                {
                LocalFree (pSD);

                if (!(pSD = LocalAlloc(LPTR, cbSD)))
                   {
                   PERROR(TEXT("Couldn't alloc %ld bytes\r\n"), cbSD);
                   break;
                   }


                uRet = NDdeGetShareSecurity (NULL,
                                             lpDdeS->lpszShareName,
                                             OWNER_SECURITY_INFORMATION,
                                             pSD,
                                             cbSD,
                                             &cbSD);
                }


            if (NDDE_NO_ERROR != uRet)
                {
                PERROR(TEXT("GetSec fail %d"), uRet);
                break;
                }


            if (!GetSecurityDescriptorOwner(pSD, &psidPage, &fDump))
                {
                PERROR(TEXT("Couldn't get owner, even tho we asked\r\n"));
                break;
                }

            if (!psidPage || !IsUserMember(psidPage))
                {
                PINFO(TEXT("User isn't member of owner\r\n"));
                break;
                }



            EnableWindow (GetDlgItem (hwnd, IDC_STARTAPP), TRUE);

            // 207 is the group box around the checkboxes
            EnableWindow (GetDlgItem (hwnd, 207), TRUE);

            NDdeGetTrustedShare (NULL,
                                 lpDdeS->lpszShareName,
                                 adwTrust,
                                 adwTrust + 1,
                                 adwTrust + 2);

            if (!(adwTrust[0] & NDDE_TRUST_SHARE_START))
                {
                PINFO (TEXT("Buttons shouldn't check\r\n"));
                }
            else
                {
                CheckDlgButton(hwnd, IDC_STARTAPP, 1);

                EnableWindow (GetDlgItem (hwnd, IDC_MINIMIZED), TRUE);
                CheckDlgButton (hwnd,
                                IDC_MINIMIZED,
                                (SW_MINIMIZE == (adwTrust[0] & NDDE_CMD_SHOW_MASK)) ? 1 : 0);
               }

            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                   dwTrustOptions = NDDE_TRUST_SHARE_INIT;

                   if (IsDlgButtonChecked(hwnd, IDC_STARTAPP))
                      {
                      dwTrustOptions |= NDDE_TRUST_SHARE_START;

                      if (IsDlgButtonChecked(hwnd, IDC_MINIMIZED))
                         {
                         dwTrustOptions |= NDDE_TRUST_CMD_SHOW | SW_MINIMIZE;
                         }
                      }

                   // Update the share start flag.
                   if (dwTrustOptions & NDDE_TRUST_SHARE_START)
                       lpDdeS->fStartAppFlag = TRUE;
                   else
                       lpDdeS->fStartAppFlag = FALSE;

                   NDdeSetTrustedShare(NULL, lpDdeS->lpszShareName, dwTrustOptions);
                   EndDialog (hwnd, TRUE);
                   break;

                case IDCANCEL:
                   EndDialog (hwnd, FALSE);
                   break;

                case IDHELP:
                   WinHelp (hwnd, szHelpFile, HELP_CONTEXT, IDH_SHAREDLG);
                   break;

                case IDC_PERMISSIONS:
                   EditPermissions2 (hwnd, lpDdeS->lpszShareName, FALSE);
                   break;

                case  IDC_STARTAPP:
                   EnableWindow(GetDlgItem(hwnd, IDC_MINIMIZED),
                         IsDlgButtonChecked(hwnd, IDC_STARTAPP));
                   break;

                default:
                   bRet = FALSE;
                }
            break;

        default:
            bRet = FALSE;
        }



    if (pSD)
        LocalFree (pSD);

    return bRet;

}










/*
 *      IsUniqueName
 *
 *  Check to see if the name is unique
 */

static BOOL IsUniqueName (
    LPTSTR key)
{
PMDIINFO    pMDI;
LISTENTRY   ListEntry;


    if (!(pMDI = GETMDIINFO(hwndLocal)))
        return FALSE;


    lstrcpy (ListEntry.name, key);

    if (SendMessage (pMDI->hWndListbox,
                     LB_FINDSTRING,
                     (WPARAM)-1,
                     (LPARAM)(LPCSTR) &ListEntry )
        != LB_ERR )
       {
       return FALSE;
       }

    return TRUE;

}







/*
 *      StrHas2BkSlash
 *
 *  Check to see if the string has
 *  double back slashes.  If double
 *  back slashes exists then return
 *  TRUE else return FALSE.
 */

static BOOL    StrHas2BkSlash (LPTSTR  str)
{
TCHAR   c;

    while (c = *str++)
        {
        if (c == TEXT('\\') && *str == TEXT('\\'))
            return TRUE;
        }

    return FALSE;

}






/*
 *      KeepAsDlgProc
 *
 *  Ask the user for a page name.
 */

BOOL CALLBACK KeepAsDlgProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam)
{

    switch (msg)
        {
        case WM_INITDIALOG:
            SendDlgItemMessage ( hwnd, IDC_KEEPASEDIT, EM_LIMITTEXT, MAX_NDDESHARENAME - 15, 0L );
            SendDlgItemMessage ( hwnd, IDC_SHARECHECKBOX, BM_SETCHECK, fSharePreference, 0L );
            break;

        case WM_COMMAND:
            switch (wParam)
                {
                case IDOK:
                    fSharePreference = (BOOL)SendDlgItemMessage (hwnd,
                                                                 IDC_SHARECHECKBOX,
                                                                 BM_GETCHECK,
                                                                 0,
                                                                 0L );

                    if (!GetDlgItemText(hwnd, IDC_KEEPASEDIT, szKeepAs+1, MAX_PAGENAME_LENGTH))
                        {
                        SetFocus (GetDlgItem (hwnd, IDC_KEEPASEDIT));
                        break;
                        }

                    szKeepAs[0] = SHR_CHAR;

                    if (!NDdeIsValidShareName(szKeepAs + 1))
                        //|| StrHas2BkSlash (szKeepAs)) // this was a temp fix for NDde bug
                       {
                       MessageBoxID (hInst,
                                     hwnd,
                                     IDS_PAGENAMESYNTAX,
                                     IDS_PASTEDLGTITLE,
                                     MB_OK|MB_ICONEXCLAMATION);
                       break;
                       }

                    szKeepAs[0] = UNSHR_CHAR;

                    // make sure name is unique
                    if ( !IsUniqueName (szKeepAs))
                       {
                       MessageBoxID (hInst,
                                     hwnd,
                                     IDS_NAMEEXISTS,
                                     IDS_PASTEDLGTITLE,
                                     MB_OK|MB_ICONEXCLAMATION);
                       break;
                       }

                    EndDialog( hwnd, TRUE );
                    break;

                case IDCANCEL:
                     EndDialog( hwnd, FALSE );
                     break;

                case IDHELP:
                     WinHelp(hwnd, szHelpFile, HELP_CONTEXT, IDH_DLG_PASTEDATA );
                     break;

                default:
                     return FALSE;
                }
            break;

        default:
            return FALSE;
        }

    return TRUE;

}
