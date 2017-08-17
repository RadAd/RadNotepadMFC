
#pragma once

/////////////////////////////////////////////////////////////////////////////
// COutputList window

class COutputList : public CScintillaCtrl
{
// Construction
public:
	COutputList();

// Implementation
public:
	virtual ~COutputList();

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
    void Clear(OutputWindowE ow);
    void AppendText(OutputWindowE ow, LPCSTR pText, int nLen);
    void AppendText(OutputWindowE ow, LPCWSTR pText, int nLen);
    void AppendText(OutputWindowE ow, const CString& str) { AppendText(ow, str, str.GetLength()); }

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

