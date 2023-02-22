#include "stdafx.h"
#include "RadDialog.h"

CRadDialog::CRadDialog(UINT nIDTemplate, CWnd* pParent /*= NULL*/)
    : CDialogEx(nIDTemplate, pParent)
{
}

BEGIN_MESSAGE_MAP(CRadDialog, CDialogEx)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

HBRUSH CRadDialog::OnCtlColor(CDC* pDC, CWnd* /*pWnd*/, UINT nCtlColor)
{
    switch (nCtlColor)
    {
    case CTLCOLOR_EDIT:
        pDC->SetTextColor(GetGlobalData()->clrWindowText);
        pDC->SetBkColor(GetGlobalData()->clrWindow);
        return (HBRUSH) GetGlobalData()->brWindow.GetSafeHandle();

    default:
        pDC->SetTextColor(GetGlobalData()->clrBtnText);
        pDC->SetBkColor(GetGlobalData()->clrBtnFace);
        return (HBRUSH) GetGlobalData()->brBtnFace.GetSafeHandle();
    }
}
