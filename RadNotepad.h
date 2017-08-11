
// RadNotepad.h : main header file for the RadNotepad application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols
#include "Settings.h"
#include "Tools.h"

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
    HMODULE m_hSciDLL;
    Settings m_Settings;
    std::vector<Tool> m_Tools;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
    virtual BOOL SaveAllModified();

    afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
    afx_msg void OnFileCloseAll();
    afx_msg void OnUpdateFileCloseAll(CCmdUI *pCmdUI);
    afx_msg void OnFileSaveAll();
    afx_msg void OnUpdateFileSaveAll(CCmdUI *pCmdUI);
};

extern CRadNotepadApp theApp;
