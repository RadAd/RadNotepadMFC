
#include "stdafx.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "RadNotepad.h"
#include "RadNotepadDoc.h"
#include "Theme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// TODO
// Allow a choice of lexer - maybe use the tab name for the lexer
// Search for next/prev error

/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputWnd::COutputWnd()
{
}

COutputWnd::~COutputWnd()
{
}

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create tabs window:
	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 1))
	{
		TRACE0("Failed to create output tab window\n");
		return -1;      // fail to create
	}

	//UpdateFonts();

    if (Get(_T("Output"), TRUE) == nullptr)
    {
        TRACE0("Failed to create output windows\n");
        return -1;      // fail to create
    }

	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Tab control should cover the whole client area:
	m_wndTabs.SetWindowPos(NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&afxGlobalData.fontRegular);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i ++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, (int)dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void COutputWnd::UpdateFonts()
{
    for (int i = 0; i < m_wndTabs.GetTabsNum(); ++i)
    {
        CWnd* pWnd = m_wndTabs.GetTabWnd(i);
        pWnd->SetFont(&afxGlobalData.fontRegular);
    }
}

void COutputWnd::NotifySettingsChanged()
{
    for (int i = 0; i < m_wndTabs.GetTabsNum(); ++i)
    {
        COutputList* pWnd = DYNAMIC_DOWNCAST(COutputList, m_wndTabs.GetTabWnd(i));
        pWnd->NotifySettingsChanged();
    }
}

COutputList* COutputWnd::Get(LPCTSTR pOutput, BOOL bCreate)
{
    COutputList* pOutputList = nullptr;
    for (int i = 0; i < m_wndTabs.GetTabsNum() && pOutputList == nullptr; ++i)
    {
        CString strLabel;
        m_wndTabs.GetTabLabel(i, strLabel);
        if (strLabel.CompareNoCase(pOutput) == 0)
            pOutputList = DYNAMIC_DOWNCAST(COutputList, m_wndTabs.GetTabWnd(i));
    }
    if (bCreate && pOutputList == nullptr)
    {
        CRect rectDummy;
        rectDummy.SetRectEmpty();
        const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
        pOutputList = new COutputList();
        if (!pOutputList->Create(dwStyle, rectDummy, &m_wndTabs, 0))
        {
            TRACE0("Failed to create output windows\n");
            delete pOutputList;
            return nullptr;      // fail to create
        }
        m_wndTabs.AddTab(pOutputList, pOutput, (UINT) 0);
    }
    return pOutputList;
}

COutputList* COutputWnd::Reset(LPCTSTR pOutput, LPCTSTR pDirectory)
{
    COutputList* pOutputList = Get(pOutput, TRUE);
    int i = m_wndTabs.GetTabFromHwnd(pOutputList->GetSafeHwnd());
    m_wndTabs.SetActiveTab(i);
    pOutputList->SetFocus();
    pOutputList->SetDirectory(pDirectory);
    pOutputList->Clear();
    return pOutputList;
}

/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList()
{
}

COutputList::~COutputList()
{
}

void COutputList::NotifySettingsChanged()
{
    const Theme* pTheme = &theApp.m_Settings.user;
    const Language* pLanguage = GetLanguage(pTheme, _T("output"));
    Apply(*this, pLanguage, pTheme);
}

void COutputList::Clear()
{
    SetReadOnly(FALSE);
    SelectAll();
    CScintillaCtrl::Clear();
    SetReadOnly(TRUE);
}

void COutputList::AppendText(LPCSTR pText, int nLen)
{
    SetReadOnly(FALSE);
    bool bAtEnd = GetCurrentPos() == GetLength();
    CScintillaCtrl::AppendText(nLen, pText);
    if (bAtEnd)
        GotoPos(GetLength());
    SetReadOnly(TRUE);
}

void COutputList::AppendText(LPCWSTR pText, int nLen)
{
    SetReadOnly(FALSE);
    bool bAtEnd = GetCurrentPos() == GetLength();
    CScintillaCtrl::AppendText(nLen, pText);
    if (bAtEnd)
        GotoPos(GetLength());
    SetReadOnly(TRUE);
}

BEGIN_MESSAGE_MAP(COutputList, CListBox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
    ON_NOTIFY_REFLECT(SCN_HOTSPOTCLICK, OnHotSpotClick)
	ON_WM_WINDOWPOSCHANGING()
    ON_WM_CREATE()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// COutputList message handlers

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnEditCopy()
{
    Copy();
}

void COutputList::OnEditClear()
{
    Clear();
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != NULL && pParentBar != NULL)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}

void COutputList::OnHotSpotClick(NMHDR* pHdr, LRESULT* pResult)
{
    SCNotification* pSCNotification = reinterpret_cast<SCNotification*>(pHdr);
    int nLine = LineFromPosition(pSCNotification->position);
    CString strLine = GetLine(nLine);
    strLine.Trim();

    // TODO Better file, line number extraction
    // Look for format: {file}({line})
    int nFileEnd = strLine.Mid(2).FindOneOf(L"(:");
    if (nFileEnd >= 0)
    {
        nFileEnd += 2;

        CString strFile = strLine.Left(nFileEnd);
        strFile.Trim();

        TCHAR path[_MAX_PATH];
        PathCombine(path, m_strDirectory, strFile);

        int nFileLine = 0;
        CString strEnd = strLine[nFileEnd] == _T('(') ? _T(")") : _T(":");
        int nNumberEnd = strLine.Find(strEnd, nFileEnd + 1);
        if (nNumberEnd >= 0)
        {
            CString strLineNumber = strLine.Mid(nFileEnd + 1, nNumberEnd - nFileEnd - 1);
            nFileLine = _ttoi(strLineNumber);
        }

        Cancel();
        CRadNotepadDoc* pDoc = DYNAMIC_DOWNCAST(CRadNotepadDoc, theApp.OpenDocumentFile(path));
        if (pDoc != nullptr && nFileLine > 0)
        {
            CScintillaView* pScintillaView = pDoc->GetView();
            if (pScintillaView != nullptr)
            {
                pScintillaView->SetFocus();
                pScintillaView->GetCtrl().GotoLine(nFileLine - 1);
            }
        }
    }

    *pResult = 0;
}

int COutputList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScintillaCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    SetupDirectAccess();    // Should be in CScintillaCtrl::OnCreate

    const Theme* pTheme = &theApp.m_Settings.user;
    const Language* pLanguage = GetLanguage(pTheme, _T("output"));
    Apply(*this, pLanguage, pTheme);

    SetReadOnly(TRUE);
    SetHotspotActiveUnderline(TRUE);
    return 0;
}


BOOL COutputWnd::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYUP && pMsg->wParam == VK_ESCAPE)
    {
        CMDIFrameWndEx* pMainWnd = DYNAMIC_DOWNCAST(CMDIFrameWndEx, AfxGetMainWnd());
        CFrameWnd* pFrameWnd = pMainWnd->GetActiveFrame();
        if (pFrameWnd != nullptr)
        {
            CView* pView = pFrameWnd->GetActiveView();
            if (pView)
                pView->SetFocus();
        }
    }

    return CDockablePane::PreTranslateMessage(pMsg);
}
