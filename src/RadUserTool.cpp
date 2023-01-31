#include "stdafx.h"
#include "RadUserTool.h"

#include "MainFrm.h"
#include "OutputWnd.h"
#include "RadNotepadView.h"
#include "RadDocManager.h"

IMPLEMENT_SERIAL(CRadUserTool, CUserTool, VERSIONABLE_SCHEMA | 1)

struct CaptureOutputData
{
    HANDLE hProcess;
    HANDLE hRead;
    COutputWnd* pWndOutput; // TODO Maybe into the Scintilla control
    CString sCapture;
};

static DWORD WINAPI CaptureOutput(LPVOID lpParameter)
{
    CaptureOutputData* pData = reinterpret_cast<CaptureOutputData*>(lpParameter);

    char Buffer[1024];
    DWORD dwRead = 0;
    while (ReadFile(pData->hRead, Buffer, ARRAYSIZE(Buffer), &dwRead, nullptr))
    {
        COutputList* pOutputList = pData->pWndOutput->Get(pData->sCapture);
        if (pOutputList)
            pOutputList->AppendText(Buffer, dwRead);
    }

    CloseHandle(pData->hRead);

    DWORD dwExitCode = 0;
    GetExitCodeProcess(pData->hProcess, &dwExitCode);

    CString msg;
    msg.Format(_T("\n> Exit code: %d\n"), dwExitCode);

    COutputList* pOutputList = pData->pWndOutput->Get(pData->sCapture);
    if (pOutputList)
        pOutputList->AppendText(msg);

    CloseHandle(pData->hProcess);

    delete pData;
    return 0;
}

static CString ExpandEnvironmentStrings(LPCTSTR str)
{
    CString ret;
    ExpandEnvironmentStrings(str, ret.GetBufferSetLength(MAX_PATH), MAX_PATH);
    ret.ReleaseBuffer();
    return ret;
}

void CRadUserTool::Serialize(CArchive& ar)
{
    CUserTool::Serialize(ar);

    if (ar.IsLoading())
    {
        ar >> m_bCapture;
        ar >> m_strTab;
        ar >> m_bSave;
    }
    else
    {
        ar << m_bCapture;
        ar << m_strTab;
        ar << m_bSave;
    }
}

BOOL CRadUserTool::Invoke()
{
    CRadNotepadView* pView = DYNAMIC_DOWNCAST(CRadNotepadView, CRadDocManager::GetActiveView());
    CDocument* pDoc = pView == nullptr ? nullptr : pView->CView::GetDocument();
    if (m_bSave && pDoc != nullptr && !pDoc->DoFileSave())
        return FALSE;

    CString strCommand = ExpandEnvironmentStrings(m_strCommand);
    CString strArguments = ExpandEnvironmentStrings(m_strArguments);
    CString strInitialDirectory = m_strInitialDirectory.IsEmpty() ? _T("{path}") : ExpandEnvironmentStrings(m_strInitialDirectory);

    if (pDoc != nullptr && pView != nullptr)
    {
        TCHAR FileName[MAX_PATH] = _T("");
        StrCpy(FileName, pDoc->GetPathName());

        TCHAR Dir[MAX_PATH] = _T("");
        StrCpy(Dir, pDoc->GetPathName());
        PathRemoveFileSpec(Dir);

        for (CString* s : { &strCommand, &strInitialDirectory })
        {
            s->Replace(_T("{file}"), FileName);
            s->Replace(_T("{path}"), Dir);
            s->Replace(_T("{selected}"), pView->GetCurrentWord());
        }

        PathQuoteSpaces(FileName);
        PathQuoteSpaces(Dir);

        for (CString* s : { &strArguments })
        {
            s->Replace(_T("{file}"), FileName);
            s->Replace(_T("{path}"), Dir);
            s->Replace(_T("{selected}"), pView->GetCurrentWord());
        }
    }
    else
    {
        for (CString* s : { &strCommand, &strInitialDirectory })
        {
            s->Replace(_T("{file}"), _T(""));
            s->Replace(_T("{path}"), _T(""));
            s->Replace(_T("{selected}"), _T(""));
        }

        for (CString* s : { &strCommand, &strInitialDirectory, &strArguments })
        {
            s->Replace(_T("{file}"), _T("\"\""));
            s->Replace(_T("{path}"), _T("\"\""));
            s->Replace(_T("{selected}"), _T(""));
        }
    }

    if (m_bCapture)
    {
        CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
        COutputWnd* pWndOutput = pMainFrame->GetWndOutput();

        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

        HANDLE hRead, hWrite;
        CreatePipe(&hRead, &hWrite, &sa, 1024);

        STARTUPINFO si = { sizeof(STARTUPINFO) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;

        PROCESS_INFORMATION pi = {};

        TCHAR cmdline[MAX_PATH] = _T("");
        StrCpy(cmdline, strCommand);
        StrCat(cmdline, _T(" "));
        StrCat(cmdline, strArguments);

        if (CreateProcess(nullptr, cmdline, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, strInitialDirectory.IsEmpty() ? nullptr : (LPCWSTR) strInitialDirectory, &si, &pi))
        {
            pWndOutput->ShowPane(TRUE, FALSE, FALSE);

            CloseHandle(hWrite);
            CloseHandle(pi.hThread);

            CaptureOutputData* cd = new CaptureOutputData;
            cd->hProcess = pi.hProcess;
            cd->hRead = hRead;
            cd->pWndOutput = pWndOutput;
            cd->sCapture = !m_strTab.IsEmpty() ? m_strTab : L"Output";

            COutputList* pOutputList = pWndOutput->Reset(cd->sCapture, strInitialDirectory);
            if (pOutputList)
            {
                pOutputList->AppendText(_T("> "), -1);
                pOutputList->AppendText(cmdline, -1);
                pOutputList->AppendText(_T("\n"), -1);
            }

            if (CreateThread(nullptr, 0, CaptureOutput, cd, 0, nullptr) == NULL)
            {
                CloseHandle(hRead);
                CloseHandle(pi.hProcess);

                CString msg;
                msg.Format(_T("Error CreateThread: %d"), GetLastError());
                AfxMessageBox(msg, MB_ICONSTOP | MB_OK);
            }

            return TRUE;
        }
        else
        {
            CloseHandle(hRead);
            CloseHandle(hWrite);

            CString msg;
            msg.Format(_T("Error CreateProcess: %d"), GetLastError());
            AfxMessageBox(msg, MB_ICONSTOP | MB_OK);
            return FALSE;
        }
    }
    else
    {
        //return CUserTool::Invoke();
        HINSTANCE hExeInst = ::ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), NULL, strCommand, strArguments, strInitialDirectory, SW_SHOWNORMAL);
        if (hExeInst <= HINSTANCE(32))
        {
            CString msg;
            msg.Format(_T("Error ShellExecute: %d"), static_cast<int>(reinterpret_cast<INT_PTR>(hExeInst)));
            AfxMessageBox(msg, MB_ICONSTOP | MB_OK);
            return FALSE;
        }
        else
            return TRUE;
    }
}
