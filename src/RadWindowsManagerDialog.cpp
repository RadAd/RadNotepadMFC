#include "stdafx.h"
#include "RadWindowsManagerDialog.h"

CRadWindowsManagerDialog::CRadWindowsManagerDialog(CMDIFrameWndEx* pMDIFrame, BOOL bHelpButton)
    : CMFCWindowsManagerDialog(pMDIFrame, bHelpButton)
{
}

CRadWindowsManagerDialog::~CRadWindowsManagerDialog()
{
}

BEGIN_MESSAGE_MAP(CRadWindowsManagerDialog, CMFCWindowsManagerDialog)
    ON_WM_DRAWITEM()
END_MESSAGE_MAP()

void CRadWindowsManagerDialog::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
    // Copied from CMFCWindowsManagerDialog::OnDrawItem

    if (nIDCtl != IDC_AFXBARRES_LIST)
    {
        CDialog::OnDrawItem(nIDCtl, lpDIS);
    }

    if (lpDIS->itemID == LB_ERR)
    {
        return;
    }

    CBrush& brFill = (lpDIS->itemState & ODS_SELECTED) ? GetGlobalData()->brHilite : GetGlobalData()->brWindow;
    COLORREF clText = (lpDIS->itemState & ODS_SELECTED) ? GetGlobalData()->clrTextHilite : GetGlobalData()->clrWindowText;
    CRect rect = lpDIS->rcItem;
    CDC* pDC = CDC::FromHandle(lpDIS->hDC);

    if (lpDIS->itemAction &(ODA_DRAWENTIRE | ODA_SELECT))
    {
        pDC->FillRect(rect, &brFill);
    }

    pDC->SetBkMode(TRANSPARENT);
    pDC->SetTextColor(clText);

    //-----------
    // Draw text:
    //-----------
    CString str;
    m_wndList.GetText(lpDIS->itemID, str);

    CRect rectText = rect;
    rectText.left += rectText.Height() + 4;

    pDC->DrawText(str, rectText, DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);

    //-----------
    // Draw icon:
    //-----------
    //HICON hIcon = (HICON) (LONG_PTR) GetClassLongPtr((HWND) lpDIS->itemData, GCLP_HICONSM);
    HICON hIcon = (HICON) ::SendMessage((HWND) lpDIS->itemData, WM_GETICON, FALSE, 0);
    if (hIcon != NULL)
    {
        CRect rectIcon = rect;
        rectIcon.right = rectIcon.left + rectIcon.Height();
        rectIcon.DeflateRect(2, 0);

        ::DrawIconEx(pDC->GetSafeHdc(), rectIcon.left, rectIcon.top, hIcon, rectIcon.Height(), rectIcon.Height(), 0, NULL, DI_NORMAL);
    }

    if (lpDIS->itemAction & ODA_FOCUS)
    {
        pDC->DrawFocusRect(rect);
    }
}
