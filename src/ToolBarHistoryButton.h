#pragma once

#include "afxtoolbarcomboboxbutton.h"

class CToolBarHistoryButton : public CMFCToolBarComboBoxButton
{
public:
    CToolBarHistoryButton();
    CToolBarHistoryButton(UINT uiID, int iImage, DWORD dwStyle = CBS_DROPDOWNLIST, int iWidth = 0);
    ~CToolBarHistoryButton();

    DECLARE_SERIAL(CToolBarHistoryButton)

private:
    virtual INT_PTR AddItem(LPCTSTR lpszItem, DWORD_PTR dwData = 0) override;
    virtual INT_PTR AddSortedItem(LPCTSTR lpszItem, DWORD_PTR dwData = 0) override;
    virtual int Compare(LPCTSTR lpszItem1, LPCTSTR lpszItem2) override;
};
