#include "stdafx.h"
#include "Tools.h"

void InitTools(std::vector<Tool>& rTools)
{
    // TODO Load form registry
    rTools.push_back(Tool(_T("Run"), _T("{file}"), _T("")));
    rTools.push_back(Tool(_T("CMD"), _T("cmd.exe"), _T("")));
    rTools.push_back(Tool(_T("Explorer"), _T("explorer.exe"), _T("/select,\"{file}\"")));
    rTools.push_back(Tool(_T("Google"), _T("https://www.google.com.au/search?q={selected}"), _T("")));

    for (Tool& t : rTools)
    {
        if (t.hIcon == NULL)
            //t.hIcon = ExtractIcon(AfxGetInstanceHandle(), t.cmd, 0);
            ExtractIconEx(t.cmd, 0, nullptr, &t.hIcon, 1);
    }
}
