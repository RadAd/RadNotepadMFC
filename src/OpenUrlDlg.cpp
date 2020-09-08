// OpenUrl.cpp : implementation file
//

#include "stdafx.h"
#include "OpenUrlDlg.h"
#include "afxdialogex.h"
#include "resource.h"


// OpenUrl dialog

IMPLEMENT_DYNAMIC(COpenUrlDlg, CDialogEx)

COpenUrlDlg::COpenUrlDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_OPEN_URL, pParent)
	, m_strUrl(_T(""))
{

}

COpenUrlDlg::~COpenUrlDlg()
{
}

void COpenUrlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_URL, m_strUrl);
}


BEGIN_MESSAGE_MAP(COpenUrlDlg, CDialogEx)
END_MESSAGE_MAP()


// OpenUrl message handlers
