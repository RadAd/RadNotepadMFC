#include "stdafx.h"
#include "RadDocManager.h"
#include "RadNotepad.h"
#include "SaveModifiedDlg.h"

CRadDocManager::CRadDocManager()
{
}

CRadDocManager::~CRadDocManager()
{
}

CDocument* CRadDocManager::GetActiveDocument()
{
    CMDIFrameWndEx* pMainWnd = DYNAMIC_DOWNCAST(CMDIFrameWndEx, AfxGetMainWnd());
    CMDIChildWnd* pChild = pMainWnd->MDIGetActive();
    if (pChild != nullptr)
        return pChild->GetActiveDocument();
    else
        return nullptr;
}

CView* CRadDocManager::GetActiveView()
{
    CMDIFrameWndEx* pMainWnd = DYNAMIC_DOWNCAST(CMDIFrameWndEx, AfxGetMainWnd());
    CMDIChildWnd* pChild = pMainWnd->MDIGetActive();
    if (pChild != nullptr)
        return pChild->GetActiveView();
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
                if (!pDoc->GetPathName().IsEmpty() && pDoc->IsModified())
                    ++nModified;
            }
        }
    }
    return nModified;
}

BOOL CRadDocManager::DoPromptFileName(CString& fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* /*pTemplate*/)
{
    CString strDefaultName;
    ENSURE(strDefaultName.LoadString(AFX_IDS_UNTITLED));
    strDefaultName += '*';
    if (fileName == strDefaultName)
        fileName.Empty();

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
    const Theme* pTheme = &theApp.m_Settings.user;
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

    CFileDialog dlgFile(bOpenFileDialog, strDefaultExt, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT /* | OFN_CREATEPROMPT*/, strFilter);

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

CDocument* CRadDocManager::OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU)
{
    if (lpszFileName == NULL)
    {
        AfxThrowInvalidArgException();
    }

    if (!CRadNotepadApp::IsInternetUrl(lpszFileName))
        return CDocManager::OpenDocumentFile(lpszFileName, bAddToMRU);
    else
    {
        // find the highest confidence
        POSITION pos = m_templateList.GetHeadPosition();
        CDocTemplate::Confidence bestMatch = CDocTemplate::noAttempt;
        CDocTemplate* pBestTemplate = NULL;
        CDocument* pOpenDocument = NULL;

        TCHAR szPath[_MAX_PATH];
        ASSERT(AtlStrLen(lpszFileName) < _countof(szPath));
#if 0   // This is why we can't just call CDocManager::OpenDocumentFile
        TCHAR szTemp[_MAX_PATH];
        if (lpszFileName[0] == '\"')
            ++lpszFileName;
        Checked::tcsncpy_s(szTemp, _countof(szTemp), lpszFileName, _TRUNCATE);
        LPTSTR lpszLast = _tcsrchr(szTemp, '\"');
        if (lpszLast != NULL)
            *lpszLast = 0;

        if (AfxFullPath(szPath, szTemp) == FALSE)
        {
            ASSERT(FALSE);
            return NULL; // We won't open the file. MFC requires paths with
                         // length < _MAX_PATH
        }

        TCHAR szLinkName[_MAX_PATH];
        if (AfxResolveShortcut(AfxGetMainWnd(), szPath, szLinkName, _MAX_PATH))
            Checked::tcscpy_s(szPath, _countof(szPath), szLinkName);
#else
        Checked::tcscpy_s(szPath, _countof(szPath), lpszFileName);
#endif

        while (pos != NULL)
        {
            CDocTemplate* pTemplate = (CDocTemplate*) m_templateList.GetNext(pos);
            ASSERT_KINDOF(CDocTemplate, pTemplate);

            CDocTemplate::Confidence match;
            ASSERT(pOpenDocument == NULL);
            match = pTemplate->MatchDocType(szPath, pOpenDocument);
            if (match > bestMatch)
            {
                bestMatch = match;
                pBestTemplate = pTemplate;
            }
            if (match == CDocTemplate::yesAlreadyOpen)
                break;      // stop here
        }

        if (pOpenDocument != NULL)
        {
            POSITION posOpenDoc = pOpenDocument->GetFirstViewPosition();
            if (posOpenDoc != NULL)
            {
                CView* pView = pOpenDocument->GetNextView(posOpenDoc); // get first one
                ASSERT_VALID(pView);
                CFrameWnd* pFrame = pView->GetParentFrame();

                if (pFrame == NULL)
                    TRACE(traceAppMsg, 0, "Error: Can not find a frame for document to activate.\n");
                else
                {
                    pFrame->ActivateFrame();

                    if (pFrame->GetParent() != NULL)
                    {
                        CFrameWnd* pAppFrame;
                        if (pFrame != (pAppFrame = (CFrameWnd*) AfxGetApp()->m_pMainWnd))
                        {
                            ASSERT_KINDOF(CFrameWnd, pAppFrame);
                            pAppFrame->ActivateFrame();
                        }
                    }
                }
            }
            else
                TRACE(traceAppMsg, 0, "Error: Can not find a view for document to activate.\n");

            return pOpenDocument;
        }

        if (pBestTemplate == NULL)
        {
            AfxMessageBox(AFX_IDP_FAILED_TO_OPEN_DOC);
            return NULL;
        }

        return pBestTemplate->OpenDocumentFile(szPath, bAddToMRU, TRUE);
    }
}

BOOL CRadDocManager::SaveAllModified()
{
    //return CDocManager::SaveAllModified();
    int nModified = GetModifiedDocumentCount();

    if (nModified > 0)
    {
        CSaveModifiedDlg dlg;
        return dlg.DoModal() != IDCANCEL;
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

void CRadDocManager::CloseOtherDocuments(CDocument* pThisDoc)
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
                if (pDoc != pThisDoc)
                    pDoc->OnCloseDocument();
            }
        }
    }
}
