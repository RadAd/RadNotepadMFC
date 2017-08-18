
#pragma once

/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CScintillaCtrl
{
// Construction
public:
	COutputList();

public:
    void Clear();
    void SetDirectory(LPCTSTR pText) { m_strDirectory = pText; }
    void AppendText(LPCSTR pText, int nLen);
    void AppendText(LPCWSTR pText, int nLen);
    void AppendText(const CString& str) { AppendText(str, str.GetLength()); }

// Implementation
public:
	virtual ~COutputList();

private:
    CString m_strDirectory;

protected:
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnEditCopy();
	afx_msg void OnEditClear();
	afx_msg void OnViewOutput();
    afx_msg void OnHotSpotClick(NMHDR* pHdr, LRESULT* pResult);

	DECLARE_MESSAGE_MAP()
public:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};

enum OutputWindowE {
    OW_OUTPUT,
    OW_LOG,
    OW_MAX,
};

class COutputWnd : public CDockablePane
{
// Construction
public:
	COutputWnd();

	void UpdateFonts();
    COutputList* Get(OutputWindowE ow) { return &m_wndOutput[ow]; }

// Attributes
protected:
	CMFCTabCtrl	m_wndTabs;

	COutputList m_wndOutput[OW_MAX];

protected:
	void AdjustHorzScroll(CListBox& wndListBox);

// Implementation
public:
	virtual ~COutputWnd();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()
};

