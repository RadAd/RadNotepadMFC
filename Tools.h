#pragma once

#include <vector>

struct Tool
{
    Tool(CString name, CString cmd, CString param)
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
};

void InitTools(std::vector<Tool>& rTools);
