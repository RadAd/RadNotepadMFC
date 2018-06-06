#pragma once

class CRadWaitCursor
{
public:
    CRadWaitCursor();
    ~CRadWaitCursor();

    CMFCStatusBar* GetStatusBar();

    static int s_Count;

private:
    CFrameWnd* m_pMainWnd;
};
