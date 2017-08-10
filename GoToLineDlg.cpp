// GoToLineDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RadNotepad.h"
#include "GoToLineDlg.h"
#include "afxdialogex.h"


// CGoToLineDlg dialog

IMPLEMENT_DYNAMIC(CGoToLineDlg, CDialogEx)

CGoToLineDlg::CGoToLineDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_GOTOLINE, pParent)
    , m_nLine(0)
    , m_nMaxLine(0)
{

}

CGoToLineDlg::~CGoToLineDlg()
{
}

void CGoToLineDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_LINE, m_nLine);
    if (m_nMaxLine > 0)
        DDV_MinMaxInt(pDX, m_nLine, 1, m_nMaxLine);
}


BEGIN_MESSAGE_MAP(CGoToLineDlg, CDialogEx)
END_MESSAGE_MAP()


// CGoToLineDlg message handlers
