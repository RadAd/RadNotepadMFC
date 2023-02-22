#pragma once

// CRadDialog dialog

class CRadDialog : public CDialogEx
{
public:
    CRadDialog(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor

protected:
    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

    DECLARE_MESSAGE_MAP()
};
