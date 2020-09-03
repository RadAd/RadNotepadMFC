
#pragma once

#include "PaneToolBar.h"

struct Settings;

class CPropertiesWnd : public CDockablePane
{
// Construction
public:
	CPropertiesWnd();

    void InitLanguages();
	void AdjustLayout();

// Attributes
public:
	void SetVSDotNetLook(BOOL bSet)
	{
		m_wndPropList.SetVSDotNetLook(bSet);
		m_wndPropList.SetGroupNameFullWidth(bSet);
	}

protected:
    Settings* m_pSettings;
	CFont m_fntPropList;
	CComboBox m_wndObjectCombo;
    CPaneToolBar m_wndToolBar;
    CMFCPropertyGridCtrl m_wndPropList;
	std::vector<LPCTSTR> m_LanguageValues;
	std::vector<LPCTSTR> m_LanguageNames;
	CMFCPropertyGridProperty* m_pExtGroup;

// Implementation
public:
	virtual ~CPropertiesWnd();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnExpandAllProperties();
	afx_msg void OnUpdateExpandAllProperties(CCmdUI* pCmdUI);
	afx_msg void OnSortProperties();
	afx_msg void OnUpdateSortProperties(CCmdUI* pCmdUI);
	afx_msg void OnPropertiesReset();
	afx_msg void OnUpdatePropertiesReset(CCmdUI* pCmdUI);
	afx_msg void OnPropertiesNew();
	afx_msg void OnUpdatePropertiesNew(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg LRESULT OnPropertyChanged(WPARAM wParam, LPARAM lParam);
    afx_msg void OnComboSelChange();

	DECLARE_MESSAGE_MAP()

	void InitPropList();
	void SetPropListFont();
	void FillExtensions();

	int m_nComboHeight;
public:
    afx_msg void OnDestroy();
};

