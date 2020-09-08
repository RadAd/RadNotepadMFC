#pragma once
#include <afxadv.h>

class CRadRecentFileList :
    public CRecentFileList
{
public:
    CRadRecentFileList(const CRecentFileList& rfl);

    virtual void Add(LPCTSTR lpszPathName) override;
    virtual BOOL GetDisplayName(CString& strName, int nIndex,
        LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName = TRUE) const override;
};
