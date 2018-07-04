#include "stdafx.h"
#include "RadToolBarsCustomizeDialog.h"

#include "RadUserTool.h"

#define ID_CAPTURE  500
#define ID_TAB      501
#define ID_SAVE     502

// TODO
// Doesn't save current tool changes when closing
//   Need to override CMFCToolBarsToolsPropertyPage::UpdateTool
//   may need to replace CMFCToolBarsToolsPropertyPage

void CRadToolBarsCustomizeDialog::CreateDefaultTools()
{
    CRadUserTool* pUserTool;

    pUserTool = (CRadUserTool*) afxUserToolsManager->CreateNewTool();
    pUserTool->m_strLabel = _T("Run");
    pUserTool->SetCommand(_T("{file}"));
    pUserTool->m_strInitialDirectory = _T("{path}");

    pUserTool = (CRadUserTool*) afxUserToolsManager->CreateNewTool();
    pUserTool->m_strLabel = _T("CMD");
    pUserTool->SetCommand(_T("cmd.exe"));
    pUserTool->m_strInitialDirectory = _T("{path}");

    pUserTool = (CRadUserTool*) afxUserToolsManager->CreateNewTool();
    pUserTool->m_strLabel = _T("Explorer");
    pUserTool->SetCommand(_T("explorer.exe"));
    pUserTool->m_strArguments = _T("/select,{file}");

    pUserTool = (CRadUserTool*) afxUserToolsManager->CreateNewTool();
    pUserTool->m_strLabel = _T("Google");
    pUserTool->SetCommand(_T("https://www.google.com.au/search?q={selected}"));

    pUserTool = (CRadUserTool*) afxUserToolsManager->CreateNewTool();
    pUserTool->m_strLabel = _T("Find in Files");
    pUserTool->SetCommand(_T("findstr.exe"));
    pUserTool->m_strArguments = _T("/S /N /P /C:\"{selected}\" *.*");
    pUserTool->m_strInitialDirectory = _T("{path}");
    pUserTool->m_bCapture = true;
    pUserTool->m_strTab = _T("Find");
}

void CRadToolBarsCustomizeDialog::OnInitToolsPage()
{
    if (m_pToolsPage != nullptr)
    {
        CFont* pFont = m_pToolsPage->GetFont();

        CRect rCapture(8, 131, 8 + 70, 131 + 8);
        m_pToolsPage->MapDialogRect(&rCapture);
        btnCapture.CreateEx(0, WC_BUTTON, _T("Capture output"), BS_CHECKBOX | BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, rCapture, m_pToolsPage, ID_CAPTURE);
        btnCapture.SetFont(pFont);

        CRect rTab(80, 129, 80 + 167, 129 + 14);
        m_pToolsPage->MapDialogRect(&rTab);
        editTab.CreateEx(0, WC_EDIT, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE, rTab, m_pToolsPage, ID_TAB);
        editTab.SetFont(pFont);

        CRect rSave(8, 146, 8 + 100, 146 + 8);
        m_pToolsPage->MapDialogRect(&rSave);
        btnSave.CreateEx(0, WC_BUTTON, _T("Save before executing"), BS_CHECKBOX | BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, rSave, m_pToolsPage, ID_SAVE);
        btnSave.SetFont(pFont);
    }
}

void CRadToolBarsCustomizeDialog::OnBeforeChangeTool(CUserTool* pSelTool)
{
    CRadUserTool* pRadSelTool = dynamic_cast<CRadUserTool*>(pSelTool);
    if (pRadSelTool != nullptr)
    {
        pRadSelTool->m_bCapture = btnCapture.GetCheck() == BST_CHECKED;
        editTab.GetWindowText(pRadSelTool->m_strTab);
        pRadSelTool->m_bSave = btnSave.GetCheck() == BST_CHECKED;
    }
}

void CRadToolBarsCustomizeDialog::OnAfterChangeTool(CUserTool* pSelTool)
{
    CRadUserTool* pRadSelTool = dynamic_cast<CRadUserTool*>(pSelTool);
    btnCapture.EnableWindow(pRadSelTool != nullptr);
    btnCapture.SetCheck(pRadSelTool != nullptr && pRadSelTool->m_bCapture ? BST_CHECKED : BST_UNCHECKED);
    editTab.EnableWindow(pRadSelTool != nullptr);
    editTab.SetWindowText(pRadSelTool != nullptr ? pRadSelTool->m_strTab : _T(""));
    btnSave.EnableWindow(pRadSelTool != nullptr);
    btnSave.SetCheck(pRadSelTool != nullptr && pRadSelTool->m_bSave ? BST_CHECKED : BST_UNCHECKED);
}
