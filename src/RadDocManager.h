#pragma once
#include "afxwin.h"

class CRadDocManager :
    public CDocManager
{
public:
    CRadDocManager();
    ~CRadDocManager();

    static CDocument* GetActiveDocument();
    static CView* GetActiveView();

    int GetModifiedDocumentCount() const;

    virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
        DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate) override;
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) override;

    virtual BOOL SaveAllModified() override;
    void SaveAll();
    void UpdateAllViews(CView* pSender, LPARAM lHint = 0L, CObject* pHint = NULL);
    void CloseOtherDocuments(CDocument* pThisDoc);
};
