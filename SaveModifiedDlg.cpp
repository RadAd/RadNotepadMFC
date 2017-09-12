// SaveModifiedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RadNotepad.h"
#include "SaveModifiedDlg.h"
#include "afxdialogex.h"

// TODO Give dialog an icon

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
    ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


// CSaveModifiedDlg message handlers


BOOL CSaveModifiedDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    GetWindowRect(&m_OrigWndRect);

    HIMAGELIST ImlLarge, ImlSmall;
    Shell_GetImageLists(&ImlLarge, &ImlSmall);
    CImageList il;
    il.Attach(ImlSmall);

    //m_List.SetView(LV_VIEW_DETAILS);
    CRect r;
    m_List.GetWindowRect(&r);
    m_List.SetImageList(&il, LVSIL_SMALL);
    m_List.InsertColumn(0, _T("Name"));
    m_List.InsertColumn(1, _T("Path"));
    m_List.SetExtendedStyle(m_List.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

    il.Detach();

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
                    SHFILEINFO fi = {};
                    if (!pDoc->GetPathName().IsEmpty())
                        SHGetFileInfo(pDoc->GetPathName(), 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
                    if (fi.hIcon == NULL)
                        SHGetFileInfo(_T(".txt"), 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
                    DestroyIcon(fi.hIcon);

                    int i = m_List.InsertItem(m_List.GetItemCount(), pDoc->GetTitle(), fi.iIcon);
                    m_List.SetItemData(i, (DWORD_PTR) pDoc);
                    m_List.SetItemText(i, 1, pDoc->GetPathName());
                    m_List.SetCheck(i);
                }
            }
        }
    }

    m_List.SetColumnWidth(0, LVSCW_AUTOSIZE);
    m_List.SetColumnWidth(1, LVSCW_AUTOSIZE);

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
    for (int i = 0; bSaved && i < m_List.GetItemCount(); ++i)
    {
        if (m_List.GetCheck(i))
        {
            CDocument* pDoc = (CDocument*) m_List.GetItemData(i);
            bSaved = pDoc->DoFileSave();
        }
    }
    if (bSaved)
        CDialogEx::OnOK();
}

void CSaveModifiedDlg::OnGetMinMaxInfo(MINMAXINFO* pMinMaxInfo)
{
    pMinMaxInfo->ptMinTrackSize.x = m_OrigWndRect.Width();
    pMinMaxInfo->ptMinTrackSize.y = m_OrigWndRect.Height();
}
