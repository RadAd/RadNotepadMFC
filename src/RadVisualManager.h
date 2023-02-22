#pragma once

class CRadVisualManagerDark : public CMFCVisualManagerOffice2007
{
    DECLARE_DYNCREATE(CRadVisualManagerDark);

    static void InitApp();
    static void Init(CWnd* pWnd, bool fDarkMode);

private:
    CRadVisualManagerDark();

    virtual void OnUpdateSystemColors();


    virtual void OnFillButtonInterior(CDC* pDC, CMFCToolBarButton* pButton, CRect rect, CMFCVisualManager::AFX_BUTTON_STATE state)
    {
        // Skip CMFCVisualManagerOffice2007, call CMFCVisualManagerOffice2003
        CMFCVisualManagerOffice2003::OnFillButtonInterior(pDC, pButton, rect, state);
    }

    virtual void OnHighlightMenuItem(CDC* pDC, CMFCToolBarMenuButton* pButton, CRect rect, COLORREF& clrText)
    {
        // Skip CMFCVisualManagerOffice2007, call CMFCVisualManagerOffice2003
        CMFCVisualManagerOffice2003::OnHighlightMenuItem(pDC, pButton, rect, clrText);
    }

    virtual void OnDrawTab(CDC* pDC, CRect rectTab, int iTab, BOOL bIsActive, const CMFCBaseTabCtrl* pTabWnd)
    {
        // Skip CMFCVisualManagerOffice2007, call CMFCVisualManagerOffice2003
        CMFCVisualManagerOffice2003::OnDrawTab(pDC, rectTab, iTab, bIsActive, pTabWnd);
    }

    virtual void OnFillTab(CDC* pDC, CRect rectFill, CBrush* pbrFill, int iTab, BOOL bIsActive, const CMFCBaseTabCtrl* pTabWnd)
    {
        // Skip CMFCVisualManagerOffice2007, call CMFCVisualManagerOfficeXP
        CMFCVisualManagerOfficeXP::OnFillTab(pDC, rectFill, pbrFill, iTab, bIsActive, pTabWnd);
    }

    virtual void OnFillBarBackground(CDC* pDC, CBasePane* pBar, CRect rectClient, CRect rectClip, BOOL bNCArea = FALSE)
    {
        CMFCVisualManagerOffice2003::OnFillBarBackground(pDC, pBar, rectClient, rectClip, bNCArea);
    }

    virtual void OnEraseTabsArea(CDC* pDC, CRect rect, const CMFCBaseTabCtrl* pTabWnd)
    {
        CMFCVisualManagerOffice2003::OnEraseTabsArea(pDC, rect, pTabWnd);
    }
};
