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
    ON_COMMAND_RANGE(ID_VIEW_OUTPUTWND, ID_VIEW_PROPERTIESWND, &CMainFrame::OnViewPane)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, &CMainFrame::OnUpdateClear)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, &CMainFrame::OnUpdateClear)
    ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
    ON_REGISTERED_MESSAGE(AFX_WM_ON_GET_TAB_TOOLTIP, &CMainFrame::OnAfxWmOnGetTabTooltip)
    ON_REGISTERED_MESSAGE(AFX_WM_RESETTOOLBAR, &CMainFrame::OnAfxWmResetToolbar)
    ON_REGISTERED_MESSAGE(WM_RADNOTEPAD, &CMainFrame::OnRadNotepad)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DOCKINGWINDOWS, &CMainFrame::OnUpdateDockingWindows)
    ON_UPDATE_COMMAND_UI_RANGE(ID_MARGINS_1, ID_MARGINS_5, &CMainFrame::OnUpdateViewMargin)
    ON_WM_SETCURSOR()
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_LINE,
    ID_INDICATOR_OVR,
    ID_INDICATOR_CAPS,
    ID_INDICATOR_NUM,
    ID_INDICATOR_SCRL,
};

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

    BOOL bNameValid;

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

    if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    CString strToolBarName;
    bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
    ASSERT(bNameValid);
    m_wndToolBar.SetWindowText(strToolBarName);

    CString strCustomize;
    bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
    ASSERT(bNameValid);
    m_wndToolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

    CMFCToolBarComboBoxButton::SetFlatMode(FALSE);

    if (!m_searchToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(1, 1, 1, 1), IDR_SEARCH) ||
        !m_searchToolBar.LoadToolBar(IDR_SEARCH))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    //CString strToolBarName;
    bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_SEARCH);
    ASSERT(bNameValid);
    m_searchToolBar.SetWindowText(strToolBarName);

    if (!m_dockToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(1, 1, 1, 1), IDR_DOCKING) ||
        !m_dockToolBar.LoadToolBar(IDR_DOCKING))
    {
        TRACE0("Failed to create toolbar\n");
        return -1;      // fail to create
    }

    //UINT dockButtons[] = { ID_VIEW_FILEVIEW, ID_VIEW_PROPERTIESWND, ID_VIEW_OUTPUTWND };
    //m_dockToolBar.SetButtons(dockButtons, ARRAYSIZE(dockButtons));

    //CString strToolBarName;
    bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_DOCKING);
    ASSERT(bNameValid);
    m_dockToolBar.SetWindowText(strToolBarName);

    // Allow user-defined toolbars operations:
    InitUserToolbars(NULL, uiFirstUserToolBarId, uiLastUserToolBarId);

    if (!m_wndStatusBar.Create(this))
    {
        TRACE0("Failed to create status bar\n");
        return -1;      // fail to create
    }
    m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT));

    m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
    m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
    m_searchToolBar.EnableDocking(CBRS_ALIGN_ANY);
    m_dockToolBar.EnableDocking(CBRS_ALIGN_ANY);
    EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndMenuBar);
    DockPane(&m_wndToolBar);
    DockPane(&m_searchToolBar);
    DockPane(&m_dockToolBar);

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

    m_wndFileView.EnableDocking(CBRS_ALIGN_ANY);
    //m_wndClassView.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndFileView);
    //CDockablePane* pTabbedBar = NULL;
    //m_wndClassView.AttachToTabWnd(&m_wndFileView, DM_SHOW, TRUE, &pTabbedBar);
    m_wndOutput.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndOutput);
    m_wndProperties.EnableDocking(CBRS_ALIGN_ANY);
    DockPane(&m_wndProperties);

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
                HGLOBAL hRes = ::LoadResource(hInstance, hResInfo);
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
            m_UserImages.SetImageSize(m_wndToolBar.GetImageSize());
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
    int i = m_searchToolBar.CommandToIndex(ID_SEARCH_TEXT);
    return i >= 0 ? dynamic_cast<CMFCToolBarComboBoxButton*>(m_searchToolBar.GetButton(i)) : nullptr;
}

void CMainFrame::SaveSearch(LPCTSTR lpszSectionName)
{
    m_searchToolBar.SaveState(lpszSectionName);
}

INT_PTR CMainFrame::DoWindowsDialog()
{
    // Same as ShowWindowsDialog() but returns result
    CRadWindowsManagerDialog dlg(this, m_bShowWindowsDlgHelpButton);
    return dlg.DoModal() == IDOK;
}

BOOL CMainFrame::CreateDockingWindows()
{
    BOOL bNameValid;

#if 0
    // Create class view
    CString strClassView;
    bNameValid = strClassView.LoadString(IDS_CLASS_VIEW);
    ASSERT(bNameValid);
    if (!m_wndClassView.Create(strClassView, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_CLASSVIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
    {
        TRACE0("Failed to create Class View window\n");
        return FALSE; // failed to create
    }
#endif

    // Create file view
    CString strFileView;
    bNameValid = strFileView.LoadString(IDS_FILE_VIEW);
    ASSERT(bNameValid);
    if (!m_wndFileView.Create(strFileView, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_FILEVIEW, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI))
    {
        TRACE0("Failed to create File View window\n");
        return FALSE; // failed to create
    }

    // Create output window
    CString strOutputWnd;
    bNameValid = strOutputWnd.LoadString(IDS_OUTPUT_WND);
    ASSERT(bNameValid);
    if (!m_wndOutput.Create(strOutputWnd, this, CRect(0, 0, 100, 100), TRUE, ID_VIEW_OUTPUTWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI))
    {
        TRACE0("Failed to create Output window\n");
        return FALSE; // failed to create
    }

    // Create properties window
    CString strPropertiesWnd;
    bNameValid = strPropertiesWnd.LoadString(IDS_PROPERTIES_WND);
    ASSERT(bNameValid);
    if (!m_wndProperties.Create(strPropertiesWnd, this, CRect(0, 0, 200, 200), TRUE, ID_VIEW_PROPERTIESWND, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
    {
        TRACE0("Failed to create Properties window\n");
        return FALSE; // failed to create
    }

    SetDockingWindowIcons();
    return TRUE;
}

void CMainFrame::SetDockingWindowIcons()
{
    HICON hFileViewIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_FILE_VIEW), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndFileView.SetIcon(hFileViewIcon, FALSE);

#if 0
    HICON hClassViewIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CLASS_VIEW), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndClassView.SetIcon(hClassViewIcon, FALSE);
#endif

    HICON hOutputBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_OUTPUT_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndOutput.SetIcon(hOutputBarIcon, FALSE);

    HICON hPropertiesBarIcon = (HICON) ::LoadImage(::AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_PROPERTIES_WND), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
    m_wndProperties.SetIcon(hPropertiesBarIcon, FALSE);

    UpdateMDITabbedBarsIcons();
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

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
    LRESULT lres = CMDIFrameWndEx::OnToolbarCreateNew(wp,lp);
    if (lres == 0)
    {
        return 0;
    }

    CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
    ASSERT_VALID(pUserToolbar);

    BOOL bNameValid;
    CString strCustomize;
    bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
    ASSERT(bNameValid);

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
    BOOL bNameValid;
    CString strCustomize;
    bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
    ASSERT(bNameValid);

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
        CToolBarHistoryButton searchText(ID_SEARCH_TEXT, -1, CBS_DROPDOWN | CBS_AUTOHSCROLL, 200);
        searchText.EnableWindow();
        searchText.SetDropDownHeight(125);
        m_searchToolBar.ReplaceButton(searchText.m_nID, searchText);
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
