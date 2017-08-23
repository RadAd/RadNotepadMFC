
// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "RadNotepad.h"

#include "ChildFrm.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
    ON_WM_SYSCOMMAND()
    ON_WM_MDIACTIVATE()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
	// TODO: add member initialization code here
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
#if 0
    return CMDIChildWndEx::OnCreateClient(lpcs, pContext);
#else
    BOOL bRet = m_wndSplitter.Create(this,
        2, 1,			// TODO: adjust the number of rows, columns
        CSize(10, 10),	// TODO: adjust the minimum pane size
        pContext);
    if (bRet)
        m_wndSplitter.SetScrollStyle(0);
    return bRet;
#endif
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs
	if( !CMDIChildWndEx::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWndEx::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWndEx::Dump(dc);
}
#endif //_DEBUG

// CChildFrame message handlers


void CChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
    if (GetIcon(FALSE) == NULL)
    {
        CDocument* pDoc = GetActiveDocument();

        SHFILEINFO fi = {};
        if (!pDoc->GetPathName().IsEmpty())
            SHGetFileInfo(pDoc->GetPathName(), 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        if (fi.hIcon == NULL)
            SHGetFileInfo(_T(".txt"), 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
        HICON hIcon = fi.hIcon;

        DestroyIcon(SetIcon(hIcon, FALSE));
    }
    CMDIChildWndEx::OnUpdateFrameTitle(bAddToTitle);
}

void CChildFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    // TODO Would have been better to subclass the MDIClient
    // and override WM_MDINEXT but it is already subclassed and
    // I couldn't work out how to sublass again
    switch (nID)
    {
    case SC_NEXTWINDOW:
        {
            CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
            pMainWnd->ChildMDINextWindow(this, FALSE);
        }
        break;

    case SC_PREVWINDOW:
        {
            CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
            pMainWnd->ChildMDINextWindow(this, TRUE);
        }
        break;

    default:
        CMDIChildWndEx::OnSysCommand(nID, lParam);
        break;
    }
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
    CMDIChildWndEx::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

    if (bActivate)
    {
        CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
        pMainWnd->ChildMDIActiviate(this);
    }
}

void CChildFrame::OnDestroy()
{
    CMDIChildWndEx::OnDestroy();

    CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainWnd->ChildMDIDesrtoyed(this);
}
