
// MainFrm.h : interface of the CMainFrame class
//

#pragma once
#include "FileView.h"
#include "ClassView.h"
#include "OutputWnd.h"
#include "PropertiesWnd.h"
#include <vector>

extern UINT NEAR WM_RADNOTEPAD;
#define MSG_RADNOTEPAD 0xac20

class CMainFrame : public CMDIFrameWndEx
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:

// Operations
public:
    INT_PTR DoWindowsDialog();
    void ChildMDIActiviate(CWnd* pWndMDIChild);
    void ChildMDIDestroyed(CWnd* pWndMDIChild);
    void ChildMDINextWindow(CWnd* pWndMDIChild, BOOL bIsPrev);

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
    int m_PrevNext;
    std::vector<CWnd*> m_MDIStack;
	CMFCMenuBar       m_wndMenuBar;
	CMFCToolBar       m_wndToolBar;
	CMFCStatusBar     m_wndStatusBar;
	CMFCToolBarImages m_UserImages;
	CFileView         m_wndFileView;
	//CClassView        m_wndClassView;
	COutputWnd        m_wndOutput;
	CPropertiesWnd    m_wndProperties;

// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowManager();
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnViewPane(UINT nID);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnUpdateClear(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons();
    afx_msg LRESULT OnAfxWmOnGetTabTooltip(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnRadNotepad(WPARAM wParam, LPARAM lParam);
public:
    afx_msg void OnToolsTool(UINT nID);
    afx_msg void OnUpdateToolsTool(CCmdUI *pCmdUI);
    afx_msg void OnUpdateDockingWindows(CCmdUI *pCmdUI);
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};


