#include "stdafx.h"
#include "Tools.h"

void InitTools(std::vector<Tool>& rTools)
{
    // TODO Load form registry
    rTools.push_back(Tool(_T("Run"), _T("{file}"), _T("")));
    rTools.push_back(Tool(_T("CMD"), _T("cmd.exe"), _T("")));
    rTools.push_back(Tool(_T("Explorer"), _T("explorer.exe"), _T("/select,\"{file}\"")));
    rTools.push_back(Tool(_T("Google"), _T("https://www.google.com.au/search?q={selected}"), _T("")));
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
#endif
    }
}
