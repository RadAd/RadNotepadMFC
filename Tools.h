#pragma once

#include <vector>

struct Tool
{
    Tool(CString name, CString cmd, CString param = _T(""))
        : name(name)
        , cmd(cmd)
        , param(param)
        , hIcon(NULL)
    {

    }

    Tool(const Tool&) = delete;

    Tool(Tool&& other)
        : name(other.name)
        , cmd(other.cmd)
        , param(other.param)
        , hIcon(other.hIcon)
        , capture(other.capture)
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
    HICON hIcon;
    CString capture;
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
