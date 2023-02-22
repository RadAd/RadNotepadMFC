#pragma once

template<class Base>
class CRadCtlColor : public Base
{
public:
    using Base::Base;

protected:
    HBRUSH OnCtlColor(CDC* pDC, CWnd* /*pWnd*/, UINT nCtlColor)
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

    DECLARE_MESSAGE_MAP()
};


BEGIN_TEMPLATE_MESSAGE_MAP(CRadCtlColor, Base, Base)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

typedef CRadCtlColor<CDialogEx> CRadDialog;
typedef CRadCtlColor<CMFCToolBar> CRadToolbar;
