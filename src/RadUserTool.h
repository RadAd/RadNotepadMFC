#pragma once

#include "afxusertool.h"

class CRadUserTool : public CUserTool
{
    DECLARE_SERIAL(CRadUserTool);

public:
    virtual void Serialize(CArchive& ar);
    virtual BOOL Invoke();

    bool m_bCapture = false;
    CString m_strTab;
    bool m_bSave = false;
};
