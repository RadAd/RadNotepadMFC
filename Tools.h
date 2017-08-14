#pragma once

#include <vector>

struct Tool
{
    Tool(CString name, CString cmd, CString param = _T(""), BOOL capture = FALSE)
        : name(name)
        , cmd(cmd)
        , param(param)
        , hIcon(NULL)
        , bCapture(capture)
    {

    }

    Tool(const Tool&) = delete;

    Tool(Tool&& other)
        : name(other.name)
        , cmd(other.cmd)
        , param(other.param)
        , hIcon(other.hIcon)
        , bCapture(other.bCapture)
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
    BOOL bCapture;
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
