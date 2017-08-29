
#pragma once

#include "ViewTree.h"
#include "PaneToolBar.h"

struct TreeItem;

struct ITEMIDLISTDeleter
{
    void operator()(LPITEMIDLIST p)
    {
        ILFree(p);
    }
};

typedef std::unique_ptr<ITEMIDLIST __unaligned, ITEMIDLISTDeleter> PtrIDChild;
typedef std::unique_ptr<ITEMIDLIST __unaligned, ITEMIDLISTDeleter> PtrIDAbsolute;

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
    CPaneToolBar m_wndToolBar;

protected:
	void FillFileView();
    void InsertChildren(HTREEITEM hNode, TreeItem* ti);
    void InsertChildren(CComPtr<IShellFolder>& Folder, HTREEITEM hParent);
    HTREEITEM FindItem(HTREEITEM hParentItem, PCITEMID_CHILD pidls);
    HTREEITEM FindItem(PCIDLIST_ABSOLUTE pidls, BOOL bExpandChildren);
    HTREEITEM FindParentItem(PCIDLIST_ABSOLUTE pidls);
    HTREEITEM InsertChild(HTREEITEM hParent, CComPtr<IShellFolder>& folder, PtrIDChild ItemId);
    HTREEITEM FindSortedPos(HTREEITEM hParent, const TreeItem* tir);
    void SortChildren(HTREEITEM hParent);

    void OnDeleteItem(PCIDLIST_ABSOLUTE pidls);
    void OnRenameItem(PCIDLIST_ABSOLUTE pidls, PCIDLIST_ABSOLUTE new_pidls);
    void OnAddItem(PCIDLIST_ABSOLUTE pidls);
    void OnUpdateItem(PCIDLIST_ABSOLUTE pidls);

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

