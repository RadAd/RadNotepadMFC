
// RadNotepad.h : main header file for the RadNotepad application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "Settings.h"

// CRadNotepadApp:
// See RadNotepad.cpp for the implementation of this class
//

class CRadNotepadApp : public CWinAppEx
{
public:
	CRadNotepadApp();

    int GetModifiedDocumentCount() const;

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	BOOL  m_bHiColorIcons;
    Settings m_Settings;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
    virtual BOOL SaveAllModified();

    BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
        DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);

    afx_msg void OnAppAbout();
    afx_msg void OnFileOpen();
	DECLARE_MESSAGE_MAP()
    afx_msg void OnFileCloseAll();
    afx_msg void OnUpdateFileCloseAll(CCmdUI *pCmdUI);
    afx_msg void OnFileSaveAll();
    afx_msg void OnUpdateFileSaveAll(CCmdUI *pCmdUI);
};

extern CRadNotepadApp theApp;
