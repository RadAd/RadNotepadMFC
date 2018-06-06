
#include "stdafx.h"
#include "ViewTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewTree

CViewTree::CViewTree()
{
}

CViewTree::~CViewTree()
{
}

BEGIN_MESSAGE_MAP(CViewTree, CTreeCtrl)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewTree message handlers

BOOL CViewTree::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	BOOL bRes = CTreeCtrl::OnNotify(wParam, lParam, pResult);

	NMHDR* pNMHDR = (NMHDR*)lParam;
	ASSERT(pNMHDR != NULL);

	if (pNMHDR && pNMHDR->code == TTN_SHOW && GetToolTips() != NULL)
	{
		GetToolTips()->SetWindowPos(&wndTop, -1, -1, -1, -1, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSIZE);
	}

	return bRes;
}

BOOL CViewTree::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYUP && GetEditControl() != NULL)
    {
        if (pMsg->wParam == VK_ESCAPE)
            //m_wndFileView.EndEditLabelNow(TRUE);
            PostMessage(TVM_ENDEDITLABELNOW, TRUE);
        else if (pMsg->wParam == VK_RETURN)
            //m_wndFileView.EndEditLabelNow(TRUE);
            PostMessage(TVM_ENDEDITLABELNOW, FALSE);
    }

    return CTreeCtrl::PreTranslateMessage(pMsg);
}
