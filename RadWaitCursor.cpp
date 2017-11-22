#include "stdafx.h"
#include "RadWaitCursor.h"

CRadWaitCursor::CRadWaitCursor()
{
    m_pMainWnd = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd());
    m_pMainWnd->EnableWindow(FALSE);
    ++s_Count;
}

CRadWaitCursor::~CRadWaitCursor()
{
    m_pMainWnd->EnableWindow(TRUE);
    --s_Count;
}

CMFCStatusBar* CRadWaitCursor::GetStatusBar()
{
    return DYNAMIC_DOWNCAST(CMFCStatusBar, m_pMainWnd->GetMessageBar());
}

int CRadWaitCursor::s_Count = 0;
