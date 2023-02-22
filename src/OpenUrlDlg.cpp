// OpenUrl.cpp : implementation file
//

#include "stdafx.h"
#include "OpenUrlDlg.h"
#include "afxdialogex.h"
#include "..\resource.h"

#ifdef UNICODE
#define CF_TTEXT    CF_UNICODETEXT
#else
#define CF_TTEXT    CF_TEXT
#endif

inline bool StartsWith(LPCTSTR pStr, LPCTSTR pBegin)
{
    return _tcsncicmp(pStr, pBegin, _tcsclen(pBegin)) == 0;
}

// OpenUrl dialog

IMPLEMENT_DYNAMIC(COpenUrlDlg, CRadDialog)

COpenUrlDlg::COpenUrlDlg(CWnd* pParent /*=nullptr*/)
    : CRadDialog(IDD_OPEN_URL, pParent)
    , m_strUrl(_T(""))
{
    if (pParent->OpenClipboard())
    {
        HANDLE hClip = GetClipboardData(CF_TTEXT);
        if (hClip != NULL)
        {
            LPCTSTR pStr = static_cast<LPCTSTR>(GlobalLock(hClip));

            if (StartsWith(pStr, _T("http:")) || StartsWith(pStr, _T("https:")) || StartsWith(pStr, _T("ftp:")))
                m_strUrl = pStr;

            GlobalUnlock(hClip);
        }

        CloseClipboard();
    }
}

void COpenUrlDlg::DoDataExchange(CDataExchange* pDX)
{
    CRadDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_URL, m_strUrl);
}


BEGIN_MESSAGE_MAP(COpenUrlDlg, CRadDialog)
END_MESSAGE_MAP()


// OpenUrl message handlers
