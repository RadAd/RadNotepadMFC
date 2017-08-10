#pragma once


// CGoToLineDlg dialog

class CGoToLineDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGoToLineDlg)

public:
	CGoToLineDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGoToLineDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GOTOLINE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    int m_nLine;
    int m_nMaxLine;
};
