
#pragma once

#include "ViewTree.h"

struct TreeItem;

class CFileViewToolBar : public CMFCToolBar
{
public:
    CFileViewToolBar()
    {
        m_nSize = 0;
        m_pAccel = nullptr;
    }

    virtual ~CFileViewToolBar()
    {
        delete [] m_pAccel;
    }

private:
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler) override
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

    virtual BOOL OnUserToolTip(CMFCToolBarButton* pButton, CString& strTTText) const override
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

    virtual BOOL AllowShowOnList() const { return FALSE; }

public:
    void SetAccel(HACCEL hAccel)
    {
        m_nSize = ::CopyAcceleratorTable(hAccel, nullptr, 0);
        delete[] m_pAccel;
        m_pAccel = new ACCEL[m_nSize];
        ::CopyAcceleratorTable(hAccel, m_pAccel, m_nSize);
    }

private:
    int m_nSize;
    ACCEL* m_pAccel;
};

class CFileView : public CDockablePane
{
// Construction
public:
	CFileView();

	void AdjustLayout();

// Attributes
protected:
    HACCEL m_hAccel;
    ULONG m_Notify;
    LPITEMIDLIST m_pRootPidl;
    CComPtr<IMalloc> m_Malloc;
	CViewTree m_wndFileView;
	CImageList m_FileViewImages;
	CFileViewToolBar m_wndToolBar;

protected:
	void FillFileView();
    void InsertChildren(HTREEITEM hNode, TreeItem* ti);
    void InsertChildren(CComPtr<IShellFolder>& Folder, HTREEITEM hParent);
    HTREEITEM FindItem(HTREEITEM hParentItem, PCITEMID_CHILD pidls);
    HTREEITEM FindItem(PCIDLIST_RELATIVE pidls, BOOL bExpandChildren);
    HTREEITEM FindParentItem(PCIDLIST_RELATIVE pidls);
    HTREEITEM InsertChild(HTREEITEM hParent, CComPtr<IShellFolder>& folder, PITEMID_CHILD ItemId);
    HTREEITEM FindSortedPos(HTREEITEM hParent, const TreeItem* tir);
    void SortChildren(HTREEITEM hParent);

    void OnDeleteItem(PCIDLIST_RELATIVE pidls);
    void OnRenameItem(PCIDLIST_RELATIVE pidls, PCIDLIST_RELATIVE new_pidls);
    void OnAddItem(PCIDLIST_RELATIVE pidls);
    void OnUpdateItem(PCIDLIST_RELATIVE pidls);

// Implementation
public:
	virtual ~CFileView();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnProperties();
    afx_msg void OnUpdateFileSelected(CCmdUI *pCmdUI);
    afx_msg void OnUpdateActiveDocument(CCmdUI *pCmdUI);
    afx_msg void OnSync();
    afx_msg void OnEditRename();
    afx_msg void OnEditView();
    afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnItemExpanding(NMHDR* pHdr, LRESULT* pResult);
    afx_msg void OnDeleteItem(NMHDR* pHdr, LRESULT* pResult);
    afx_msg void OnBeginLabelEdit(NMHDR* pHdr, LRESULT* pResult);
    afx_msg void OnEndLabelEdit(NMHDR* pHdr, LRESULT* pResult);
    afx_msg void OnDblClick(NMHDR* pHdr, LRESULT* pResult);
    afx_msg LRESULT OnShellChange(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL PreTranslateMessage(MSG* pMsg);
};

