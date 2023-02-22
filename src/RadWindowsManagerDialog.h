#pragma once
#include "afxwindowsmanagerdialog.h"

class CRadWindowsManagerDialog :
    public CMFCWindowsManagerDialog
{
public:
    CRadWindowsManagerDialog(CMDIFrameWndEx* pMDIFrame, BOOL bHelpButton = FALSE);
    ~CRadWindowsManagerDialog();

    DECLARE_MESSAGE_MAP()

    afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
