
#include "stdafx.h"

#include "FileView.h"
#include "RadNotepad.h"
#include "RadDocManager.h"
#include <set>

#include "..\resource.h"

// TODO
// top level file updates
// file view collapse all
// file view change root
// auto sync with current file

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define ID_FILE_VIEW_TREE 4

#define MSG_SHELLCHANGE (WM_USER + 117)

static inline bool operator==(const PtrIDChild& p1, PCITEMID_CHILD p2)
{
    return ILIsEqual(p1.get(), p2) != FALSE;
}

static inline PtrIDAbsolute operator+(const PtrIDAbsolute& p1, PCITEMID_CHILD p2)
{
    return PtrIDAbsolute(ILCombine(p1.get(), p2));
}

static inline PtrIDAbsolute operator+(const PtrIDAbsolute& p1, const PtrIDChild& p2)
{
    return PtrIDAbsolute(ILCombine(p1.get(), p2.get()));
}

static inline CComPtr<IShellFolder> GetFolder(PCIDLIST_ABSOLUTE pidl)
{
    HRESULT    hr = NOERROR;

    CComPtr<IShellFolder>    Desktop;
    hr = SHGetDesktopFolder(&Desktop);

    CComPtr<IShellFolder>    Parent;

    if (pidl && !ILIsEmpty(pidl))
        Desktop->BindToObject(pidl, 0, IID_IShellFolder, (LPVOID *) &Parent);
    else
        Parent = Desktop;

    return Parent;
}

static inline CComPtr<IShellFolder> GetParentFolder(PCIDLIST_ABSOLUTE pidl)
{
    PtrIDAbsolute parent_pidl(ILClone(pidl));
    ILRemoveLastID(parent_pidl.get());

    CComPtr<IShellFolder> Parent = GetFolder(parent_pidl.get());

    return Parent;
}

static inline CString GetDisplayNameOf(const CComPtr<IShellFolder>& Folder, PCITEMID_CHILD ItemId, const CComPtr<IMalloc>& Malloc, SHGDNF Flags = SHGDN_NORMAL)
{
    STRRET    Name = {};
    HRESULT hr = Folder->GetDisplayNameOf(ItemId, Flags, &Name);
    CString ret;
    if (SUCCEEDED(hr))
    {
        StrRetToBuf(&Name, ItemId, ret.GetBufferSetLength(MAX_PATH), MAX_PATH);
        ret.ReleaseBuffer();
        if (Name.uType == STRRET_WSTR)
            Malloc->Free(Name.pOleStr);
    }
    return ret;
}

struct TreeItem
{
    CComPtr<IShellFolder>     Parent;
    PtrIDChild    ItemId;

    CComPtr<IShellFolder> GetFolder() const
    {
        if (ItemId == nullptr)
            return  Parent;
        else
        {
            CComPtr<IShellFolder> Folder;
            Parent->BindToObject(ItemId.get(), 0, IID_IShellFolder, (LPVOID *) &Folder);
            return Folder;
        }
    }

    HRESULT GetAttributesOf(SFGAOF *Flags) const
    {
        LPCITEMIDLIST    IdList[1] = { ItemId.get() };
        HRESULT hr = Parent->GetAttributesOf(1, IdList, Flags);
        return hr;
    }

    CString GetDisplayNameOf(const CComPtr<IMalloc>& Malloc, SHGDNF Flags = SHGDN_NORMAL) const
    {
        return ::GetDisplayNameOf(Parent, ItemId.get(), Malloc, Flags);
    }

    BOOL SetName(HWND hWnd, LPCWSTR text, SHGDNF Flags)
    {
        LPITEMIDLIST pnewidls = nullptr;
        HRESULT hr = Parent->SetNameOf(hWnd, ItemId.get(), text, Flags, &pnewidls);
        if (SUCCEEDED(hr) && pnewidls != nullptr)
        {
            ItemId.reset(pnewidls);
            return TRUE;
        }
        else
            return FALSE;
    }

    bool IsFolder() const
    {
        SFGAOF Flags = SHCIDS_BITMASK;
        GetAttributesOf(&Flags);
        return ((Flags & SFGAO_FOLDER) || (Flags & SFGAO_HASSUBFOLDER));
    }

    template <class I>
    HRESULT GetUIObjectOf(CComPtr<I>& Inteface) const
    {
        LPCITEMIDLIST    IdList[1] = { ItemId.get() };
        HRESULT hr = Parent->GetUIObjectOf(NULL, 1, IdList, __uuidof(Inteface), 0, (LPVOID *) &Inteface);
        return hr;
    }
};

static inline int Compare(const TreeItem& til, const TreeItem& tir, const CComPtr<IMalloc>& Malloc)
{
    bool isFolder1 = til.IsFolder();
    bool isFolder2 = tir.IsFolder();

#ifdef _DEBUG
    CString lName = til.GetDisplayNameOf(Malloc);
    CString rName = tir.GetDisplayNameOf(Malloc);
#else
    Malloc;
#endif

    int res = 0;
    if (isFolder1 == isFolder2)
    {
        HRESULT hr = til.Parent->CompareIDs(0, til.ItemId.get(), tir.ItemId.get());
        res = (short) HRESULT_CODE(hr);
        //res = _tcsicmp(lName.c_str(), rName.c_str());
    }
    else
        res = isFolder2 - isFolder1;
    return res;
}

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    IMalloc* Malloc((IMalloc*) lParamSort);

    TreeItem* ti1 = (TreeItem*) lParam1;
    TreeItem* ti2 = (TreeItem*) lParam2;

    return Compare(*ti1, *ti2, Malloc);
}

static inline int GetChildren(const CTreeCtrl& rTreeCtrl, HTREEITEM hItem)
{
    TVITEM item = {};
    item.hItem = hItem;
    item.mask = TVIF_CHILDREN;
    rTreeCtrl.GetItem(&item);
    return item.cChildren;
}

static inline void SetChildren(CTreeCtrl& rTreeCtrl, HTREEITEM hItem, int cChildren)
{
    TVITEM item = {};
    item.hItem = hItem;
    item.mask = TVIF_CHILDREN;
    item.cChildren = cChildren;
    rTreeCtrl.SetItem(&item);
}

//#define ID_NEW_FOLDER 1
#define ID_VIEW 2
#define MIN_SHELL_ID 1000
#define MAX_SHELL_ID 2000

static IContextMenu2* g_pIContext2 = 0;
static IContextMenu3* g_pIContext3 = 0;

static LRESULT CALLBACK ContextMenuHookWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
    switch (msg)
    {
    case WM_MENUSELECT:
        {
            // if this is a shell item, get it's descriptive text
            UINT uItem = (UINT) LOWORD(wp);
            if (0 == (MF_POPUP & HIWORD(wp))
                && uItem >= MIN_SHELL_ID && uItem <= MAX_SHELL_ID)
            {
                union
                {
                    char szBuf[MAX_PATH];
                    wchar_t szBufW[MAX_PATH];
                };

                if (g_pIContext2)
                    g_pIContext2->GetCommandString(uItem - MIN_SHELL_ID, GCS_HELPTEXT, NULL, szBuf, MAX_PATH - 1);

                // set the status bar text
                CFrameWnd* pMainWnd = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd());
                pMainWnd->SetMessageText(szBufW);
                return 0;
            }
            else
            {
                CFrameWnd* pMainWnd = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd());
                pMainWnd->SetMessageText(_T(""));
                return 0;
            }
        }
        break;

    default:
        if (g_pIContext3)
        {
            LRESULT ret = 0;
            if (g_pIContext3->HandleMenuMsg2(msg, wp, lp, &ret) == S_OK)
                return ret;
        }
        else if (g_pIContext2)
        {
            if (g_pIContext2->HandleMenuMsg(msg, wp, lp) == S_OK)
                return 0;
        }
        break;
    }

    // for all untreated messages, call the original wndproc
    return DefSubclassProc(hWnd, msg, wp, lp);
}

static void DoContextMenu(CWnd* pWnd, CComPtr<IContextMenu>& TheContextMenu, int Flags, int x, int y, BOOL bCanView, BOOL bIsFolder)
{
    CMenu    Menu;
    Menu.CreatePopupMenu();

    if (bIsFolder)
    {
        Menu.AppendMenu(MF_ENABLED | MF_STRING, ID_NEW_FOLDER, _T("New Folder"));
        // TODO Set an icon
        Menu.AppendMenu(MF_ENABLED | MF_SEPARATOR);
    }

    if (bCanView)
    {
        Menu.AppendMenu(MF_ENABLED | MF_STRING, ID_VIEW, _T("View"));
        Menu.SetDefaultItem(ID_VIEW);
    }

    TheContextMenu->QueryContextMenu(Menu, Menu.GetMenuItemCount(), MIN_SHELL_ID, MAX_SHELL_ID, Flags);

    CMINVOKECOMMANDINFOEX    Command = { sizeof(CMINVOKECOMMANDINFOEX) };
    Command.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
    Command.hwnd = pWnd->GetSafeHwnd();
    Command.nShow = SW_NORMAL;
    Command.ptInvoke.x = x;
    Command.ptInvoke.y = y;
    ClientToScreen(Command.hwnd, &Command.ptInvoke);
    if (GetKeyState(VK_CONTROL) < 0)
        Command.fMask |= CMIC_MASK_CONTROL_DOWN;
    if (GetKeyState(VK_SHIFT) < 0)
        Command.fMask |= CMIC_MASK_SHIFT_DOWN;

    CComPtr<IContextMenu2>    TheContextMenu2;
    TheContextMenu->QueryInterface(IID_PPV_ARGS(&TheContextMenu2));
    CComPtr<IContextMenu3>    TheContextMenu3;
    TheContextMenu->QueryInterface(IID_PPV_ARGS(&TheContextMenu3));
    if (!!TheContextMenu2 || !!TheContextMenu3)
    {
        SetWindowSubclass(pWnd->GetSafeHwnd(), ContextMenuHookWndProc, 0, 0);
        g_pIContext2 = TheContextMenu2;
        g_pIContext3 = TheContextMenu3;
    }

    POINT pos = { x, y };
    pWnd->ClientToScreen(&pos);
    int Cmd = Menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN, pos.x, pos.y, pWnd);

    if (Cmd > 0)
    {
        if (Cmd == ID_NEW_FOLDER)
        {
            pWnd->SendMessage(WM_COMMAND, ID_NEW_FOLDER);
        }
        else if (Cmd == ID_VIEW)
        {
            pWnd->SendMessage(WM_COMMAND, ID_EDIT_VIEW);
        }
        else
        {
            union
            {
                char szBuf[MAX_PATH];
                wchar_t szBufW[MAX_PATH];
            };
            szBuf[0] = '\0';
            szBufW[0] = '\0';
            if (!!TheContextMenu2)
                TheContextMenu2->GetCommandString(Cmd - MIN_SHELL_ID, GCS_VERB, NULL, szBuf, MAX_PATH - 1);
            if (wcscmp(szBufW, L"rename") == 0)
            {
                pWnd->SendMessage(WM_COMMAND, ID_EDIT_RENAME);
            }
            else
            {
                Command.lpVerb = MAKEINTRESOURCEA(Cmd - MIN_SHELL_ID);
                Command.lpVerbW = MAKEINTRESOURCEW(Cmd - MIN_SHELL_ID);
                pWnd->SendMessage(0);

                TheContextMenu->InvokeCommand((LPCMINVOKECOMMANDINFO) &Command);
            }
        }
    }

    if (!!TheContextMenu2 || !!TheContextMenu3)
        RemoveWindowSubclass(pWnd->GetSafeHwnd(), ContextMenuHookWndProc, 0);
    g_pIContext2 = 0;
    g_pIContext3 = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CFileView

CFileView::CFileView()
{
    m_hAccel = NULL;
    m_Notify = 0;
    m_pRootPidl = nullptr;
    HRESULT    hr = NOERROR;
    hr = SHGetMalloc(&m_Malloc);
}

CFileView::~CFileView()
{
    SHChangeNotifyDeregister(m_Notify);
}

BEGIN_MESSAGE_MAP(CFileView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
    ON_WM_PAINT()
    ON_WM_SETFOCUS()
    ON_COMMAND(ID_PROPERTIES, OnProperties)
    ON_UPDATE_COMMAND_UI(ID_PROPERTIES, OnUpdateFileSelected)
    ON_COMMAND(ID_SYNC, OnSync)
    ON_UPDATE_COMMAND_UI(ID_SYNC, OnUpdateActiveDocument)
    ON_COMMAND(ID_EDIT_RENAME, OnEditRename)
    ON_UPDATE_COMMAND_UI(ID_EDIT_RENAME, OnUpdateFileSelected)
    ON_COMMAND(ID_EDIT_VIEW, OnEditView)
    ON_UPDATE_COMMAND_UI(ID_EDIT_VIEW, OnUpdateFileSelected)
    ON_COMMAND(ID_NEW_FOLDER, OnNewFolder)
    ON_NOTIFY(TVN_ITEMEXPANDING, ID_FILE_VIEW_TREE, OnItemExpanding)
    ON_NOTIFY(TVN_DELETEITEM, ID_FILE_VIEW_TREE, OnDeleteItem)
    ON_NOTIFY(TVN_BEGINLABELEDIT, ID_FILE_VIEW_TREE, OnBeginLabelEdit)
    ON_NOTIFY(TVN_ENDLABELEDIT, ID_FILE_VIEW_TREE, OnEndLabelEdit)
    ON_NOTIFY(NM_DBLCLK, ID_FILE_VIEW_TREE, OnDblClick)
    ON_MESSAGE(MSG_SHELLCHANGE, OnShellChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CFileView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

    m_hAccel = LoadAccelerators(NULL, MAKEINTRESOURCE(IDR_EXPLORER));
    m_wndToolBar.SetAccel(m_hAccel);

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create view:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_EDITLABELS;

	if (!m_wndFileView.Create(dwViewStyle, rectDummy, this, ID_FILE_VIEW_TREE))
	{
		TRACE0("Failed to create file view\n");
		return -1;      // fail to create
	}

	// Load view images:
    HIMAGELIST ImlLarge, ImlSmall;
    Shell_GetImageLists(&ImlLarge, &ImlSmall);
    CImageList il;
    il.Attach(ImlSmall);
    m_wndFileView.SetImageList(&il, TVSIL_NORMAL);
    il.Detach();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_EXPLORER);
	m_wndToolBar.LoadToolBar(IDR_EXPLORER, 0, 0, TRUE /* Is locked */);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	// Fill in some static tree view data (dummy code, nothing magic here)
	FillFileView();
	AdjustLayout();

	return 0;
}

void CFileView::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CFileView::FillFileView()
{
    {
        SHChangeNotifyEntry cne;
        cne.fRecursive = TRUE;
        cne.pidl = m_pRootPidl.get();

        const LONG fEvents = SHCNE_CREATE | SHCNE_DELETE |
            SHCNE_MKDIR | SHCNE_RMDIR | SHCNE_RENAMEFOLDER |
            SHCNE_RENAMEITEM;
        //SHCNE_UPDATEITEM;     // TODO Look into this, not sure if handled properly at the moment
        //SHCNE_ATTRIBUTES;

        m_Notify = SHChangeNotifyRegister(GetSafeHwnd(),
            SHCNRF_ShellLevel | SHCNRF_NewDelivery,
            SHCNE_ALLEVENTS,
            MSG_SHELLCHANGE,
            1,
            &cne);
    }

#if 1
    TreeItem ti;
    /*HRESULT hr =*/ SHGetDesktopFolder(&ti.Parent);
    ti.ItemId = nullptr;
#else
    PIDLIST_ABSOLUTE pRootPidl = nullptr;
    /*HRESULT hr =*/ SHGetSpecialFolderLocation(GetSafeHwnd(), CSIDL_DRIVES, &pRootPidl);
    m_pRootPidl.reset(pRootPidl);

    TreeItem ti;
    ti.ItemId = nullptr;
#endif

#if 1
    if (m_pRootPidl != nullptr)
    {
        ti.Parent = GetFolder(m_pRootPidl.get());
        ti.ItemId = nullptr;
    }
    InsertChildren(TVI_ROOT, &ti);
#else
    if (m_pRootPidl != nullptr)
    {
        ti.Parent = GetParentFolder(m_pRootPidl);
        ti.ItemId = ILClone(ILFindLastID(m_pRootPidl));
        ILRemoveLastID(m_pRootPidl);
    }
    HTREEITEM hRoot = InsertChild(TVI_ROOT, ti.Parent, ti.ItemId);
    m_wndFileView.Expand(hRoot, TVE_EXPAND);
#endif
}

void CFileView::InsertChildren(HTREEITEM hNode, TreeItem* ti)
{
    if (m_wndFileView.GetChildItem(hNode) == NULL)
    {
        CComPtr<IShellFolder> Folder = ti->GetFolder();

        if (!!Folder)
            InsertChildren(Folder, hNode);
    }
}

void CFileView::InsertChildren(CComPtr<IShellFolder>& Folder, HTREEITEM hParent)
{
    CWaitCursor wc;

    bool children = false;
    CComPtr<IEnumIDList>    EnumIDList;
    HRESULT hr = Folder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_ENABLE_ASYNC, &EnumIDList);

    if (!!EnumIDList)
    {
        while (SUCCEEDED(hr) && (hr != S_FALSE))
        {
            LPITEMIDLIST    ItemId = NULL;
            hr = EnumIDList->Next(1, &ItemId, NULL);
            if (ItemId)
            {
                children = true;
#ifdef _DEBUG
                CString name = GetDisplayNameOf(Folder, ItemId, m_Malloc);
#endif
                SFGAOF AttrFlags = SHCIDS_BITMASK;
                LPCITEMIDLIST    IdList[1] = { ItemId };
                hr = Folder->GetAttributesOf(1, IdList, &AttrFlags);

                if (AttrFlags & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM))
                    InsertChild(hParent, Folder, PtrIDChild(ItemId));
            }
        }
    }

    if (!children)
        SetChildren(m_wndFileView, hParent, 0);
}

HTREEITEM CFileView::FindItem(HTREEITEM hParentItem, PCITEMID_CHILD pidl)
{
    ASSERT(ILIsChild(pidl));

    HTREEITEM hItem = m_wndFileView.GetChildItem(hParentItem);
    while (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

#ifdef _DEBUG
        CString name = ti->GetDisplayNameOf(m_Malloc);
#endif

        if (ti->ItemId == pidl)
            return hItem;

        hItem = m_wndFileView.GetNextSiblingItem(hItem);
    }

    return NULL;
}

HTREEITEM CFileView::FindItem(PCIDLIST_ABSOLUTE pidl, BOOL bExpandChildren)
{
    HTREEITEM hNode = TVI_ROOT;
    PtrIDAbsolute parentpidl(ILClone(m_pRootPidl.get()));

    HTREEITEM hChild = m_wndFileView.GetChildItem(hNode);
    while (hChild != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hChild);

#ifdef _DEBUG
        CString name = ti->GetDisplayNameOf(m_Malloc);
#endif

        PtrIDAbsolute childpidl = parentpidl + ti->ItemId;
        if (childpidl == pidl)
        {
            return hChild;
        }
        else if (ti->ItemId == nullptr || ILIsParent(childpidl.get(), pidl, FALSE))
        {
            if (bExpandChildren)
                m_wndFileView.Expand(hChild, TVE_EXPAND);

            hNode = hChild;
            hChild = m_wndFileView.GetChildItem(hNode);
            parentpidl = std::move(childpidl);
        }
        else
        {
            hChild = m_wndFileView.GetNextSiblingItem(hChild);
        }
    }

    return NULL;
}

HTREEITEM CFileView::FindParentItem(PCIDLIST_ABSOLUTE pidl)
{
    PtrIDAbsolute pparentidl(ILClone(pidl));
    ILRemoveLastID(pparentidl.get());

    HTREEITEM hNode = FindItem(pparentidl.get(), FALSE);

    return hNode;
}

HTREEITEM CFileView::InsertChild(HTREEITEM hParent, CComPtr<IShellFolder>& folder, PtrIDChild ItemId)
{
    ASSERT(ILIsChild(ItemId.get()));

    TreeItem* ti = new TreeItem;
    ti->Parent = folder;
    ti->ItemId = std::move(ItemId);

    CString name = ti->GetDisplayNameOf(m_Malloc);

    TVINSERTSTRUCT tvis = {};
    tvis.hParent = hParent;
    tvis.hInsertAfter = FindSortedPos(hParent, ti);
    //tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvis.item.pszText = (LPTSTR) (LPCTSTR) name;
    tvis.item.iImage = SHMapPIDLToSystemImageListIndex(ti->Parent, ti->ItemId.get(), &tvis.item.iSelectedImage);
    //tvis.item.iSelectedImage = tvis.item.iImage;
    tvis.item.cChildren = ti->IsFolder() ? 1 : 0;
    tvis.item.lParam = (LPARAM) ti;

    return m_wndFileView.InsertItem(&tvis);
}

HTREEITEM CFileView::FindSortedPos(HTREEITEM hParent, const TreeItem* tir)
{
    HTREEITEM hLastChild = TVI_FIRST;
    HTREEITEM hChild = m_wndFileView.GetChildItem(hParent);
    while (hChild != NULL)
    {
        const TreeItem* til = (TreeItem*)  m_wndFileView.GetItemData(hChild);
        //ASSERT(til->Parent == tir->Parent);

        if (Compare(*til, *tir, m_Malloc) > 0)
            return hLastChild;

        hLastChild = hChild;
        hChild = m_wndFileView.GetNextSiblingItem(hChild);
    }
    return TVI_LAST;
}

void CFileView::SortChildren(HTREEITEM hParent)
{
    TVSORTCB scb;
    scb.hParent = hParent;
    scb.lpfnCompare = CompareFunc;
    scb.lParam = (LPARAM) (IMalloc*) m_Malloc;
    m_wndFileView.SortChildrenCB(&scb);
}

void CFileView::OnDeleteItem(PCIDLIST_ABSOLUTE pidls)
{
    HTREEITEM hItem = FindItem(pidls, FALSE);
    if (hItem != NULL)
    {
        HTREEITEM hParentItem = m_wndFileView.GetParentItem(hItem);
        m_wndFileView.DeleteItem(hItem);

        if (hParentItem != NULL && m_wndFileView.GetChildItem(hParentItem) == NULL)
            SetChildren(m_wndFileView, hParentItem, 0);
    }
}

void CFileView::OnRenameItem(PCIDLIST_ABSOLUTE pidls, PCIDLIST_ABSOLUTE new_pidls)
{
    HTREEITEM hItem = FindItem(pidls, FALSE);
    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

        ti->ItemId = PtrIDChild(ILClone(ILFindLastID(new_pidls)));

        CString name = ti->GetDisplayNameOf(m_Malloc);

        TVITEM item = {};
        item.hItem = hItem;
        item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        item.pszText = (LPWSTR) name.GetString();
        item.iImage = SHMapPIDLToSystemImageListIndex(ti->Parent, ti->ItemId.get(), &item.iSelectedImage);
        //item.iSelectedImage = item.iImage;
        m_wndFileView.SetItem(&item);

        HTREEITEM hParentItem = m_wndFileView.GetParentItem(hItem);
        if (hParentItem != NULL)
            SortChildren(hParentItem);
    }
}

void CFileView::OnAddItem(PCIDLIST_ABSOLUTE pidls)
{
    HTREEITEM hParentItem = FindParentItem(pidls);
    if (hParentItem)
    {
        int cChildren = GetChildren(m_wndFileView, hParentItem);

        if (cChildren == 0 || (cChildren != 0 && m_wndFileView.GetChildItem(hParentItem) != NULL))
        {
            TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hParentItem);
            CComPtr<IShellFolder> Folder = ti->GetFolder();

            InsertChild(hParentItem, Folder, PtrIDChild(ILClone(ILFindLastID(pidls))));

            if (hParentItem != TVI_ROOT)
                SetChildren(m_wndFileView, hParentItem, 1);

            if (hParentItem != TVI_ROOT)
                m_wndFileView.Expand(hParentItem, TVE_EXPAND);
        }
    }
}

void CFileView::OnUpdateItem(PCIDLIST_ABSOLUTE pidls)
{
    HTREEITEM hParentItem = FindParentItem(pidls);
    if (hParentItem)
    {
        int cChildren = GetChildren(m_wndFileView, hParentItem);

        if (cChildren == 0 || (cChildren != 0 && m_wndFileView.GetChildItem(hParentItem) != NULL))
        {
            TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hParentItem);
            CComPtr<IShellFolder> Folder = ti->GetFolder();

            CComPtr<IEnumIDList>    EnumIDList;
            HRESULT hr = Folder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &EnumIDList);

            if (!!EnumIDList)
            {
                std::set<HTREEITEM> found;

                while (SUCCEEDED(hr) && (hr != S_FALSE))
                {
                    LPITEMIDLIST    ItemId = NULL;
                    hr = EnumIDList->Next(1, &ItemId, NULL);
                    if (ItemId != NULL)
                    {
                        HTREEITEM hItem = FindItem(hParentItem, ItemId);
#ifdef _DEBUG
                        CString name = GetDisplayNameOf(Folder, ItemId, m_Malloc);
#endif

                        if (hItem == NULL)
                            hItem = InsertChild(hParentItem, Folder, PtrIDChild(ILClone(ItemId)));

                        found.insert(hItem);
                    }
                }

                HTREEITEM hItem = m_wndFileView.GetChildItem(hParentItem);
                while (hItem != NULL)
                {
                    HTREEITEM hNextItem = m_wndFileView.GetNextSiblingItem(hItem);
                    if (found.find(hItem) == found.end())
                        m_wndFileView.DeleteItem(hItem);
                    hItem = hNextItem;
                }

                SetChildren(m_wndFileView, hParentItem, m_wndFileView.GetChildItem(hParentItem) == NULL ? 0 : 1);
            }
        }
    }
}

void CFileView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CTreeCtrl* pWndTree = (CTreeCtrl*) &m_wndFileView;
	ASSERT_VALID(pWndTree);

	if (pWnd != pWndTree)
	{
		CDockablePane::OnContextMenu(pWnd, point);
		return;
	}

	if (point != CPoint(-1, -1))
	{
		// Select clicked item:
		CPoint ptTree = point;
		pWndTree->ScreenToClient(&ptTree);

		UINT flags = 0;
        HTREEITEM hItem = pWndTree->HitTest(ptTree, &flags);
        pWndTree->SetFocus();
        if (hItem != NULL)
        {
            pWndTree->SelectItem(hItem);

            TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);
            CComPtr<IContextMenu>    TheContextMenu;
            /*HRESULT hr =*/ ti->GetUIObjectOf(TheContextMenu);
            if (!!TheContextMenu)
            {
                int Flags = /*CMF_EXPLORE |*/ CMF_NORMAL | CMF_CANRENAME | CMF_ITEMMENU;
                if (GetKeyState(VK_SHIFT) < 0)
                    Flags |= CMF_EXTENDEDVERBS;

                SFGAOF AttrFlags = SHCIDS_BITMASK;
                ti->GetAttributesOf(&AttrFlags);
                BOOL bCanView = (AttrFlags & SFGAO_FILESYSTEM) && !(AttrFlags & SFGAO_FOLDER);
                BOOL bIsFolder = (AttrFlags & SFGAO_FILESYSTEM) && (AttrFlags & SFGAO_FOLDER);

                ::DoContextMenu(this, TheContextMenu, Flags, ptTree.x, ptTree.y, bCanView, bIsFolder);
            }
        }
    }
}

void CFileView::AdjustLayout()
{
	if (GetSafeHwnd() == NULL)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndFileView.SetWindowPos(NULL, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CFileView::OnProperties()
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();
    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);
#ifdef _DEBUG
        CString name = ti->GetDisplayNameOf(m_Malloc);
#endif
        CComPtr<IContextMenu>    TheContextMenu;
        /*HRESULT hr =*/ ti->GetUIObjectOf(TheContextMenu);
        if (!!TheContextMenu)
        {
            CMINVOKECOMMANDINFOEX    Command = { sizeof(CMINVOKECOMMANDINFOEX) };
            Command.fMask = CMIC_MASK_UNICODE;
            Command.hwnd = GetSafeHwnd();
            Command.nShow = SW_NORMAL;
            if (GetKeyState(VK_CONTROL) < 0)
                Command.fMask |= CMIC_MASK_CONTROL_DOWN;
            if (GetKeyState(VK_SHIFT) < 0)
                Command.fMask |= CMIC_MASK_SHIFT_DOWN;

            Command.lpVerb = "properties";
            Command.lpVerbW = L"properties";
            SendMessage(0);

            TheContextMenu->InvokeCommand((LPCMINVOKECOMMANDINFO) &Command);
        }
    }
}

void CFileView::OnUpdateFileSelected(CCmdUI *pCmdUI)
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();
    pCmdUI->Enable(hItem != NULL);
}

void CFileView::OnUpdateActiveDocument(CCmdUI *pCmdUI)
{
    CDocument* pDoc = CRadDocManager::GetActiveDocument();
    pCmdUI->Enable(pDoc != nullptr && !pDoc->GetPathName().IsEmpty());
}

void CFileView::OnSync()
{
    CDocument* pDoc = CRadDocManager::GetActiveDocument();
    if (pDoc != nullptr)
    {
        const CString& path = pDoc->GetPathName();
        PIDLIST_ABSOLUTE tpidl = nullptr;
        /*HRESULT hr =*/ SHParseDisplayName(path, NULL, &tpidl, 0, 0);
        PtrIDAbsolute pidl(tpidl);

        HTREEITEM hNode = FindItem(pidl.get(), TRUE);
        if (hNode != NULL)
        {
            m_wndFileView.Select(hNode, TVGN_CARET);
            m_wndFileView.EnsureVisible(hNode);
        }
        else
        {
            AfxMessageBox(L"Cannot find file.", MB_OK | MB_ICONERROR);
        }
    }
}

void CFileView::OnEditRename()
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();
    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);
#ifdef _DEBUG
        CString name = ti->GetDisplayNameOf(m_Malloc);
#endif
        SFGAOF Flags = SHCIDS_COLUMNMASK;
        ti->GetAttributesOf(&Flags);
        if (Flags & SFGAO_CANRENAME)
            m_wndFileView.EditLabel(hItem);
    }
}

void CFileView::OnEditView()
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();

    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

        SFGAOF AttrFlags = SHCIDS_BITMASK;
        ti->GetAttributesOf(&AttrFlags);
        BOOL bCanView = (AttrFlags & SFGAO_FILESYSTEM) && !(AttrFlags & SFGAO_FOLDER);

        if (bCanView)
        {
            CString name = ti->GetDisplayNameOf(m_Malloc, SHGDN_FORPARSING);
            theApp.OpenDocumentFile(name);
        }
        else
        {
            AfxMessageBox(L"Cannot view folders.", MB_OK | MB_ICONERROR);
        }
    }
}

void CFileView::OnNewFolder()
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();

    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

        SFGAOF AttrFlags = SHCIDS_BITMASK;
        ti->GetAttributesOf(&AttrFlags);
        BOOL bCanNewFolder = (AttrFlags & SFGAO_FILESYSTEM) && (AttrFlags & SFGAO_FOLDER);

        if (bCanNewFolder)
        {
            CComPtr<IShellFolder> Folder = ti->GetFolder();

            CComQIPtr<IStorage> pStorage(Folder);
            if (pStorage)
            {
                CComPtr<IStorage> dummy;
                /*hr = */pStorage->CreateStorage(L"New Folder", STGM_FAILIFTHERE, 0, 0, &dummy);
                // TODO If exist try another name ie "New Folder (2)"
                // TODO Select and rename it???
            }
        }
        else
        {
            AfxMessageBox(L"Cannot create folder.", MB_OK | MB_ICONERROR);
        }
    }
}

void CFileView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_wndFileView.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CFileView::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_wndFileView.SetFocus();
}

void CFileView::OnItemExpanding(NMHDR* pHdr, LRESULT* pResult)
{
    LPNMTREEVIEW ntv = (LPNMTREEVIEW) pHdr;
    InsertChildren(ntv->itemNew.hItem, (TreeItem*) ntv->itemNew.lParam);
    *pResult = 0;
}

void CFileView::OnDeleteItem(NMHDR* pHdr, LRESULT* pResult)
{
    LPNMTREEVIEW ntv = (LPNMTREEVIEW) pHdr;
    TreeItem* ti = (TreeItem*) ntv->itemOld.lParam;
    delete ti;
    *pResult = 0;
}

void CFileView::OnBeginLabelEdit(NMHDR* pHdr, LRESULT* pResult)
{
    LPNMTVDISPINFO ntdi = (LPNMTVDISPINFO) pHdr;
    TreeItem* ti = (TreeItem*) ntdi->item.lParam;
#ifdef _DEBUG
    CString name = ti->GetDisplayNameOf(m_Malloc);
#endif

    SFGAOF Flags = SHCIDS_COLUMNMASK;
    ti->GetAttributesOf(&Flags);
    if (Flags & SFGAO_CANRENAME)
    {
        CEdit* pEdit = m_wndFileView.GetEditControl();
        if (pEdit != nullptr)
            pEdit->SetWindowText(ti->GetDisplayNameOf(m_Malloc, SHGDN_FOREDITING));
        *pResult = 0;
    }
    else
        *pResult = TRUE;
}

void CFileView::OnEndLabelEdit(NMHDR* pHdr, LRESULT* pResult)
{
    LPNMTVDISPINFO ntdi = (LPNMTVDISPINFO) pHdr;
    if (ntdi->item.pszText != nullptr)
    {
        TreeItem* ti = (TreeItem*) ntdi->item.lParam;
        if (ti->SetName(GetSafeHwnd(), ntdi->item.pszText, SHGDN_FOREDITING))
        {
            TVITEM item = {};
            item.hItem = ntdi->item.hItem;
            item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            item.pszText = ntdi->item.pszText;
            item.iImage = SHMapPIDLToSystemImageListIndex(ti->Parent, ti->ItemId.get(), &item.iSelectedImage);
            //item.iSelectedImage = item.iImage;
            m_wndFileView.SetItem(&item);

            HTREEITEM hParentItem = m_wndFileView.GetParentItem(ntdi->item.hItem);
            if (hParentItem != NULL)
                SortChildren(hParentItem);
        }
    }
    *pResult = 0;
}

void CFileView::OnDblClick(NMHDR* /*pHdr*/, LRESULT* pResult)
{
    DWORD msgpos = GetMessagePos();
    POINT pt;
    POINTSTOPOINT(pt, msgpos);
    m_wndFileView.ScreenToClient(&pt);

    UINT nFlags = 0;
    HTREEITEM hItem = m_wndFileView.HitTest(pt, &nFlags);
    if (hItem != NULL && nFlags & TVHT_ONITEM)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

        SFGAOF Flags = SHCIDS_BITMASK;
        ti->GetAttributesOf(&Flags);
        if (Flags & SFGAO_STREAM)
        {
            CString name = ti->GetDisplayNameOf(m_Malloc, SHGDN_FORPARSING);
            theApp.OpenDocumentFile(name);
        }
    }
    *pResult = 0;
}

LRESULT CFileView::OnShellChange(WPARAM wParam, LPARAM lParam)
{
    PIDLIST_ABSOLUTE* pidls = nullptr;
    LONG wEventId = 0;
    HANDLE hLock = SHChangeNotification_Lock((HANDLE) wParam, (DWORD) lParam, &pidls, &wEventId);

    if (pidls[0] != nullptr)
    {
        CShellChanged sc;
        sc.wEventId = wEventId;

        CComPtr<IShellFolder>    Desktop;
        SHGetDesktopFolder(&Desktop);
        sc.strName = GetDisplayNameOf(Desktop, pidls[0], m_Malloc, SHGDN_FORPARSING);

        if (wEventId & SHCNE_DELETE || wEventId & SHCNE_RMDIR)
            // TODO What to do if deleting root
            OnDeleteItem(pidls[0]);

        if (wEventId & SHCNE_RENAMEITEM || wEventId & SHCNE_RENAMEFOLDER)
        {
            // TODO What to do if renaming root
            sc.strNewName = GetDisplayNameOf(Desktop, pidls[1], m_Malloc, SHGDN_FORPARSING);
            OnRenameItem(pidls[0], pidls[1]);
        }

        if (wEventId & SHCNE_CREATE || wEventId & SHCNE_MKDIR)
            OnAddItem(pidls[0]);

        if (wEventId & SHCNE_UPDATEITEM || wEventId & SHCNE_UPDATEDIR)
            OnUpdateItem(pidls[0]);

        CRadDocManager* pDocManager = DYNAMIC_DOWNCAST(CRadDocManager, theApp.m_pDocManager);
        pDocManager->UpdateAllViews(nullptr, HINT_SHELL_CHANGED, &sc);
    }

    SHChangeNotification_Unlock(hLock);
    return 0;
}


BOOL CFileView::PreTranslateMessage(MSG* pMsg)
{
    if (m_hAccel != NULL && pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST && m_wndFileView.GetEditControl() == NULL)
        return ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg);

    return CDockablePane::PreTranslateMessage(pMsg);
}
