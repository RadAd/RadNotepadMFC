
// RadNotepad.h : main header file for the RadNotepad application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "..\resource.h"       // main symbols
#include "Settings.h"

// CRadNotepadApp:
// See RadNotepad.cpp for the implementation of this class
//

class CRadNotepadApp : public CWinAppEx
{
public:
	CRadNotepadApp();

    static BOOL IsInternetUrl(LPCTSTR lpszFileName);

// Overrides
public:
	virtual BOOL InitInstance() override;
	virtual int ExitInstance() override;

public:
    void NotifySettingsChanged();

// Implementation
    HMODULE m_hSciDLL;
    Settings m_Settings;
    BOOL m_SaveSettings;

    virtual BOOL SaveState(LPCTSTR lpszSectionName = NULL, CFrameImpl* pFrameImpl = NULL) override;
	virtual void PreLoadState() override;
	virtual void LoadCustomState() override;
	virtual void SaveCustomState() override;
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName) override;
    virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) override;

    afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
    afx_msg void OnFileCloseAll();
    afx_msg void OnUpdateFileCloseAll(CCmdUI *pCmdUI);
    afx_msg void OnFileSaveAll();
    afx_msg void OnUpdateFileSaveAll(CCmdUI *pCmdUI);
    afx_msg void OnFileCloseOthers();
    afx_msg void OnUpdateFileCloseOthers(CCmdUI *pCmdUI);
    afx_msg void OnUpdateViewSaveSettingsOnExit(CCmdUI *pCmdUI);
    afx_msg void OnViewSaveSettingsOnExit();
};

extern CRadNotepadApp theApp;
