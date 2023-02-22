#pragma once
#include "afxwin.h"

#include "RadCtlColor.h"

// CSaveModifiedDlg dialog

class CSaveModifiedDlg : public CRadDialog
{
    DECLARE_DYNAMIC(CSaveModifiedDlg)

public:
    CSaveModifiedDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CSaveModifiedDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SAVE_MODIFIED };
#endif

private:
    CRect m_OrigWndRect;

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
    virtual void OnOK() override;

    DECLARE_MESSAGE_MAP()
    CListCtrl m_List;
public:
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedNo();
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* pMinMaxInfo);
};
