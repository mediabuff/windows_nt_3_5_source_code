//
// ipaddr.cpp:  CWndIpAddress class control implementation
//

#include <afxwin.h>     // MFC core and standard components
#include <afxext.h>         // MFC extensions (including VB)

#include "ipaddr.hpp"

extern "C"
{
   #include "ipadd.h"
   #include "ipaddr.h"
}

    //  Static class-level data

    //  Super window proc address
WNDPROC CWndIpAddress :: m_wproc_super = NULL ;

    //  Window class initialization flag                  
BOOL CWndIpAddress :: m_b_inited = FALSE ;


WNDPROC * CWndIpAddress :: GetSuperWndProcAddr ()
{
    return & m_wproc_super ;
}


BOOL CWndIpAddress :: CreateWindowClass ( HINSTANCE hInstance )
{
    TRACE("CreateWindowClass");
    if ( ! m_b_inited ) 
    {
        m_b_inited = ::IPAddrInit( hInstance )  ;
    }
    return m_b_inited ;
}

IMPLEMENT_DYNAMIC(CWndIpAddress, CWnd)

CWndIpAddress :: CWndIpAddress ()
{
    TRACE("IP Address Constructor");
}

CWndIpAddress :: ~ CWndIpAddress ()
{
    TRACE("Destructor");
    DestroyWindow();
}

BOOL CWndIpAddress :: Create ( 
    LPCSTR lpszText, 
    DWORD dwStyle,
    const RECT & rect, 
    CWnd * pParentWnd, 
    UINT nID )
{
    return CWnd::Create( "IPAddress", lpszText, dwStyle, rect, pParentWnd, nID);
}

    //  Modification flag handling
void CWndIpAddress :: SetModify ( BOOL bModified )
{
    ::SendMessage( m_hWnd, IP_SETMODIFY, bModified, 0 );
}

BOOL CWndIpAddress :: GetModify () const
{
    return ::SendMessage( m_hWnd, IP_GETMODIFY, 0, 0 ) > 0 ;
}

void CWndIpAddress :: SetFocusField( int iField )
{
    ::SendMessage( m_hWnd, IP_SETFOCUS, iField, 0);
}

void CWndIpAddress::ClearAddress ( )
{
    ::SendMessage( m_hWnd, IP_CLEARADDRESS, 0, 0);
}

BOOL CWndIpAddress :: SetAddress ( DWORD dwAddr )
{
    return ::SendMessage( m_hWnd, IP_SETADDRESS, 0, dwAddr ) > 0 ;
}

BOOL CWndIpAddress :: GetAddress ( DWORD * pdwAddr ) const
{
    return ::SendMessage( m_hWnd, IP_GETADDRESS, 0,(LPARAM) pdwAddr ) > 0 ;
}
    void SetReadOnly (BOOL fReadonly = TRUE);
    void SetField(int dwField, BYTE bValue);

void CWndIpAddress :: SetReadOnly ( BOOL fReadOnly )
{
    ::SendMessage( m_hWnd, IP_SETREADONLY, (WPARAM)fReadOnly, (LPARAM)0 );
}

void CWndIpAddress :: SetField (int dwField, BOOL fSet, BYTE bValue)
{
    ::SendMessage( m_hWnd, IP_SETFIELD, (WPARAM)dwField, fSet 
        ? MAKELPARAM(0, MAKEWORD(0, bValue))
        : (LPARAM)-1
        );
}

BYTE CWndIpAddress :: GetMask () const
{
    DWORD dw = ::SendMessage( m_hWnd, IP_GETMASK, 0, 0);
    return (BYTE)dw;
}

BOOL CWndIpAddress :: SetMask(DWORD dwAddress, BYTE bMask)
{
    return ::SendMessage( m_hWnd, IP_SETMASK, (WPARAM)bMask, (LPARAM)dwAddress) > 0;
}
