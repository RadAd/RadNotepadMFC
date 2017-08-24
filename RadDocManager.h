#pragma once
#include "afxwin.h"
class CRadDocManager :
    public CDocManager
{
public:
    CRadDocManager();
    ~CRadDocManager();

    static CDocument* GetActiveDocument();

    int GetModifiedDocumentCount() const;

    virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
        DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate) override;

    virtual BOOL SaveAllModified() override;
    void SaveAll();
    void UpdateAllViews(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);
};
