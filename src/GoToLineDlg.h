#pragma once

#include "RadDialog.h"

// CGoToLineDlg dialog

class CGoToLineDlg : public CRadDialog
{
    DECLARE_DYNAMIC(CGoToLineDlg)

public:
    CGoToLineDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GOTOLINE };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    DECLARE_MESSAGE_MAP()

public:
    Scintilla::Line m_nLine;
    Scintilla::Line m_nMaxLine;
};
