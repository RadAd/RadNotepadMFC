#pragma once
#include "afxwin.h"
class CRadDocManager :
    public CDocManager
{
public:
    CRadDocManager();
    ~CRadDocManager();

    static CDocument* GetActiveDocument();

    virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
        DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
};
