#pragma once

#include <vector>

struct Tool
{
    Tool(LPCTSTR name, LPCTSTR cmd, LPCTSTR param = _T(""))
        : name(name)
        , cmd(cmd)
        , param(param)
    {
    }

    Tool(const Tool&) = delete;

    Tool(Tool&& other)
        : name(other.name)
        , cmd(other.cmd)
        , param(other.param)
        , hIcon(other.hIcon)
        , capture(other.capture)
        , save(other.save)
    {
        other.hIcon = NULL;
    }

    ~Tool()
    {
        DestroyIcon(hIcon);
    }

    CString name;
    CString cmd;
    CString param;
    HICON hIcon = NULL;
    CString capture;
    BOOL save = FALSE;
};

class COutputWnd;

struct ToolExecuteData
{
    CString cmd;
    CString param;
    CString directory;
    COutputWnd* pWndOutput; // TODO Maybe into the Scintilla control
};

void InitTools(std::vector<Tool>& rTools);
void ExecuteTool(const Tool& tool, const ToolExecuteData& ted);
