#include "stdafx.h"
#include "RadDocManager.h"


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

BOOL CRadDocManager::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* /*pTemplate*/)
{
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
    strFilter += _T("CPP files|*.cpp;*.c;*.cc;*.h|");

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

    CFileDialog dlgFile(bOpenFileDialog, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, strFilter);

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
