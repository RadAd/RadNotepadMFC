#include "stdafx.h"
#include "Tools.h"
#include "OutputWnd.h"
#import <MSXML6.dll> exclude("ISequentialStream", "_FILETIME")

// TODO
// Save before execute
// Stop too many outputting to the same window at the same time

inline bool operator==(const _bstr_t& s1, LPCWSTR s2)
{
    return wcscmp(s1, s2) == 0;
}

inline bool isnull(LPCWSTR s)
{
    return s == nullptr;
}

inline bool Required(LPCTSTR name, LPCTSTR value, LPCTSTR section)
{
    if (isnull(value))
    {
        CString msg;
        msg.Format(_T("Missing %s: %s"), name, section);
        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
        return false;
    }
    else
        return true;
}

inline _bstr_t GetAttribute(MSXML2::IXMLDOMNode* pXMLNode, LPCWSTR name)
{
    MSXML2::IXMLDOMNamedNodeMapPtr pXMLAttributes(pXMLNode->Getattributes());

    MSXML2::IXMLDOMNodePtr pXMLAttr(pXMLAttributes->getNamedItem(const_cast<LPWSTR>(name)));

    _bstr_t bstrAttrValue;
    if (pXMLAttr)
        bstrAttrValue = pXMLAttr->Gettext();
    //pXMLAttr->GetnodeValue();   // TODO Use this instead???

    return bstrAttrValue;
}

inline _bstr_t GetElementText(MSXML2::IXMLDOMNode* pXMLNode, LPCWSTR name)
{
    MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLNode->selectSingleNode(name));

    _bstr_t bstrText;

    if (pXMLChildNode != nullptr)
        bstrText = pXMLChildNode->Gettext();

    return bstrText;
}

void ProcessTools(MSXML2::IXMLDOMNodePtr pXMLNode, std::vector<Tool>& rTools)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"tool")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t cmd = GetElementText(pXMLChildNode, _T("cmd"));
                _bstr_t icon = GetElementText(pXMLChildNode, _T("icon"));
                _bstr_t param = GetElementText(pXMLChildNode, _T("param"));
                _bstr_t capture = GetAttribute(pXMLChildNode, _T("capture"));
                _bstr_t save = GetAttribute(pXMLChildNode, _T("save"));

                if (Required(_T("name"), name, bstrName) && Required(_T("cmd"), cmd, bstrName))
                {
                    rTools.push_back(Tool(name, cmd, param));
                    Tool& tool(rTools.back());

                    TCHAR strIcon[MAX_PATH];
                    int nIcon = 1;
                    if (!isnull(icon))
                    {
                        wcscpy_s(strIcon, icon);
                        nIcon = PathParseIconLocation(strIcon);
                    }
                    else
                        wcscpy_s(strIcon, tool.cmd);
                    ExtractIconEx(strIcon, 0, nullptr, &tool.hIcon, nIcon);
                    if (!isnull(capture) && !(capture == L"false"))
                    {
                        if (capture == L"true")
                            tool.capture = L"Output";
                        else
                            tool.capture = (LPCTSTR) capture;
                    }
                    if (!isnull(save))
                        tool.save = save == L"true";
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void LoadTools(LPCTSTR strToolFile, std::vector<Tool>& rTools)
{
    MSXML2::IXMLDOMDocumentPtr pDoc(__uuidof(MSXML2::DOMDocument60));
    pDoc->Putasync(VARIANT_FALSE);
    pDoc->PutvalidateOnParse(VARIANT_FALSE);
    pDoc->PutresolveExternals(VARIANT_FALSE);
    pDoc->PutpreserveWhiteSpace(VARIANT_TRUE);

    VARIANT_BOOL varStatus = pDoc->load((LPCTSTR) strToolFile);
    if (varStatus == VARIANT_TRUE)
    {
        MSXML2::IXMLDOMElementPtr pXMLRoot(pDoc->GetdocumentElement());
        ProcessTools(pXMLRoot, rTools);
    }
    else
    {
        MSXML2::IXMLDOMParseErrorPtr pXMLErr(pDoc->GetparseError());

        CString msg;
        msg.Format(_T("%s(%ld, %ld): Code: 0x%x - %s\n"), strToolFile, pXMLErr->line, pXMLErr->linepos, pXMLErr->errorCode, (LPCTSTR) pXMLErr->reason);
        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
    }
}

void LoadToolDirectory(LPCTSTR strDirectory, std::vector<Tool>& rTools)
{
    TCHAR full[_MAX_PATH];

    PathCombine(full, strDirectory, _T("tools.dat"));
    if (PathFileExists(full))
        LoadTools(full, rTools);
}

void InitTools(std::vector<Tool>& rTools)
{
#if 0
    rTools.push_back(Tool(_T("Run"), _T("{file}")));
    rTools.push_back(Tool(_T("CMD"), _T("cmd.exe")));
    rTools.push_back(Tool(_T("Explorer"), _T("explorer.exe"), _T("/select,\"{file}\"")));
    rTools.push_back(Tool(_T("Google"), _T("https://www.google.com.au/search?q={selected}")));
    rTools.push_back(Tool(_T("FindStr"), _T("findstr.exe"), _T("/L /S /N \"{selected}\" *.txt *.cpp *.h *.java"), TRUE));
    rTools.push_back(Tool(_T("VCMake"), _T("%HOMEDRIVE%%HOMEPATH%\\Dropbox\\Utils\\CommandLine\\bin\\vcmake.bat"), _T(""), TRUE));
    rTools.push_back(Tool(_T("Output"), _T("cmd.exe"), _T("/C type {file}"), TRUE));

    for (Tool& t : rTools)
    {
        TCHAR exe[MAX_PATH];
        PTSTR pArgs = PathGetArgs(t.cmd);
        while (pArgs > t.cmd && pArgs[-1] == _T(' '))
            --pArgs;
        wcsncpy_s(exe, t.cmd, pArgs - t.cmd);
        PathUnquoteSpaces(exe);

        if (t.hIcon == NULL)
            //t.hIcon = ExtractIcon(AfxGetInstanceHandle(), t.cmd, 0);
            ExtractIconEx(exe, 0, nullptr, &t.hIcon, 1);
#if 0
        if (t.hIcon == NULL)
        {
            SHFILEINFO fi = {};
            SHGetFileInfo(exe, 0, &fi, sizeof(fi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
            t.hIcon = fi.hIcon;
        }
    }
#endif
#else
    HMODULE hModule = NULL;
    TCHAR exepath[_MAX_PATH];

    GetModuleFileName(hModule, exepath, MAX_PATH);
    PathFindFileName(exepath)[0] = _T('\0');
    LoadToolDirectory(exepath, rTools);
#if 0
    PathCombine(exepath, exepath, _T("..\\.."));
    LoadToolDirectory(exepath, rTools);
#endif
#endif
}

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

void ExecuteTool(const Tool& tool, const ToolExecuteData& ted)
{
    if (!tool.capture.IsEmpty())
    {
        SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };

        HANDLE hRead, hWrite;
        CreatePipe(&hRead, &hWrite, &sa, 1024);

        STARTUPINFO si = { sizeof(STARTUPINFO) };
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWrite;
        si.hStdError = hWrite;
        PROCESS_INFORMATION pi = {};
        TCHAR cmdline[MAX_PATH] = _T("");
        StrCpy(cmdline, ted.cmd);
        StrCat(cmdline, _T(" "));
        StrCat(cmdline, ted.param);
        if (CreateProcess(nullptr, cmdline, nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, ted.directory, &si, &pi))
        {
            ted.pWndOutput->ShowPane(TRUE, FALSE, FALSE);
            COutputList* pOutputList = ted.pWndOutput->Reset(tool.capture, ted.directory);
            if (pOutputList)
            {
                pOutputList->AppendText(_T("> "), -1);
                pOutputList->AppendText(cmdline, -1);
                pOutputList->AppendText(_T("\n"), -1);
            }

            CloseHandle(hWrite);
            CloseHandle(pi.hThread);

            CaptureOutputData* cd = new CaptureOutputData;
            cd->hProcess = pi.hProcess;
            cd->hRead = hRead;
            cd->pWndOutput = ted.pWndOutput;
            cd->sCapture = tool.capture;
            if (CreateThread(nullptr, 0, CaptureOutput, cd, 0, nullptr) == NULL)
            {
                CloseHandle(hRead);
                CloseHandle(pi.hProcess);

                CString msg;
                msg.Format(_T("Error CreateThread: %d"), GetLastError());
                AfxMessageBox(msg, MB_ICONSTOP | MB_OK);
            }
        }
        else
        {
            CloseHandle(hRead);
            CloseHandle(hWrite);

            CString msg;
            msg.Format(_T("Error CreateProcess: %d"), GetLastError());
            AfxMessageBox(msg, MB_ICONSTOP | MB_OK);
        }
    }
    else
    {
        HINSTANCE hExeInst = ShellExecute(AfxGetMainWnd()->GetSafeHwnd(), L"open", ted.cmd, ted.param, ted.directory, SW_SHOW);
        if (hExeInst <= HINSTANCE(32))
        {
            CString msg;
            msg.Format(_T("Error ShellExecute: %d"), reinterpret_cast<INT_PTR>(hExeInst));
            AfxMessageBox(msg, MB_ICONSTOP | MB_OK);
        }
    }
}
