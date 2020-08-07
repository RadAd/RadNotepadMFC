#include "stdafx.h"
#include "RadNotepad.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "RadNotepadView.h"
#include "RadWindowsManagerDialog.h"
#include "RadDocManager.h"
#include "RadWaitCursor.h"
#include "RadToolBarsCustomizeDialog.h"
#include "ToolBarHistoryButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#if 0
// Copied from CMFCToolBarImages::AddIcon because it is failing if pImages->IsScaled()
static int AddIcon(CMFCToolBarImages* pImages, HICON hIcon)
{
    CWindowDC dc(NULL);

    CDC dcMem;
    dcMem.CreateCompatibleDC(NULL);

    CBitmap bmpMem;

    CSize sizeIcon = pImages->GetImageSize();

    bmpMem.CreateCompatibleBitmap(&dc, sizeIcon.cx, sizeIcon.cy);

    CBitmap* pBmpOriginal = dcMem.SelectObject(&bmpMem);

    dcMem.FillRect(CRect(0, 0, sizeIcon.cx, sizeIcon.cy), &(GetGlobalData()->brBtnFace));

    if (hIcon != NULL)
    {
        dcMem.DrawState(CPoint(0, 0), sizeIcon, hIcon, DSS_NORMAL, (CBrush*) NULL);
    }

    dcMem.SelectObject(pBmpOriginal);

    return pImages->AddImage(bmpMem);
}
#endif

UINT NEAR WM_RADNOTEPAD = RegisterWindowMessage(_T("RADNOTEPAD"));

static LRESULT CALLBACK MDIClientHookWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    CMainFrame* pMainWnd = (CMainFrame*) dwRefData;
    switch (msg)
    {
    case WM_MDINEXT:
        {
            CWnd* pWndMDIChild = CWnd::FromHandle((HWND) wp);
            pMainWnd->ChildMDINextWindow(pWndMDIChild, lp != 0);
            return 0;
        }
        break;

    case WM_MDIACTIVATE:
        {
            CWnd* pWndMDIChild = CWnd::FromHandle((HWND) wp);
            pMainWnd->ChildMDIActiviate(pWndMDIChild);
            return DefSubclassProc(hWnd, msg, wp, lp);
        }
        break;

    case WM_MDIDESTROY:
        {
            CWnd* pWndMDIChild = CWnd::FromHandle((HWND) wp);
            pMainWnd->ChildMDIDestroyed(pWndMDIChild);
            return DefSubclassProc(hWnd, msg, wp, lp);
        }
        break;

    default:
        // for all untreated messages, call the original wndproc
        return DefSubclassProc(hWnd, msg, wp, lp);
        break;
    }
}

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
    ON_WM_CREATE()
    ON_WM_SETTINGCHANGE()
    ON_WM_CONTEXTMENU()
    ON_MESSAGE(WM_SETMESSAGESTRING, &CMainFrame::OnSetMessageString)
    ON_COMMAND(ID_WINDOW_MANAGER, &CMainFrame::OnWindowManager)
    ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
    ON_COMMAND_RANGE(ID_VIEW_FILEVIEW, ID_VIEW_CLASSVIEW, &CMainFrame::OnViewPane)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_FILEVIEW, ID_VIEW_CLASSVIEW, &CMainFrame::OnUpdateViewPane)
    ON_COMMAND(ID_INDICATOR_SCHEME, &CMainFrame::OnSchemeIndicator)
    ON_COMMAND(ID_INDICATOR_LINE_ENDING, &CMainFrame::OnLineEndingIndicator)
    ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LINE, ID_INDICATOR_LINE_ENDING, &CMainFrame::OnUpdateClear)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, &CMainFrame::OnUpdateClear)
    ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
    ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, &CMainFrame::OnAfxWmOnGetTabTooltip)
    ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, &CMainFrame::OnAfxWmResetToolbar)
    ON_REGISTERED_MESSAGE(WM_RADNOTEPAD, &CMainFrame::OnRadNotepad)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DOCKINGWINDOWS, &CMainFrame::OnUpdateDockingWindows)
    ON_UPDATE_COMMAND_UI_RANGE(ID_MARGINS_1, ID_MARGINS_5, &CMainFrame::OnUpdateViewMargin)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
    m_PrevNext = -1;
}

CMainFrame::~CMainFrame()
{
    RemoveWindowSubclass(m_hWndMDIClient, MDIClientHookWndProc, 0);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;

    SetWindowSubclass(m_hWndMDIClient, MDIClientHookWndProc, 0, (DWORD_PTR) this);

    CMDITabInfo mdiTabParams;
    mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE; // other styles available...
    mdiTabParams.m_bActiveTabCloseButton = TRUE;      // set to FALSE to place close button at right of tab area
    mdiTabParams.m_bTabIcons = TRUE;    // set to TRUE to enable document icons on MDI taba
    mdiTabParams.m_bAutoColor = FALSE;    // set to FALSE to disable auto-coloring of MDI tabs
    mdiTabParams.m_bDocumentMenu = TRUE; // enable the document menu at the right edge of the tab area
    mdiTabParams.m_nTabBorderSize = 0;
    mdiTabParams.m_bTabCustomTooltips = TRUE;
    EnableMDITabbedGroups(TRUE, mdiTabParams);

    if (!m_wndMenuBar.Create(this))
    {
        TRACE0("Failed to create menubar\n");
        return -1;      // fail to create
    }

    m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

    // prevent the menu bar from taking the focus on activation
    CMFCPopupMenu::SetForceMenuFocus(FALSE);

    CMFCToolBarComboBoxButton::SetFlatMode(FALSE);

    EnableDocking(CBRS_ALIGN_ANY);
    m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndMenuBar);

    CString strCustomize;
    VERIFY(strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE));

    struct {
        UINT nID;
        UINT nStr;
    } tbs[] = { 
        { IDR_DOCKING, IDS_TOOLBAR_DOCKING },
        { IDR_VIEW, IDS_TOOLBAR_VIEW },
        { IDR_BOOKMARK, IDS_TOOLBAR_BOOKMARK },
        { IDR_SEARCH, IDS_TOOLBAR_SEARCH },
        { IDR_MAINFRAME, IDS_TOOLBAR_STANDARD },
    };
    static_assert(ARRAYSIZE(m_wndToolBar) == ARRAYSIZE(tbs), "Tololbar sizes must match");
    CMFCToolBar* pLastToolbar = nullptr;
    for (int i = 0; i < ARRAYSIZE(tbs); ++i)
    {
        CMFCToolBar& tb = m_wndToolBar[i];
        const UINT nID = tbs[i].nID;
        if (!tb.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(1, 1, 1, 1), nID) ||
            !tb.LoadToolBar(nID))
        {
            TRACE0("Failed to create toolbar\n");
            return -1;      // fail to create
        }

        ASSERT(static_cast<UINT>(tb.GetDlgCtrlID()) == nID);

        CString strToolBarName;
        VERIFY(strToolBarName.LoadString(tbs[i].nStr));
        tb.SetWindowText(strToolBarName);

        tb.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

        tb.EnableDocking(CBRS_ALIGN_ANY);
        if (pLastToolbar != nullptr)
            DockPaneLeftOf(&tb, pLastToolbar);
        else
            DockPane(&tb);

        pLastToolbar = &tb;
    }

    // Allow user-defined toolbars operations:
    InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

    if (!m_wndStatusBar.Create(this))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }
    const UINT indicators[] =
    {
        ID_SEPARATOR,           // status line indicator
        ID_INDICATOR_LINE,
        ID_INDICATOR_SCHEME,
        ID_INDICATOR_LINE_ENDING,
        ID_INDICATOR_OVR,
        ID_INDICATOR_CAPS,
        ID_INDICATOR_NUM,
        ID_INDICATOR_SCRL,
    };
    m_wndStatusBar.SetIndicators(indicators, ARRAYSIZE(indicators));
    m_wndStatusBar.EnablePaneDoubleClick(TRUE);
    m_wndStatusBar.SetPaneWidth(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SCHEME), 100);

    // enable Visual Studio 2005 style docking window behavior
    CDockingManager::SetDockingMode(DT_SMART);
    // enable Visual Studio 2005 style docking window auto-hide behavior
    EnableAutoHidePanes(CBRS_ALIGN_ANY);

    // Load menu item image (not placed on any standard toolbars):
    CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES);

    // create docking windows
    if (!CreateDockingWindows())
    {
        TRACE0("Failed to create docking windows\n");
        return -1;
    }

    // set the visual manager used to draw all user interface elements
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Enable enhanced windows management dialog
    EnableWindowsDialog(ID_WINDOW_MANAGER, ID_WINDOW_MANAGER, TRUE);

    // Enable toolbar and docking window menu replacement
    EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR, FALSE, TRUE);

    // enable quick (Alt+drag) toolbar customization
    CMFCToolBar::EnableQuickCustomization();

    if (CMFCToolBar::GetUserImages() == NULL)
    {
        // load user-defined toolbar images
        TCHAR szPath[_MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath)))
        {
            PathAppend(szPath, _T("RadSoft\\RadNotepad"));
            PathAppend(szPath, _T("UserImages.bmp"));
            if (!PathFileExists(szPath))
            {
                HMODULE hInstance = NULL;
                HRSRC hResInfo = ::FindResource(hInstance, MAKEINTRESOURCE(IDB_USER_IMAGES), RT_BITMAP);
                if (hResInfo != 0)
                {
                    HGLOBAL hRes = ::LoadResource(hInstance, hResInfo);
                    if (hRes != 0)
                    {
                        LPVOID memRes = ::LockResource(hRes);
                        DWORD sizeRes = ::SizeofResource(hInstance, hResInfo);

                        BITMAPFILEHEADER bfh = {};
                        bfh.bfType = 0x4D42; // BM
                        bfh.bfSize = sizeof(bfh) + sizeRes;
                        bfh.bfOffBits = sizeof(bfh) + sizeof(BITMAPINFOHEADER);

                        HANDLE hFile = ::CreateFile(szPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                        if (hFile != INVALID_HANDLE_VALUE)
                        {
                            DWORD dwWritten = 0;
                            ::WriteFile(hFile, &bfh, sizeof(bfh), &dwWritten, NULL);
                            ::WriteFile(hFile, memRes, sizeRes, &dwWritten, NULL);
                            ::CloseHandle(hFile);
                        }
                    }
                }
            }
            m_UserImages.SetImageSize(m_wndToolBar[0].GetImageSize());
            if (PathFileExists(szPath) && m_UserImages.Load(szPath))
            {
                CMFCToolBar::SetUserImages(&m_UserImages);
            }
        }
    }

    // Switch the order of document name and application name on the window title bar. This
    // improves the usability of the taskbar because the document name is visible with the thumbnail.
    ModifyStyle(0, FWS_PREFIXTITLE);

    DragAcceptFiles();

    return 0;
}

CMFCToolBarComboBoxButton* CMainFrame::GetHistoryButton() const
{
    const CMFCToolBar*       searchToolBar(GetToolbar(IDR_SEARCH));
    int i = searchToolBar->CommandToIndex(ID_SEARCH_TEXT);
    return i >= 0 ? dynamic_cast<CMFCToolBarComboBoxButton*>(searchToolBar->GetButton(i)) : nullptr;
}

void CMainFrame::SaveSearch(LPCTSTR lpszSectionName)
{
    CMFCToolBar* searchToolBar(GetToolbar(IDR_SEARCH));
    searchToolBar->SaveState(lpszSectionName);
}

INT_PTR CMainFrame::DoWindowsDialog()
{
    // Same as ShowWindowsDialog() but returns result
    CRadWindowsManagerDialog dlg(this, m_bShowWindowsDlgHelpButton);
    return dlg.DoModal() == IDOK;
}

BOOL CMainFrame::CreateDockingWindows()
{
    struct
    {
        CDockablePane& pane;
        const UINT nId;
        const UINT nTitle;
        const CRect& r;
        const UINT nFlags;
        const UINT nIcon;
    } views[] = {
        { m_wndFileView, ID_VIEW_FILEVIEW, IDS_FILE_VIEW, CRect(0, 0, 200, 200), CBRS_LEFT, IDI_FILE_VIEW},
        { m_wndOutput, ID_VIEW_OUTPUTWND, IDS_OUTPUT_WND, CRect(0, 0, 100, 100), CBRS_BOTTOM, IDI_OUTPUT_WND },
        { m_wndProperties, ID_VIEW_PROPERTIESWND, IDS_PROPERTIES_WND, CRect(0, 0, 200, 200), CBRS_RIGHT,IDI_PROPERTIES_WND },
        // { m_wndClassView, ID_VIEW_CLASSVIEW, IDS_CLASS_VIEW, CRect(0, 0, 200, 200), CBRS_LEFT, IDI_CLASS_VIEW },
    };
    for (auto i : views)
    {
        CString strTitle;
        VERIFY(strTitle.LoadString(i.nTitle));
        if (!i.pane.Create(strTitle, this, i.r, TRUE, i.nId, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | i.nFlags | CBRS_FLOAT_MULTI))
        {
            TRACE0("Failed to create pane window\n");
            return FALSE; // failed to create
        }

        HICON hIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(i.nIcon), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
        i.pane.SetIcon(hIcon, FALSE);

        i.pane.EnableDocking(CBRS_ALIGN_ANY);
        DockPane(&i.pane);
    }

    UpdateMDITabbedBarsIcons();
    return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CMDIFrameWndEx::Dump(dc);
}
#endif //_DEBUG

// CMainFrame message handlers

LRESULT CMainFrame::OnSetMessageString(WPARAM wParam, LPARAM lParam)
{
    static LONG Wait = 0; // TODO Make this a member variable ?

    bool bWarning = wParam == IDS_SEARCH_NOT_FOUND;
    LONG Now = GetMessageTime();
    if (bWarning || Now > Wait)
    {
        CMFCStatusBar* pStatusBar = DYNAMIC_DOWNCAST(CMFCStatusBar, GetMessageBar());
        if (pStatusBar)
        {
            COLORREF colText = bWarning ? GetSysColor(COLOR_MENUTEXT) : -1;
            COLORREF colBg = bWarning ? GetSysColor(COLOR_MENUHILIGHT) : -1;

            pStatusBar->SetPaneTextColor(0, colText, FALSE);
            pStatusBar->SetPaneBackgroundColor(0, colBg, FALSE);
        }
        if (bWarning)
            Wait = Now + 1 * 1000;

        return CMDIFrameWndEx::OnSetMessageString(wParam, lParam);
    }
    else
        return 0;
}

void CMainFrame::OnWindowManager()
{
    //ShowWindowsDialog();
    CRadWindowsManagerDialog dlg(this, m_bShowWindowsDlgHelpButton);
    dlg.DoModal();
}

void CMainFrame::OnViewCustomize()
{
    CMFCToolBarsCustomizeDialog* pDlgCust = new CRadToolBarsCustomizeDialog(this, TRUE /* scan menus */);

    pDlgCust->RemoveButton(_T("View"), ID_VIEW_DOCKINGWINDOWS);
    CObList lstBars;
    GetDockingManager()->GetPaneList(lstBars, TRUE, RUNTIME_CLASS(CDockablePane), TRUE);
    for (POSITION pos = lstBars.GetHeadPosition(); pos != NULL;)
    {
        CDockablePane* pPane = DYNAMIC_DOWNCAST(CDockablePane, lstBars.GetNext(pos));
        if (pPane != NULL)
        {
            CString title;
            pPane->GetPaneName(title);

            CMFCToolBarButton button(pPane->GetDlgCtrlID(), GetCmdMgr()->GetCmdImage(pPane->GetDlgCtrlID(), FALSE), title);
            pDlgCust->AddButton(_T("Docking Windows"), button);
        }
    }

    for (UINT nID = ID_MARGINS_1; nID <= ID_MARGINS_5; ++nID)
    {
        pDlgCust->RemoveButton(_T("View"), nID);

        const Theme* pTheme = &theApp.m_Settings.user;
        size_t i = nID - ID_MARGINS_1;
        const std::vector<Margin>& vecMargin = pTheme->vecMargin;
        if (i >= 0 && i < vecMargin.size())
        {
            const Margin& margin = vecMargin[i];
            CMFCToolBarButton button(nID, -1, margin.name);
            pDlgCust->AddButton(_T("View"), button);
        }
    }

    pDlgCust->EnableUserDefinedToolbars();
    pDlgCust->Create();
}

static CMenu* CheckSubMenu(CMenu* pMenu, UINT nPos, LPCTSTR check)
{
    UNREFERENCED_PARAMETER(check);
    CString s;
    pMenu->GetMenuString(nPos, s, MF_BYPOSITION);
    ASSERT(s == check);
    return pMenu->GetSubMenu(nPos);
}

void CMainFrame::OnSchemeIndicator()
{
    CMenu* pMenu = CMenu::FromHandle(GetMenuBar()->GetHMenu());
    pMenu = CheckSubMenu(pMenu, 2, _T("&View"));
    pMenu = CheckSubMenu(pMenu, 8, _T("Scheme"));

    CRect r;
    m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_SCHEME), &r);
    m_wndStatusBar.ClientToScreen(&r);

    pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, r.left, r.top, this);
}

void CMainFrame::OnLineEndingIndicator()
{
    CMenu* pMenu = CMenu::FromHandle(GetMenuBar()->GetHMenu());
    pMenu = CheckSubMenu(pMenu, 2, _T("&View"));
    pMenu = CheckSubMenu(pMenu, 5, _T("Line Endings"));

    CRect r;
    m_wndStatusBar.GetItemRect(m_wndStatusBar.CommandToIndex(ID_INDICATOR_LINE_ENDING), &r);
    m_wndStatusBar.ClientToScreen(&r);

    pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_BOTTOMALIGN, r.left, r.top, this);
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
    LRESULT lres = CMDIFrameWndEx::OnToolbarCreateNew(wp,lp);
    if (lres == 0)
    {
        return 0;
    }

    CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
    ASSERT_VALID(pUserToolbar);

    CString strCustomize;
    VERIFY(strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE));

    pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
    return lres;
}

void CMainFrame::OnViewPane(UINT nID)
{
    CBasePane* pBasePane = GetPane(nID);
    if (pBasePane->IsVisible())
        pBasePane->ShowPane(FALSE, FALSE, FALSE);
    else
    {
        pBasePane->ShowPane(TRUE, FALSE, TRUE);
        pBasePane->SetFocus();
    }
}

void CMainFrame::OnUpdateViewPane(CCmdUI* pCmdUI)
{
    CBasePane* pPane = GetPane(pCmdUI->m_nID);
    if (pPane != nullptr)
        pCmdUI->SetCheck(pPane->IsPaneVisible());
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
    // base class does the real work

    if (!CMDIFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
    {
        return FALSE;
    }

    m_wndProperties.InitLanguages();
    NotifySettingsChanged();

    if (afxUserToolsManager->GetUserTools().IsEmpty())
        CRadToolBarsCustomizeDialog::CreateDefaultTools();

    // enable customization button for all user toolbars
    CString strCustomize;
    VERIFY(strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE));

    for (int i = 0; i < iMaxUserToolbars; i ++)
    {
        CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
        if (pUserToolbar != NULL)
        {
            pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
        }
    }

    return TRUE;
}

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    CMDIFrameWndEx::OnSettingChange(uFlags, lpszSection);
    m_wndOutput.UpdateFonts();
}

void CMainFrame::OnUpdateClear(CCmdUI* pCmdUI)
{
    pCmdUI->SetText(_T(""));
    pCmdUI->Enable(FALSE);
}

afx_msg LRESULT CMainFrame::OnAfxWmOnGetTabTooltip(WPARAM /*wParam*/, LPARAM lParam)
{
    CMFCTabToolTipInfo* pInfo = (CMFCTabToolTipInfo*) lParam;
    CChildFrame* pChildFrame = DYNAMIC_DOWNCAST(CChildFrame, pInfo->m_pTabWnd->GetTabWnd(pInfo->m_nTabIndex));
    if (pChildFrame != nullptr)
        pInfo->m_strText = pChildFrame->GetActiveDocument()->GetPathName();
    return 0;
}

afx_msg LRESULT CMainFrame::OnAfxWmResetToolbar(WPARAM wParam, LPARAM /*lParam*/)
{
    if (wParam == IDR_SEARCH)
    {
        CMFCToolBar* searchToolBar(GetToolbar(IDR_SEARCH));

        CToolBarHistoryButton searchText(ID_SEARCH_TEXT, -1, CBS_DROPDOWN | CBS_AUTOHSCROLL, 200);
        searchText.EnableWindow();
        searchText.SetDropDownHeight(125);
        searchToolBar->ReplaceButton(searchText.m_nID, searchText);
    }
    return 0;
}

LRESULT CMainFrame::OnRadNotepad(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    return MSG_RADNOTEPAD;
}

static CString ExpandEnvironmentStrings(LPCTSTR str)
{
    CString ret;
    ExpandEnvironmentStrings(str, ret.GetBufferSetLength(MAX_PATH), MAX_PATH);
    ret.ReleaseBuffer();
    return ret;
}

void CMainFrame::OnUpdateDockingWindows(CCmdUI *pCmdUI)
{
    if (pCmdUI->m_pSubMenu != nullptr)
    {
        pCmdUI->m_pMenu->DeleteMenu(ID_VIEW_DOCKINGWINDOWS, MF_BYCOMMAND);

        CObList lstBars;
        GetDockingManager()->GetPaneList(lstBars, TRUE, RUNTIME_CLASS(CDockablePane), TRUE);
        for (POSITION pos = lstBars.GetHeadPosition(); pos != NULL;)
        {
            CDockablePane* pPane = DYNAMIC_DOWNCAST(CDockablePane, lstBars.GetNext(pos));
            if (pPane != NULL)
            {
                CString title;
                pPane->GetPaneName(title);
                pCmdUI->m_pSubMenu->AppendMenu(MF_STRING, pPane->GetDlgCtrlID(), title);
                pCmdUI->m_pSubMenu->CheckMenuItem(pPane->GetDlgCtrlID(), (pPane->IsVisible() ? MF_CHECKED : MF_UNCHECKED) |  MF_BYCOMMAND);
            }
        }
    }
}

void CMainFrame::OnUpdateViewMargin(CCmdUI *pCmdUI)
{
    const Theme* pTheme = &theApp.m_Settings.user;
    size_t i = pCmdUI->m_nID - ID_MARGINS_1;
    const std::vector<Margin>& vecMargin = pTheme->vecMargin;
    if (i >= 0 && i < vecMargin.size())
    {
        const Margin& margin = vecMargin[i];
        pCmdUI->SetText(margin.name);
    }
    else if (pCmdUI->m_pMenu != nullptr)
        pCmdUI->m_pMenu->RemoveMenu(pCmdUI->m_nID, MF_BYCOMMAND);
}

static void Erase(std::vector<CWnd*>& m_MDIStack, CWnd* pWndMDIChild)
{
    auto it = std::find(m_MDIStack.begin(), m_MDIStack.end(), pWndMDIChild);
    if (it != m_MDIStack.end())
        m_MDIStack.erase(it);
}

static void MoveToTop(std::vector<CWnd*>& m_MDIStack, CWnd* pWndMDIChild)
{
    Erase(m_MDIStack, pWndMDIChild);
    m_MDIStack.insert(m_MDIStack.begin(), pWndMDIChild);
}

void CMainFrame::ChildMDIActiviate(CWnd* pWndMDIChild)
{
    if (m_PrevNext == -1)
        MoveToTop(m_MDIStack, pWndMDIChild);
}

void CMainFrame::ChildMDIDestroyed(CWnd* pWndMDIChild)
{
    Erase(m_MDIStack, pWndMDIChild);
}

void CMainFrame::ChildMDINextWindow(CWnd* /*pWndMDIChild*/, BOOL bIsPrev)
{
    if (m_PrevNext < 0)
        m_PrevNext = 0;

    if (bIsPrev)
    {
        --m_PrevNext;
        if (m_PrevNext < 0)
            m_PrevNext = (int) m_MDIStack.size() - 1;
    }
    else
    {
        ++m_PrevNext;
        if (m_PrevNext >= (int) m_MDIStack.size())
            m_PrevNext = 0;
    }

    CWnd* pWnd = m_MDIStack[m_PrevNext];
    m_wndClientArea.SetActiveTab(pWnd->GetSafeHwnd());
}

void CMainFrame::NotifySettingsChanged()
{
    m_wndOutput.NotifySettingsChanged();
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_CONTROL)
    {
        CMDIChildWnd* pWnd = MDIGetActive();
        if (pWnd != nullptr)
            MoveToTop(m_MDIStack, pWnd);
        m_PrevNext = -1;
    }

    return CMDIFrameWndEx::PreTranslateMessage(pMsg);
}

CWnd* TabWndFromPoint(CMDIClientAreaWnd& wndClientArea, CPoint ptScreen)
{
    for (POSITION pos = wndClientArea.GetMDITabGroups().GetHeadPosition(); pos != NULL;)
    {
        CMFCTabCtrl* pNextTab = DYNAMIC_DOWNCAST(CMFCTabCtrl, wndClientArea.GetMDITabGroups().GetNext(pos));
        ASSERT_VALID(pNextTab);
        CRect rectWnd;
        pNextTab->GetWindowRect(rectWnd);
        if (rectWnd.PtInRect(ptScreen))
        {
            CPoint ptClient(ptScreen);
            pNextTab->ScreenToClient(&ptClient);
            int i = pNextTab->GetTabFromPoint(ptClient);
            if (i >= 0)
                return pNextTab->GetTabWnd(i);
            else
                return NULL;
        }
    }
    return NULL;
}

void CMainFrame::OnContextMenu(CWnd* pWnd, CPoint point)
{
    if (pWnd == &m_wndClientArea)
    {
        CWnd* pChildWnd = TabWndFromPoint(m_wndClientArea, point);
        if (pChildWnd != nullptr)
        {
            m_wndClientArea.SetActiveTab(pChildWnd->GetSafeHwnd());
            theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_TAB, point.x, point.y, this, TRUE);
        }
    }
}

BOOL CMainFrame::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (CRadWaitCursor::s_Count > 0)
        return SetCursor(LoadCursor(NULL, IDC_WAIT)), TRUE;
    else
        return CMDIFrameWndEx::OnSetCursor(pWnd, nHitTest, message);
}
