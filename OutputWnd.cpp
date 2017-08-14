
#include "stdafx.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"
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

	if (!m_wndOutputBuild.Create(dwStyle, rectDummy, &m_wndTabs, 2) ||
		!m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, 3) ||
		!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4))
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}

	UpdateFonts();

	CString strTabName;
	BOOL bNameValid;

	// Attach list windows to tab:
	bNameValid = strTabName.LoadString(IDS_BUILD_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputBuild, strTabName, (UINT)0);
	bNameValid = strTabName.LoadString(IDS_DEBUG_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, (UINT)1);
	bNameValid = strTabName.LoadString(IDS_FIND_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)2);

	// Fill output tabs with some dummy text (nothing magic here)
	//FillBuildWindow();
	FillDebugWindow();
	FillFindWindow();

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

void COutputWnd::FillBuildWindow()
{
	m_wndOutputBuild.AppendText(-1, _T("Build output is being displayed here.\n"));
	m_wndOutputBuild.AppendText(-1, _T("The output is being displayed in rows of a list view\n"));
	m_wndOutputBuild.AppendText(-1, _T("but you can change the way it is displayed as you wish...\n"));
}

void COutputWnd::FillDebugWindow()
{
	m_wndOutputDebug.AppendText(-1, _T("Debug output is being displayed here.\n"));
	m_wndOutputDebug.AppendText(-1, _T("The output is being displayed in rows of a list view\n"));
	m_wndOutputDebug.AppendText(-1, _T("but you can change the way it is displayed as you wish...\n"));
}

void COutputWnd::FillFindWindow()
{
	m_wndOutputFind.AppendText(-1, _T("Find output is being displayed here.\n"));
	m_wndOutputFind.AppendText(-1, _T("The output is being displayed in rows of a list view\n"));
	m_wndOutputFind.AppendText(-1, _T("but you can change the way it is displayed as you wish...\n"));
}

void COutputWnd::UpdateFonts()
{
	m_wndOutputBuild.SetFont(&afxGlobalData.fontRegular);
	m_wndOutputDebug.SetFont(&afxGlobalData.fontRegular);
	m_wndOutputFind.SetFont(&afxGlobalData.fontRegular);
}

void COutputWnd::AppendText(LPCSTR pText, int nLen)
{
    m_wndOutputBuild.AppendText(nLen, pText);
}

void COutputWnd::AppendText(LPCWSTR pText, int nLen)
{
    m_wndOutputBuild.AppendText(nLen, pText);
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
    SelectAll();
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
    // TODO Extract file name and line number
    CString strFile = strLine.Left(strLine.Find(L"("));
    strFile.Trim();
    // TODO Combine with directory of process
    // TODO Open to file and line
    AfxMessageBox(strFile);
}

int COutputList::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScintillaCtrl::OnCreate(lpCreateStruct) == -1)
        return -1;

    SetupDirectAccess();    // Should be in CScintillaCtrl::OnCreate
    SetLexer(SCLEX_ERRORLIST);
    StyleSetFore(SCE_ERR_MS, RGB(255, 0, 0));
    StyleSetHotSpot(SCE_ERR_MS, TRUE);
    SetHotspotActiveUnderline(TRUE);

#if 0   // TODO Set error theme
#define SCE_ERR_DEFAULT 0
#define SCE_ERR_PYTHON 1
#define SCE_ERR_GCC 2
#define SCE_ERR_MS 3
#define SCE_ERR_CMD 4
#define SCE_ERR_BORLAND 5
#define SCE_ERR_PERL 6
#define SCE_ERR_NET 7
#define SCE_ERR_LUA 8
#define SCE_ERR_CTAG 9
#define SCE_ERR_DIFF_CHANGED 10
#define SCE_ERR_DIFF_ADDITION 11
#define SCE_ERR_DIFF_DELETION 12
#define SCE_ERR_DIFF_MESSAGE 13
#define SCE_ERR_PHP 14
#define SCE_ERR_ELF 15
#define SCE_ERR_IFC 16
#define SCE_ERR_IFORT 17
#define SCE_ERR_ABSF 18
#define SCE_ERR_TIDY 19
#define SCE_ERR_JAVA_STACK 20
#define SCE_ERR_VALUE 21
#define SCE_ERR_GCC_INCLUDED_FROM 22
#define SCE_ERR_ESCSEQ 23
#define SCE_ERR_ESCSEQ_UNKNOWN 24
#endif
    return 0;
}
