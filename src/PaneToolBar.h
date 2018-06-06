#pragma once
#include "afxtoolbar.h"
class CPaneToolBar :
    public CMFCToolBar
{
public:
    CPaneToolBar();
    ~CPaneToolBar();

private:
    virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler) override;
    virtual BOOL OnUserToolTip(CMFCToolBarButton* pButton, CString& strTTText) const override;
    virtual BOOL AllowShowOnList() const { return FALSE; }

public:
    void SetAccel(HACCEL hAccel);

private:
    int m_nSize;
    ACCEL* m_pAccel;
};
