
#pragma once

struct Language;

/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CScintillaCtrl
{
// Construction
public:
	COutputList();

public:
    void SetLanguage(LPCTSTR pOutput);
    void NotifySettingsChanged();
    void Clear();
    void SetDirectory(LPCTSTR pText) { m_strDirectory = pText; }
    void AppendText(LPCSTR pText, int nLen);
    void AppendText(LPCWSTR pText, int nLen);
    void AppendText(const CString& str) { AppendText(str, str.GetLength()); }

private:
    const Language* m_pLanguage;

// Implementation
public:
	virtual ~COutputList();

private:
    CString m_strDirectory;

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnEditCopy();
    afx_msg void OnEditClear();
    afx_msg void OnViewOutput();
    afx_msg void OnReturn();
    afx_msg void OnHotSpotClick(NMHDR* pHdr, LRESULT* pResult);
};

class COutputWnd : public CDockablePane
{
// Construction
public:
	COutputWnd();

// Attributes
protected:
    HACCEL m_hAccel;
    CMFCTabCtrl	m_wndTabs;

// Overrides
protected:
    virtual BOOL PreTranslateMessage(MSG* pMsg) override;
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;

public:
    void AdjustHorzScroll(CListBox& wndListBox);
    void UpdateFonts();
    void NotifySettingsChanged();
    COutputList* Get(LPCTSTR pOutput, BOOL bCreate = FALSE);
    COutputList* Reset(LPCTSTR pOutput, LPCTSTR pText);

// Implementation
public:
	virtual ~COutputWnd();

protected:
    DECLARE_MESSAGE_MAP()
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
};
