// SaveModifiedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RadNotepad.h"
#include "SaveModifiedDlg.h"
#include "afxdialogex.h"

// TODO Also draw icon

// CSaveModifiedDlg dialog

IMPLEMENT_DYNAMIC(CSaveModifiedDlg, CDialogEx)

CSaveModifiedDlg::CSaveModifiedDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SAVE_MODIFIED, pParent)
{

}

CSaveModifiedDlg::~CSaveModifiedDlg()
{
}

void CSaveModifiedDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST1, m_List);
}


BEGIN_MESSAGE_MAP(CSaveModifiedDlg, CDialogEx)
    ON_BN_CLICKED(IDNO, &CSaveModifiedDlg::OnBnClickedNo)
END_MESSAGE_MAP()


// CSaveModifiedDlg message handlers


BOOL CSaveModifiedDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    CDocManager* pDocManager = theApp.m_pDocManager;
    POSITION posTemplate = pDocManager->GetFirstDocTemplatePosition();
    while (posTemplate != NULL)
    {
        CDocTemplate* pTemplate = (CDocTemplate*) pDocManager->GetNextDocTemplate(posTemplate);
        ASSERT_KINDOF(CDocTemplate, pTemplate);
        {
            POSITION pos = pTemplate->GetFirstDocPosition();
            while (pos != NULL)
            {
                CDocument* pDoc = pTemplate->GetNextDoc(pos);
                if (pDoc->IsModified())
                {
                    int i = m_List.AddString(pDoc->GetTitle());
                    m_List.SetItemDataPtr(i, pDoc);
                    m_List.SetCheck(i, BST_CHECKED);
                }
            }
        }
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CSaveModifiedDlg::OnBnClickedNo()
{
    EndDialog(IDNO);
}


void CSaveModifiedDlg::OnOK()
{
    BOOL bSaved = TRUE;
    for (int i = 0; bSaved && i < m_List.GetCount(); ++i)
    {
        if (m_List.GetCheck(i))
        {
            CDocument* pDoc = (CDocument*) m_List.GetItemDataPtr(i);
            bSaved = pDoc->DoFileSave();
        }
    }
    if (bSaved)
        CDialogEx::OnOK();
}
