#include "stdafx.h"
#include "ToolBarHistoryButton.h"

CToolBarHistoryButton::CToolBarHistoryButton()
{
}

CToolBarHistoryButton::CToolBarHistoryButton(UINT uiID, int iImage, DWORD dwStyle, int iWidth)
    : CMFCToolBarComboBoxButton(uiID, iImage, dwStyle, iWidth)
{

}

CToolBarHistoryButton::~CToolBarHistoryButton()
{
}

IMPLEMENT_SERIAL(CToolBarHistoryButton, CMFCToolBarComboBoxButton, 1)

INT_PTR CToolBarHistoryButton::AddItem(LPCTSTR lpszItem, DWORD_PTR dwData)
{
    INT_PTR iRet = CMFCToolBarComboBoxButton::AddItem(lpszItem, dwData);
    //m_iSelIndex = m_pWndCombo->GetCurSel();
    return iRet;
}

INT_PTR CToolBarHistoryButton::AddSortedItem(LPCTSTR lpszItem, DWORD_PTR dwData)
{
    INT_PTR iRet = CMFCToolBarComboBoxButton::AddSortedItem(lpszItem, dwData);
    //m_iSelIndex = m_pWndCombo->GetCurSel();
    if (iRet == m_lstItems.GetCount() - 1)
        iRet = m_pWndCombo->FindStringExact(-1, lpszItem); // Bug in CMFCToolBarComboBoxButton::AddSortedItem when item is already in the list
    return iRet;
}

int CToolBarHistoryButton::Compare(LPCTSTR /*lpszItem1*/, LPCTSTR /*lpszItem2*/)
{
    return -1; // So that strings are always inserted at the top
}
