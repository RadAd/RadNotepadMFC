
#include "stdafx.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "RadNotepad.h"
#include "Theme.h"
#include <SciLexer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

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

	// Create output panes:
	const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;

    OutputWindowE vecOutputWindowE[] = { OW_OUTPUT, OW_LOG };
    int vecOWName[] = { IDS_BUILD_TAB, IDS_DEBUG_TAB, IDS_FIND_TAB };

    for (OutputWindowE ow : vecOutputWindowE)
    {
        if (!m_wndOutput[ow].Create(dwStyle, rectDummy, &m_wndTabs, 2 + ow))
        {
            TRACE0("Failed to create output windows\n");
            return -1;      // fail to create
        }
    }

	//UpdateFonts();

	// Attach list windows to tab:
    for (OutputWindowE ow : vecOutputWindowE)
    {
        CString strTabName;
        BOOL bNameValid = strTabName.LoadString(vecOWName[ow]);
        ASSERT(bNameValid);
        m_wndTabs.AddTab(&m_wndOutput[ow], strTabName, (UINT) 0);
    }

	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Tab control should cover the whole client area:
	m_wndTabs.SetWindowPos (NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
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
    OutputWindowE vecOutputWindowE[] = { OW_OUTPUT, OW_LOG };
    for (OutputWindowE ow : vecOutputWindowE)
        m_wndOutput[ow].SetFont(&afxGlobalData.fontRegular);
}

void COutputWnd::Clear(OutputWindowE ow)
{
    COutputList& wndOutput = m_wndOutput[ow];
    wndOutput.SetReadOnly(FALSE);
    wndOutput.SelectAll();
    wndOutput.Clear();
    wndOutput.SetReadOnly(TRUE);
}

void COutputWnd::AppendText(OutputWindowE ow, LPCSTR pText, int nLen)
{
    COutputList& wndOutput = m_wndOutput[ow];
    wndOutput.SetReadOnly(FALSE);
    wndOutput.AppendText(nLen, pText);
    wndOutput.SetReadOnly(TRUE);
}

void COutputWnd::AppendText(OutputWindowE ow, LPCWSTR pText, int nLen)
{
    COutputList& wndOutput = m_wndOutput[ow];
    wndOutput.SetReadOnly(FALSE);
    wndOutput.AppendText(nLen, pText);
    wndOutput.SetReadOnly(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList()
{
}

COutputList::~COutputList()
{
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
    SetReadOnly(FALSE);
    SelectAll();
    Clear();
    SetReadOnly(TRUE);
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
    // TODO Extract file name and line number
    CString strFile = strLine.Left(strLine.Find(L"("));
    strFile.Trim();
    // TODO Combine with directory of process
    // TODO Open to file and line
    AfxMessageBox(strFile);
    *pResult = 0;
}

int COutputList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScintillaCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    SetupDirectAccess();    // Should be in CScintillaCtrl::OnCreate

    Theme* pTheme = &theApp.m_Settings.editor.rTheme;
    const Language* pLanguage = GetLanguage(pTheme, _T("output"));
    Apply(*this, pLanguage, pTheme);

    SetReadOnly(TRUE);
    SetHotspotActiveUnderline(TRUE);
    return 0;
}
