// GoToLineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GoToLineDlg.h"

#include "..\resource.h"

// CGoToLineDlg dialog

IMPLEMENT_DYNAMIC(CGoToLineDlg, CRadDialog)

CGoToLineDlg::CGoToLineDlg(CWnd* pParent /*=NULL*/)
    : CRadDialog(IDD_GOTOLINE, pParent)
    , m_nLine(0)
    , m_nMaxLine(0)
{
}

void CGoToLineDlg::DoDataExchange(CDataExchange* pDX)
{
    CRadDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_LINE, m_nLine);
    if (m_nMaxLine > 0)
        DDV_MinMaxInt(pDX, static_cast<int>(m_nLine), 1, static_cast<int>(m_nMaxLine));
}

BEGIN_MESSAGE_MAP(CGoToLineDlg, CRadDialog)
END_MESSAGE_MAP()

// CGoToLineDlg message handlers
