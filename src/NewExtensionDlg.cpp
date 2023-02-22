// NewExtensionDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NewExtensionDlg.h"
#include "Settings.h"
#include "..\Resource.h"
#include "afxdialogex.h"


// CNewExtensionDlg dialog

IMPLEMENT_DYNAMIC(CNewExtensionDlg, CRadDialog)

CNewExtensionDlg::CNewExtensionDlg(Settings* pSettings, CWnd* pParent /*=NULL*/)
	: CRadDialog(IDD_NEW_EXTENSION, pParent)
    , m_pSettings(pSettings)
{

}

CNewExtensionDlg::~CNewExtensionDlg()
{
}

static bool IsEditCtrl(CWnd* pCtrl)
{
    TCHAR strClassName[255];
    GetClassName(pCtrl->GetSafeHwnd(), strClassName, ARRAYSIZE(strClassName));
    return _wcsicmp(strClassName, L"edit") == 0;
}

static void ShowError(CDataExchange* pDX, LPCTSTR strError)
{
    CString strLabel;
    CWnd* pLabel = pDX->m_pDlgWnd->GetDlgItem(pDX->m_idLastControl)->GetWindow(GW_HWNDPREV);
    if (pLabel != nullptr)
    {
        pLabel->GetWindowText(strLabel);
        strLabel.Replace(_T("&"), _T(""));
    }
    CWnd* pCtrl = pDX->m_pDlgWnd->GetDlgItem(pDX->m_idLastControl);
    if (pCtrl != nullptr && IsEditCtrl(pCtrl))
    {
        CEdit* pEdit = (CEdit*) pCtrl;
        EDITBALLOONTIP ebt = { sizeof(EDITBALLOONTIP) };
        ebt.pszTitle = strLabel;
        ebt.pszText = strError;
        ebt.ttiIcon = TTI_ERROR;
        pEdit->ShowBalloonTip(&ebt);
    }
    else
    {
        CString msg;
        if (!strLabel.IsEmpty())
            msg.Format(L"%s : %s", strLabel.GetString(), strError);
        else
            msg = strError;
        pDX->m_pDlgWnd->MessageBox(msg, nullptr, MB_ICONEXCLAMATION);
    }
    pDX->Fail();
}

static void Validate(CDataExchange* pDX, bool bTest, LPCTSTR pMessage)
{
    if (pDX->m_bSaveAndValidate && !bTest)
    {
        ShowError(pDX, pMessage);
    }
}

void CNewExtensionDlg::DoDataExchange(CDataExchange* pDX)
{
    CRadDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EXTENSION, m_strExtension);
    Validate(pDX, !m_strExtension.IsEmpty(), _T("Must not be empty."));
    Validate(pDX, m_pSettings->user.mapExt.find(m_strExtension) == m_pSettings->user.mapExt.end(), _T("Already exists."));
}


BEGIN_MESSAGE_MAP(CNewExtensionDlg, CRadDialog)
END_MESSAGE_MAP()


// CNewExtensionDlg message handlers
