#include "stdafx.h"
#include "PaneToolBar.h"

CPaneToolBar::CPaneToolBar()
{
    m_nSize = 0;
    m_pAccel = nullptr;
}

CPaneToolBar::~CPaneToolBar()
{
    delete[] m_pAccel;
}

void CPaneToolBar::OnUpdateCmdUI(CFrameWnd *, BOOL bDisableIfNoHndler)
{
    CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
}

BOOL CPaneToolBar::OnUserToolTip(CMFCToolBarButton * pButton, CString & strTTText) const
{
    TCHAR szFullText[256];

    AfxLoadString(pButton->m_nID, szFullText);
    AfxExtractSubString(strTTText, szFullText, 1, '\n');

    if (m_bShowShortcutKeys)
    {
        CString strAccelText;
        BOOL bFound = FALSE;
        for (int i = 0; i < m_nSize; i++)
        {
            if (m_pAccel[i].cmd == pButton->m_nID)
            {
                bFound = TRUE;

                CMFCAcceleratorKey helper(&m_pAccel[i]);

                CString strKey;
                helper.Format(strKey);

                if (!strAccelText.IsEmpty())
                {
                    strAccelText += _T("; ");
                }

                strAccelText += strKey;

#if 0
                if (!m_bAllAccelerators)
                {
                    break;
                }
#endif
            }
        }

        if (bFound)
        {
            strTTText += _T(" (");
            strTTText += strAccelText;
            strTTText += _T(')');
        }
    }

    return TRUE;
}

void CPaneToolBar::SetAccel(HACCEL hAccel)
{
    m_nSize = ::CopyAcceleratorTable(hAccel, nullptr, 0);
    delete[] m_pAccel;
    m_pAccel = new ACCEL[m_nSize];
    ::CopyAcceleratorTable(hAccel, m_pAccel, m_nSize);
}
