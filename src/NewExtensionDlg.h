#pragma once

#include "RadDialog.h"

struct Settings;

// CNewExtensionDlg dialog

class CNewExtensionDlg : public CRadDialog
{
	DECLARE_DYNAMIC(CNewExtensionDlg)

public:
	CNewExtensionDlg(Settings* pSettings, CWnd* pParent = NULL);   // standard constructor
	virtual ~CNewExtensionDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GOTOLINE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	Settings* m_pSettings;
public:
    CString m_strExtension;
};
