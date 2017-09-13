#include "stdafx.h"
#include "ChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
END_MESSAGE_MAP()

// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
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
        2, 1,
        CSize(10, 10),
        pContext);
    if (bRet)
        m_wndSplitter.SetScrollStyle(0);
    return bRet;
#endif
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
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
