
#pragma once
#include "FileView.h"
#include "ClassView.h"
#include "OutputWnd.h"
#include "PropertiesWnd.h"
#include "RadCtlColor.h"
#include <vector>

extern UINT NEAR WM_RADNOTEPAD;
#define MSG_RADNOTEPAD 0xac20

class CMainFrame : public CMDIFrameWndEx
{
    DECLARE_DYNAMIC(CMainFrame)
public:
    CMainFrame();

    COutputWnd* GetWndOutput() { return &m_wndOutput; }
    CMFCToolBarComboBoxButton* GetHistoryButton() const;
    void SaveSearch(LPCTSTR lpszSectionName);
    void AddRootDir(LPCTSTR lpszRootDir);

// Attributes
public:

// Operations
public:
    INT_PTR DoWindowsDialog();
    void ChildMDIActiviate(CWnd* pWndMDIChild);
    void ChildMDIDestroyed(CWnd* pWndMDIChild);
    void ChildMDINextWindow(CWnd* pWndMDIChild, BOOL bIsPrev);
    void NotifySettingsChanged();

// Overrides
protected:
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
    virtual void GetMessageString(UINT nID, CString& rMessage) const override;

public:
    virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = NULL, CCreateContext* pContext = NULL) override;

// Implementation
public:
    virtual ~CMainFrame();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    BOOL CreateDockingWindows();
    const CMFCToolBar* GetToolbar(UINT nID) const
    {
        auto it = std::find_if(std::begin(m_wndToolBar), std::end(m_wndToolBar), [nID](const CMFCToolBar& tb)
            {
                return static_cast<UINT>(tb.GetDlgCtrlID()) == nID;
            });
        return it != std::end(m_wndToolBar) ? &*it : nullptr;
    }
    CMFCToolBar* GetToolbar(UINT nID)
    {
        auto it = std::find_if(std::begin(m_wndToolBar), std::end(m_wndToolBar), [nID](const CMFCToolBar& tb)
            {
                return static_cast<UINT>(tb.GetDlgCtrlID()) == nID;
            });
        return it != std::end(m_wndToolBar) ? &*it : nullptr;
    }

protected:
    int m_PrevNext;
    std::vector<CWnd*> m_MDIStack;
    CMFCMenuBar       m_wndMenuBar;
    CRadToolbar       m_wndToolBar[5];
    CMFCStatusBar     m_wndStatusBar;
    CMFCToolBarImages m_UserImages;
    CFileView         m_wndFileView;
    COutputWnd        m_wndOutput;
    CPropertiesWnd    m_wndProperties;

// Generated message map functions
protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT OnSetMessageString(WPARAM wParam, LPARAM lParam);
    afx_msg void OnFileOpenUrl();
    afx_msg void OnUpdateDarkMode(CCmdUI* pCmdUI);
    afx_msg void OnDarkMode();
    afx_msg void OnWindowManager();
    afx_msg void OnViewCustomize();
    afx_msg void OnSchemeIndicator();
    afx_msg void OnLineEndingIndicator();
    afx_msg void OnViewPane(UINT nID);
    afx_msg void OnUpdateViewPane(CCmdUI* pCmdUI);
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg void OnUpdateClear(CCmdUI* pCmdUI);
    afx_msg void OnUpdateDockingWindows(CCmdUI *pCmdUI);
    afx_msg void OnUpdateViewMargin(CCmdUI *pCmdUI);
    afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
    afx_msg LRESULT OnAfxWmOnGetTabTooltip(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAfxWmResetToolbar(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnRadNotepad(WPARAM wParam, LPARAM lParam);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg LRESULT OnChangeVisualManager(WPARAM wParam, LPARAM lParam);
};


