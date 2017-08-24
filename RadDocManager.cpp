#include "stdafx.h"
#include "RadDocManager.h"
#include "RadNotepad.h"
#include "MainFrm.h"


CRadDocManager::CRadDocManager()
{
}


CRadDocManager::~CRadDocManager()
{
}

CDocument* CRadDocManager::GetActiveDocument()
{
    CWnd* pWnd = AfxGetMainWnd();
    ASSERT_KINDOF(CMDIFrameWndEx, pWnd);
    CMDIFrameWndEx* pMainWnd = (CMDIFrameWndEx*) pWnd;
    CMDIChildWnd* pChild = pMainWnd->MDIGetActive();
    if (pChild != nullptr)
        return pChild->GetActiveDocument();
    else
        return nullptr;
}

void CRadDocManager::UpdateAllViews(CView* pSender, LPARAM lHint, CObject* pHint)
{
    POSITION posTemplate = GetFirstDocTemplatePosition();
    while (posTemplate != NULL)
    {
        CDocTemplate* pTemplate = (CDocTemplate*) GetNextDocTemplate(posTemplate);
        ASSERT_KINDOF(CDocTemplate, pTemplate);
        {
            POSITION pos = pTemplate->GetFirstDocPosition();
            while (pos != NULL)
            {
                CDocument* pDoc = pTemplate->GetNextDoc(pos);
                pDoc->UpdateAllViews(pSender, lHint, pHint);
            }
        }
    }
}

int CRadDocManager::GetModifiedDocumentCount() const
{
    int nModified = 0;
    POSITION posTemplate = GetFirstDocTemplatePosition();
    while (posTemplate != NULL)
    {
        CDocTemplate* pTemplate = (CDocTemplate*) GetNextDocTemplate(posTemplate);
        ASSERT_KINDOF(CDocTemplate, pTemplate);
        {
            POSITION pos = pTemplate->GetFirstDocPosition();
            while (pos != NULL)
            {
                CDocument* pDoc = pTemplate->GetNextDoc(pos);
                if (pDoc->IsModified())
                    ++nModified;
            }
        }
    }
    return nModified;
}

BOOL CRadDocManager::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* /*pTemplate*/)
{
    CString strDefaultExt = PathFindExtension(fileName);
    if (strDefaultExt.IsEmpty())
        strDefaultExt = _T("txt");

    CString strFilter;
#if 0
    CString strDefault;
    if (pTemplate != NULL)
    {
        ASSERT_VALID(pTemplate);
        _AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pTemplate, &strDefault);
    }
    else
    {
        // do for all doc template
        POSITION pos = m_templateList.GetHeadPosition();
        BOOL bFirst = TRUE;
        while (pos != NULL)
        {
            pTemplate = (CDocTemplate*) m_templateList.GetNext(pos);
            _AfxAppendFilterSuffix(strFilter, dlgFile.m_ofn, pTemplate,
                bFirst ? &strDefault : NULL);
            bFirst = FALSE;
        }
    }
#endif
    const Theme* pTheme = &theApp.m_Settings.editor.rTheme;
    for (auto el : pTheme->mapExtFilter)
    {
        const Language* pLanguage = GetLanguage(pTheme, el.first);
        if (pLanguage != nullptr)
            strFilter +=  pLanguage->title + _T('|') + el.second + _T("|");
    }

    // append the "*.*" all files filter
    CString allFilter;
    VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));
    strFilter += allFilter;
    strFilter += _T("|*.*|");

    int nFilterCount = 0;
    {
        int nFind = -1;
        while ((nFind = strFilter.Find(_T('|'), nFind + 1)) != -1)
            ++nFilterCount;
        nFilterCount /= 2;
    }

    CFileDialog dlgFile(bOpenFileDialog, strDefaultExt, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter);

    CString title;
    ENSURE(title.LoadString(nIDSTitle));

    dlgFile.m_ofn.Flags |= lFlags;
    dlgFile.m_ofn.nFilterIndex = nFilterCount;

    dlgFile.m_ofn.lpstrTitle = title;
    dlgFile.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);

    TCHAR Dir[MAX_PATH] = _T("");
    CDocument* pDoc = GetActiveDocument();
    if (pDoc != nullptr)
    {
        CString FileName = pDoc->GetPathName();
        StrCpy(Dir, FileName);
        PathRemoveFileSpec(Dir);
        dlgFile.m_ofn.lpstrInitialDir = Dir;
    }

    INT_PTR nResult = dlgFile.DoModal();
    fileName.ReleaseBuffer();
    return nResult == IDOK;
}

BOOL CRadDocManager::SaveAllModified()
{
    //return CDocManager::SaveAllModified();
    int nModified = GetModifiedDocumentCount();

    if (nModified == 1)
        return CDocManager::SaveAllModified();
    else if (nModified > 0)
    {
        // TODO Need a better dialog
        CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
        return pMainWnd->DoWindowsDialog() == IDOK;
    }
    else
        return TRUE;
}

void CRadDocManager::SaveAll()
{
    // Like CDocManager::SaveAllModified() but saves instead of querying user to save

    POSITION posTemplate = GetFirstDocTemplatePosition();
    while (posTemplate != NULL)
    {
        CDocTemplate* pTemplate = (CDocTemplate*) GetNextDocTemplate(posTemplate);
        ASSERT_KINDOF(CDocTemplate, pTemplate);
        {
            POSITION pos = pTemplate->GetFirstDocPosition();
            while (pos != NULL)
            {
                CDocument* pDoc = pTemplate->GetNextDoc(pos);
                if (pDoc->IsModified())
                    pDoc->DoFileSave();
            }
        }
    }
}
