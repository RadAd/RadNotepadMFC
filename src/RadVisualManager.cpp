#include "stdafx.h"
#include "RadVisualManager.h"

IMPLEMENT_DYNCREATE(CRadVisualManagerDark, CMFCVisualManagerOffice2007);

CRadVisualManagerDark::CRadVisualManagerDark()
{
    SetStatusBarOfficeXPLook(FALSE);
    CMFCButton::EnableWindowsTheming(FALSE);
}

void CRadVisualManagerDark::InitApp()
{
    enum PreferredAppMode
    {
        Default,
        AllowDark,
        ForceDark,
        ForceLight,
        Max
    };

    HMODULE hUxtheme = GetModuleHandle(L"uxtheme.dll");
    using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, in 1903
    fnSetPreferredAppMode SetPreferredAppMode = (fnSetPreferredAppMode) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));

    SetPreferredAppMode(AllowDark);
}

void CRadVisualManagerDark::Init(CWnd* pWnd, bool fDarkMode)
{
    // See https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
    HMODULE hUxtheme = GetModuleHandle(L"uxtheme.dll");
    using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow); // ordinal 133
    fnAllowDarkModeForWindow AllowDarkModeForWindow = (fnAllowDarkModeForWindow) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));

    SetWindowTheme(*pWnd, L"Explorer", NULL);
    AllowDarkModeForWindow(*pWnd, fDarkMode);
    pWnd->SendMessageW(WM_THEMECHANGED, 0, 0);

    CTreeCtrl* pTreeCtrl = DYNAMIC_DOWNCAST(CTreeCtrl, pWnd);
    if (pTreeCtrl != nullptr)
    {
        pTreeCtrl->SetBkColor(GetGlobalData()->clrWindow);
        pTreeCtrl->SetTextColor(GetGlobalData()->clrWindowText);
        pTreeCtrl->SetLineColor(GetGlobalData()->clrWindowText);
        // TODO Choose appropriate colors
        pTreeCtrl->SetInsertMarkColor(RGB(255, 0, 255));
    }
    CListCtrl* pListCtrl = DYNAMIC_DOWNCAST(CListCtrl, pWnd);
    if (pListCtrl != nullptr)
    {
        pListCtrl->SetBkColor(GetGlobalData()->clrWindow);
        pListCtrl->SetTextBkColor(GetGlobalData()->clrWindow);
        pListCtrl->SetTextColor(GetGlobalData()->clrWindowText);
        //pListCtrl->SetLineColor(GetGlobalData()->clrWindowText);
        // TODO Choose appropriate colors
        pListCtrl->SetInsertMarkColor(RGB(255, 0, 255));
    }
    CMFCTabCtrl* pTabCtrl = DYNAMIC_DOWNCAST(CMFCTabCtrl, pWnd);
    if (pTabCtrl != nullptr)
    {
        pTabCtrl->SetActiveTabColor(GetGlobalData()->clrWindow);
    }
}

void CRadVisualManagerDark::OnUpdateSystemColors()
{
    CMFCVisualManagerOffice2007::OnUpdateSystemColors();

    const COLORREF NotSet = RGB(255, 0, 255);
    const COLORREF UserChoiceColour = RGB(113, 96, 232);    // TODO Get this somehow
    //const COLORREF Test = RGB(0, 0, 255);

    GetGlobalData()->clrBtnFace = RGB(46, 46, 46);
    GetGlobalData()->clrBtnShadow = RGB(155, 155, 155);
    GetGlobalData()->clrBtnHilite = RGB(240, 240, 240);
    GetGlobalData()->clrBtnText = RGB(214, 214, 214); // Icon buttons
    GetGlobalData()->clrWindowFrame = NotSet;
    GetGlobalData()->clrBtnDkShadow = RGB(240, 240, 240);
    GetGlobalData()->clrBtnLight = RGB(155, 155, 155);
    GetGlobalData()->clrGrayedText = NotSet;
    GetGlobalData()->clrHilite = UserChoiceColour; // Properties entry selected
    GetGlobalData()->clrTextHilite = RGB(250, 250, 250); // Properties entry selected
    GetGlobalData()->clrHotLinkNormalText = RGB(101, 162, 250);
    GetGlobalData()->clrHotLinkHoveredText = RGB(51, 112, 250);
    GetGlobalData()->clrHotLinkVisitedText = NotSet;

    GetGlobalData()->clrBarWindow = NotSet;
    GetGlobalData()->clrBarFace = RGB(46, 46, 46);  // Properties group
    GetGlobalData()->clrBarShadow = RGB(31, 31, 31); // Thin border around main text window
    GetGlobalData()->clrBarHilite = RGB(128, 128, 128);   // Shadow on toolbar separator
    GetGlobalData()->clrBarDkShadow = RGB(250, 250, 250);  // Properties group / right side of tab shadow
    GetGlobalData()->clrBarLight = RGB(128, 128, 128); // Shadow above tab
    GetGlobalData()->clrBarText = RGB(250, 250, 250); // Text on tabs

    GetGlobalData()->clrWindow = RGB(31, 31, 31); // Properties entry
    GetGlobalData()->clrWindowText = RGB(250, 250, 250); // Properties entry

    GetGlobalData()->clrCaptionText = RGB(250, 250, 250);
    GetGlobalData()->clrMenuText = NotSet;
    GetGlobalData()->clrActiveCaption = RGB(31, 31, 31);
    GetGlobalData()->clrInactiveCaption = RGB(46, 46, 46);
    GetGlobalData()->clrInactiveCaptionText = RGB(187, 187, 187);
    GetGlobalData()->clrActiveCaptionGradient = NotSet;
    GetGlobalData()->clrInactiveCaptionGradient = NotSet;

    GetGlobalData()->clrActiveBorder = NotSet;
    GetGlobalData()->clrInactiveBorder = NotSet;

    DeleteObject(GetGlobalData()->hbrBtnHilite);
    GetGlobalData()->hbrBtnHilite = CreateSolidBrush(GetGlobalData()->clrBtnHilite);
    DeleteObject(GetGlobalData()->hbrBtnShadow);
    GetGlobalData()->hbrBtnShadow = CreateSolidBrush(GetGlobalData()->clrBtnShadow);
    DeleteObject(GetGlobalData()->hbrWindow);
    GetGlobalData()->hbrWindow = CreateSolidBrush(GetGlobalData()->clrWindow);

    GetGlobalData()->brBtnFace.DeleteObject();
    GetGlobalData()->brBtnFace.CreateSolidBrush(GetGlobalData()->clrBtnFace);
    GetGlobalData()->brHilite.DeleteObject();
    GetGlobalData()->brHilite.CreateSolidBrush(GetGlobalData()->clrHilite);
    const COLORREF clrLight = RGB(GetRValue(GetGlobalData()->clrBtnFace) + ((GetRValue(GetGlobalData()->clrBtnHilite) - GetRValue(GetGlobalData()->clrBtnFace)) / 2),
        GetGValue(GetGlobalData()->clrBtnFace) + ((GetGValue(GetGlobalData()->clrBtnHilite) - GetGValue(GetGlobalData()->clrBtnFace)) / 2),
        GetBValue(GetGlobalData()->clrBtnFace) + ((GetBValue(GetGlobalData()->clrBtnHilite) - GetBValue(GetGlobalData()->clrBtnFace)) / 2));
    GetGlobalData()->brLight.DeleteObject();
    GetGlobalData()->brLight.CreateSolidBrush(clrLight);
    GetGlobalData()->brBlack.DeleteObject();
    GetGlobalData()->brBlack.CreateSolidBrush(GetGlobalData()->clrBtnDkShadow);
    GetGlobalData()->brActiveCaption.DeleteObject();
    GetGlobalData()->brActiveCaption.CreateSolidBrush(GetGlobalData()->clrActiveCaption);
    GetGlobalData()->brInactiveCaption.DeleteObject();
    GetGlobalData()->brInactiveCaption.CreateSolidBrush(GetGlobalData()->clrInactiveCaption);
    GetGlobalData()->brWindow.DeleteObject();
    GetGlobalData()->brWindow.CreateSolidBrush(GetGlobalData()->clrWindow);

    GetGlobalData()->brBarFace.DeleteObject();
    GetGlobalData()->brBarFace.CreateSolidBrush(GetGlobalData()->clrBarFace);

    GetGlobalData()->penHilite.DeleteObject();
    GetGlobalData()->penHilite.CreatePen(PS_SOLID, 1, GetGlobalData()->clrHilite);
    GetGlobalData()->penBarFace.DeleteObject();
    GetGlobalData()->penBarFace.CreatePen(PS_SOLID, 1, GetGlobalData()->clrBarFace);
    GetGlobalData()->penBarShadow.DeleteObject();
    GetGlobalData()->penBarShadow.CreatePen(PS_SOLID, 1, GetGlobalData()->clrBarShadow);

    // ----------------------------- CMFCVisualManagerOfficeXP
    m_clrBarBkgnd = RGB(46, 46, 46);
    m_clrMenuRarelyUsed = NotSet;
    m_clrMenuLight = m_clrBarBkgnd;
    m_clrInactiveTabText = RGB(178, 178, 178);
    m_clrHighlight = RGB(61, 61, 61);
    m_clrHighlightDn = NotSet;// RGB(46, 46, 46);

    m_clrHighlightChecked = NotSet;
    m_clrPressedButtonBorder = NotSet;
    CMFCVisualManagerOfficeXP::m_clrGripper = NotSet;
    m_clrSeparator = RGB(61, 61, 61);
    m_clrPaneBorder = NotSet;
    m_clrMenuBorder = RGB(66, 66, 66);
    m_clrMenuItemBorder = RGB(112, 112, 112);

    // m_brGripperHorz;
    // m_brGripperVert;
    m_brBarBkgnd.DeleteObject();
    m_brBarBkgnd.CreateSolidBrush(m_clrBarBkgnd);
    m_brMenuRarelyUsed.DeleteObject();
    m_brMenuRarelyUsed.CreateSolidBrush(m_clrMenuRarelyUsed);
    m_brMenuLight.DeleteObject();
    m_brMenuLight.CreateSolidBrush(m_clrMenuLight);
    m_brTabBack.DeleteObject();
    m_brTabBack.CreateSolidBrush(GetGlobalData()->clrBarFace);
    //m_brTabBack.CreateSolidBrush(GetGlobalData()->clrBtnFace);
    m_brHighlight.DeleteObject();
    m_brHighlight.CreateSolidBrush(m_clrHighlight);
    m_brHighlightDn.DeleteObject();
    m_brHighlightDn.CreateSolidBrush(m_clrHighlightDn);
    m_brHighlightChecked.DeleteObject();
    m_brHighlightChecked.CreateSolidBrush(m_clrHighlightChecked);
    m_brFloatToolBarBorder.DeleteObject();
    m_brFloatToolBarBorder.CreateSolidBrush(CDrawingManager::PixelAlpha(GetGlobalData()->clrBarShadow, .85, .85, .85));
    //m_brFloatToolBarBorder.CreateSolidBrush(GetGlobalData()->clrBtnShadow);

    m_penSeparator.DeleteObject();
    m_penSeparator.CreatePen(PS_SOLID, 1, m_clrSeparator);
    m_penMenuItemBorder.DeleteObject();
    m_penMenuItemBorder.CreatePen(PS_SOLID, 1, m_clrMenuItemBorder);

    // ----------------------------- CMFCVisualManagerOffice2003
    m_clrBarGradientDark = RGB(31, 31, 31);
    m_clrBarGradientLight = RGB(31, 31, 31);
    m_clrToolBarGradientDark = RGB(46, 46, 46);
    m_clrToolBarGradientLight = RGB(46, 46, 46);
    m_clrToolbarDisabled = NotSet;
    m_clrToolBarGradientVertLight = NotSet;
    m_clrToolBarGradientVertDark = NotSet;
    m_clrCustomizeButtonGradientDark = RGB(46, 46, 46);
    m_clrCustomizeButtonGradientLight = RGB(46, 46, 46);
    m_clrToolBarBottomLine = RGB(61, 61, 61);
    m_colorToolBarCornerTop = NotSet;
    m_colorToolBarCornerBottom = NotSet;
    m_clrHighlightMenuItem = NotSet;
    m_clrHighlightGradientLight = RGB(61, 61, 61);
    m_clrHighlightGradientDark = RGB(61, 61, 61);
    m_clrHighlightDnGradientLight = RGB(61, 61, 61);
    m_clrHighlightDnGradientDark = RGB(61, 61, 61);
    m_clrHighlightCheckedGradientLight = RGB(128, 128, 128);
    m_clrHighlightCheckedGradientDark = RGB(128, 128, 128);
    m_clrGripper = NotSet;
    m_clrCaptionBarGradientLight = NotSet;
    m_clrCaptionBarGradientDark = NotSet;
    m_clrTaskPaneGradientDark = NotSet;
    m_clrTaskPaneGradientLight = NotSet;
    m_clrTaskPaneGroupCaptionDark = NotSet;
    m_clrTaskPaneGroupCaptionLight = NotSet;
    m_clrTaskPaneGroupCaptionSpecDark = NotSet;
    m_clrTaskPaneGroupCaptionSpecLight = NotSet;
    m_clrTaskPaneGroupAreaLight = NotSet;
    m_clrTaskPaneGroupAreaDark = NotSet;
    m_clrTaskPaneGroupAreaSpecLight = NotSet;
    m_clrTaskPaneGroupAreaSpecDark = NotSet;
    m_clrTaskPaneGroupBorder = NotSet;

    m_penBottomLine.DeleteObject();
    m_penBottomLine.CreatePen(PS_SOLID, 1, m_clrToolBarBottomLine);
    m_penSeparatorLight.DeleteObject();
    m_penSeparatorLight.CreatePen(PS_SOLID, 1, GetGlobalData()->clrBarHilite);
    m_penTaskPaneGroupBorder.DeleteObject();
    m_penTaskPaneGroupBorder.CreatePen(PS_SOLID, 1, m_clrTaskPaneGroupBorder);

    m_brTearOffCaption.DeleteObject();
    m_brTearOffCaption.CreateSolidBrush(GetGlobalData()->clrBarFace);
    m_brFace.DeleteObject();
    m_brFace.CreateSolidBrush(m_clrToolBarGradientLight);

    // ----------------------------- CMFCVisualManagerOffice2007
    m_clrAppCaptionActiveStart = NotSet;
    m_clrAppCaptionActiveFinish = NotSet;
    m_clrAppCaptionInactiveStart = NotSet;
    m_clrAppCaptionInactiveFinish = NotSet;
    m_clrAppCaptionActiveText = NotSet;
    m_clrAppCaptionInactiveText = NotSet;
    m_clrAppCaptionActiveTitleText = NotSet;
    m_clrAppCaptionInactiveTitleText = NotSet;
    m_clrMainClientArea = RGB(128, 128, 128);
    m_clrMenuBarGradientLight = RGB(31, 31, 31);
    m_clrMenuBarGradientDark = RGB(31, 31, 31);
    m_clrMenuBarGradientVertLight = NotSet;
    m_clrMenuBarGradientVertDark = NotSet;
    m_clrMenuBarBtnText = RGB(250, 250, 250);
    m_clrMenuBarBtnTextHighlighted = RGB(250, 250, 250);
    m_clrMenuBarBtnTextDisabled = NotSet;
    m_clrToolBarBtnText = NotSet;
    m_clrToolBarBtnTextHighlighted = NotSet;
    m_clrToolBarBtnTextDisabled = NotSet;
    m_clrMenuText = RGB(255, 255, 255);
    m_clrMenuTextHighlighted = RGB(255, 255, 255);
    m_clrMenuTextDisabled = RGB(71, 71, 71);
    m_clrStatusBarText = RGB(255, 255, 255);
    m_clrStatusBarTextDisabled = RGB(71, 71, 71);
    m_clrExtenedStatusBarTextDisabled = NotSet;
    m_clrEditBorder = NotSet;
    m_clrEditBorderDisabled = NotSet;
    m_clrEditBorderHighlighted = NotSet;
    m_clrEditSelection = NotSet;
    m_clrComboBorder = RGB(61, 61, 61);
    m_clrComboBorderDisabled = RGB(61, 61, 61);
    m_clrComboBorderPressed = RGB(112, 112, 112);
    m_clrComboBorderHighlighted = RGB(66, 66, 66);
    m_clrComboBtnStart = RGB(71, 71, 71);
    m_clrComboBtnFinish = RGB(31, 31, 31);
    m_clrComboBtnBorder = RGB(31, 31, 31);
    m_clrComboBtnDisabledStart = RGB(51, 51, 51);
    m_clrComboBtnDisabledFinish = RGB(51, 51, 51);
    m_clrComboBtnBorderDisabled = RGB(61, 61, 61);
    m_clrComboBtnPressedStart = RGB(61, 61, 61);
    m_clrComboBtnPressedFinish = RGB(61, 61, 61);
    m_clrComboBtnBorderPressed = RGB(71, 71, 71);
    m_clrComboBtnHighlightedStart = RGB(61, 61, 61);
    m_clrComboBtnHighlightedFinish = RGB(61, 61, 61);
    m_clrComboBtnBorderHighlighted = RGB(31, 31, 31);
    m_clrComboSelection = NotSet;
    m_clrHeaderNormalStart = NotSet;
    m_clrHeaderNormalFinish = NotSet;
    m_clrHeaderNormalBorder = NotSet;
    m_clrHeaderHighlightedStart = NotSet;
    m_clrHeaderHighlightedFinish = NotSet;
    m_clrHeaderHighlightedBorder = NotSet;
    m_clrHeaderPressedStart = NotSet;
    m_clrHeaderPressedFinish = NotSet;
    m_clrHeaderPressedBorder = NotSet;
    m_clrBarCaption = NotSet;
    m_clrMiniFrameCaption = NotSet;
    m_clrSeparator1 = RGB(61, 61, 61);
    m_clrSeparator2 = RGB(61, 61, 61);

    m_clrGroupText = NotSet;
    m_clrCaptionBarText = NotSet;
    m_clrTaskPaneGroupCaptionHighDark = NotSet;
    m_clrTaskPaneGroupCaptionHighLight = NotSet;
    m_clrTaskPaneGroupCaptionHighSpecDark = NotSet;
    m_clrTaskPaneGroupCaptionHighSpecLight = NotSet;
    m_clrTaskPaneGroupCaptionTextSpec = NotSet;
    m_clrTaskPaneGroupCaptionTextHighSpec = NotSet;
    m_clrTaskPaneGroupCaptionText = NotSet;
    m_clrTaskPaneGroupCaptionTextHigh = NotSet;
    m_clrTabFlatBlack = RGB(71, 71, 71);
    m_clrTabFlatHighlight = NotSet;
    m_clrTabTextActive = NotSet;
    m_clrTabTextInactive = NotSet;
    m_clrOutlookPageTextNormal = NotSet;
    m_clrOutlookPageTextHighlighted = NotSet;
    m_clrOutlookPageTextPressed = NotSet;
    m_clrOutlookCaptionTextNormal = NotSet;
    m_clrRibbonCategoryText = NotSet;
    m_clrRibbonCategoryTextHighlighted = NotSet;
    m_clrRibbonCategoryTextDisabled = NotSet;
    m_clrRibbonPanelText = NotSet;
    m_clrRibbonPanelTextHighlighted = NotSet;
    m_clrRibbonPanelCaptionText = NotSet;
    m_clrRibbonPanelCaptionTextHighlighted = NotSet;
    m_clrRibbonKeyTipTextNormal = NotSet;
    m_clrRibbonKeyTipTextDisabled = NotSet;
    m_clrRibbonEdit = NotSet;
    m_clrRibbonEditDisabled = NotSet;
    m_clrRibbonEditHighlighted = NotSet;
    m_clrRibbonEditPressed = NotSet;
    m_clrRibbonEditBorder = NotSet;
    m_clrRibbonEditBorderDisabled = NotSet;
    m_clrRibbonEditBorderHighlighted = NotSet;
    m_clrRibbonEditBorderPressed = NotSet;
    m_clrRibbonEditSelection = NotSet;
    m_clrRibbonComboBtnStart = NotSet;
    m_clrRibbonComboBtnFinish = NotSet;
    m_clrRibbonComboBtnBorder = NotSet;
    m_clrRibbonComboBtnDisabledStart = NotSet;
    m_clrRibbonComboBtnDisabledFinish = NotSet;
    m_clrRibbonComboBtnBorderDisabled = NotSet;
    m_clrRibbonComboBtnPressedStart = NotSet;
    m_clrRibbonComboBtnPressedFinish = NotSet;
    m_clrRibbonComboBtnBorderPressed = NotSet;
    m_clrRibbonComboBtnHighlightedStart = NotSet;
    m_clrRibbonComboBtnHighlightedFinish = NotSet;
    m_clrRibbonComboBtnBorderHighlighted = NotSet;
    m_clrRibbonContextPanelText = NotSet;
    m_clrRibbonContextPanelTextHighlighted = NotSet;
    m_clrRibbonContextPanelCaptionText = NotSet;
    m_clrRibbonContextPanelCaptionTextHighlighted = NotSet;
    m_clrPlannerTodayCaption[0] = NotSet;
    m_clrPlannerTodayCaption[1] = NotSet;
    m_clrPlannerTodayCaption[2] = NotSet;
    m_clrPlannerTodayCaption[3] = NotSet;
    m_clrPlannerTodayBorder = NotSet;
    m_clrPlannerNcArea = NotSet;
    m_clrPlannerNcLine = NotSet;
    m_clrPlannerNcText = NotSet;
    m_clrPopupGradientLight = NotSet;
    m_clrPopupGradientDark = NotSet;
    m_clrRibbonHyperlinkInactive = NotSet;
    m_clrRibbonHyperlinkActive = NotSet;
    m_clrRibbonStatusbarHyperlinkInactive = NotSet;
    m_clrRibbonStatusbarHyperlinkActive = NotSet;

    m_penTabFlatInner[0].DeleteObject();
    m_penTabFlatInner[0].CreatePen(PS_SOLID, 1, NotSet);
    m_penTabFlatInner[1].DeleteObject();
    m_penTabFlatInner[1].CreatePen(PS_SOLID, 1, NotSet);
    m_penTabFlatOuter[0].DeleteObject();
    m_penTabFlatOuter[0].CreatePen(PS_SOLID, 1, NotSet);
    m_penTabFlatOuter[1].DeleteObject();
    m_penTabFlatOuter[1].CreatePen(PS_SOLID, 1, NotSet);
    m_penSeparator2.DeleteObject();
    m_penSeparator2.CreatePen(PS_SOLID, 1, m_clrSeparator2);
    m_penSeparatorDark.DeleteObject();
    m_penSeparatorDark.CreatePen(PS_SOLID, 1, CDrawingManager::PixelAlpha(m_clrToolBarBottomLine, RGB(255, 255, 255), 95));
    //m_penSeparatorLight.DeleteObject();
    //m_penSeparatorLight.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    m_brMainClientArea.DeleteObject();
    m_brMainClientArea.CreateSolidBrush(m_clrMainClientArea);
    m_brGroupBackground.DeleteObject();
    m_brGroupBackground.CreateSolidBrush(NotSet);
}
