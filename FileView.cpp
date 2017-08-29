
#include "stdafx.h"
#include "mainfrm.h"
#include "FileView.h"
#include "Resource.h"
#include "RadNotepad.h"
#include "RadDocManager.h"
#include "MainFrm.h"
#include <set>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define ID_FILE_VIEW_TREE 4

#define MSG_SHELLCHANGE (WM_USER + 117)

struct TreeItem
{
    CComPtr<IShellFolder>     Parent;
    PITEMID_CHILD    ItemId;
};

static inline CString GetCLSIDName(const wchar_t* path)
{
    CRegKey reg, object;
    reg.Open(HKEY_CLASSES_ROOT, TEXT("CLSID"), KEY_READ);
#if UNICODE
    object.Open(reg, path, KEY_READ);
#else
    object.Open(reg, ToTString(path).c_str(), KEY_READ);
#endif
    CString value;
    if (object.m_hKey != NULL)
    {
        ULONG nLength = 0;
        object.QueryStringValue((TCHAR*) nullptr, nullptr, &nLength);
        object.QueryStringValue((TCHAR*) nullptr, value.GetBufferSetLength(nLength), &nLength);
        value.ReleaseBuffer();
    }
    return value;
}

static inline CComPtr<IShellFolder> GetParentFolder(LPITEMIDLIST pidl)
{
    HRESULT    hr = NOERROR;

    CComPtr<IShellFolder>    Desktop;
    hr = SHGetDesktopFolder(&Desktop);

    CComPtr<IShellFolder>    Parent;

    LPITEMIDLIST parent_pidl = ILClone(pidl);
    ILRemoveLastID(parent_pidl);

    if (parent_pidl && !ILIsEmpty(parent_pidl))
        Desktop->BindToObject(parent_pidl, 0, IID_IShellFolder, (LPVOID *) &Parent);
    else
        Parent = Desktop;

    if (parent_pidl)
        ILFree(parent_pidl);

    return Parent;
}

static inline CComPtr<IShellFolder> GetFolder(const CComPtr<IShellFolder>& Parent, PCITEMID_CHILD ItemId)
{
    if (ItemId == nullptr)
        return  Parent;
    else
    {
        CComPtr<IShellFolder> Folder;
        Parent->BindToObject(ItemId, 0, IID_IShellFolder, (LPVOID *) &Folder);
        return Folder;
    }
}

static inline HRESULT GetAttributesOf(const CComPtr<IShellFolder>& Parent, PCITEMID_CHILD ItemId, SFGAOF *Flags)
{
    HRESULT    hr = NOERROR;
    LPCITEMIDLIST    IdList[1] = { ItemId };
    hr = Parent->GetAttributesOf(1, IdList, Flags);
    return hr;
}

static inline bool IsFolder(const CComPtr<IShellFolder>& Parent, PCITEMID_CHILD ItemId)
{
    SFGAOF Flags = SHCIDS_BITMASK;
    GetAttributesOf(Parent, ItemId, &Flags);
    return ((Flags & SFGAO_FOLDER) || (Flags & SFGAO_HASSUBFOLDER));
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

template <class I>
HRESULT GetUIObjectOf(const CComPtr<IShellFolder>& Folder, PCITEMID_CHILD ItemId, CComPtr<I>& Inteface)
{
    LPCITEMIDLIST    IdList[1] = { ItemId };
    HRESULT hr = Folder->GetUIObjectOf(NULL, 1, IdList, __uuidof(Inteface), 0, (LPVOID *) &Inteface);
    return hr;
}

static inline int Compare(const CComPtr<IShellFolder>& Parent, PCITEMID_CHILD ItemId1, PCITEMID_CHILD ItemId2, const CComPtr<IMalloc>& Malloc)
{
    bool isFolder1 = IsFolder(Parent, ItemId1);
    bool isFolder2 = IsFolder(Parent, ItemId2);

#ifdef _DEBUG
    CString lName = GetDisplayNameOf(Parent, ItemId1, Malloc);
    CString rName = GetDisplayNameOf(Parent, ItemId2, Malloc);
#else
    Malloc;
#endif

    int res = 0;
    if (isFolder1 == isFolder2)
    {
        HRESULT hr = Parent->CompareIDs(0, ItemId1, ItemId2);
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

    return Compare(ti1->Parent, ti1->ItemId, ti2->ItemId, Malloc);
}

#define ID_VIEW 1
#define MIN_SHELL_ID 2
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
                CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
                pMainWnd->SetMessageText(szBufW);
                return 0;
            }
            else
            {
                CMainFrame* pMainWnd = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
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

        if (g_pIContext2)
        {
            if (g_pIContext2->HandleMenuMsg(msg, wp, lp) == S_OK)
                return 0;
        }
        break;
    }

    // for all untreated messages, call the original wndproc
    return DefSubclassProc(hWnd, msg, wp, lp);
}

static void DoContextMenu(CWnd* pWnd, CComPtr<IContextMenu>& TheContextMenu, int Flags, int x, int y, BOOL bCanView)
{
    CMenu    Menu;
    Menu.CreatePopupMenu();

    if (bCanView)
    {
        Menu.AppendMenu(MF_ENABLED | MF_STRING, ID_VIEW, _T("View"));
        Menu.SetDefaultItem(ID_VIEW);
    }

    TheContextMenu->QueryContextMenu(Menu, Menu.GetMenuItemCount(), MIN_SHELL_ID, MAX_SHELL_ID, Flags);

    CMINVOKECOMMANDINFOEX    Command;
    ZeroMemory(&Command, sizeof(Command));
    Command.cbSize = sizeof(Command);
    Command.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
    Command.hwnd = pWnd->GetSafeHwnd();
    Command.nShow = SW_NORMAL;
    Command.ptInvoke.x = x;
    Command.ptInvoke.y = y;
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
        if (Cmd == ID_VIEW)
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
    HIMAGELIST ImlLarge, ImlSmall;
    Shell_GetImageLists(&ImlLarge, &ImlSmall);
    m_FileViewImages.Attach(ImlSmall);
}

CFileView::~CFileView()
{
    SHChangeNotifyDeregister(m_Notify);
    ILFree(m_pRootPidl);
    m_FileViewImages.Detach();
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
    m_wndFileView.SetImageList(&m_FileViewImages, TVSIL_NORMAL);

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
#if 0
	HTREEITEM hRoot = m_wndFileView.InsertItem(_T("FakeApp files"), 0, 0);
	m_wndFileView.SetItemState(hRoot, TVIS_BOLD, TVIS_BOLD);

	HTREEITEM hSrc = m_wndFileView.InsertItem(_T("FakeApp Source Files"), 0, 0, hRoot);

	m_wndFileView.InsertItem(_T("FakeApp.cpp"), 1, 1, hSrc);
	m_wndFileView.InsertItem(_T("FakeApp.rc"), 1, 1, hSrc);
	m_wndFileView.InsertItem(_T("FakeAppDoc.cpp"), 1, 1, hSrc);
	m_wndFileView.InsertItem(_T("FakeAppView.cpp"), 1, 1, hSrc);
	m_wndFileView.InsertItem(_T("MainFrm.cpp"), 1, 1, hSrc);
	m_wndFileView.InsertItem(_T("StdAfx.cpp"), 1, 1, hSrc);

	HTREEITEM hInc = m_wndFileView.InsertItem(_T("FakeApp Header Files"), 0, 0, hRoot);

	m_wndFileView.InsertItem(_T("FakeApp.h"), 2, 2, hInc);
	m_wndFileView.InsertItem(_T("FakeAppDoc.h"), 2, 2, hInc);
	m_wndFileView.InsertItem(_T("FakeAppView.h"), 2, 2, hInc);
	m_wndFileView.InsertItem(_T("Resource.h"), 2, 2, hInc);
	m_wndFileView.InsertItem(_T("MainFrm.h"), 2, 2, hInc);
	m_wndFileView.InsertItem(_T("StdAfx.h"), 2, 2, hInc);

	HTREEITEM hRes = m_wndFileView.InsertItem(_T("FakeApp Resource Files"), 0, 0, hRoot);

	m_wndFileView.InsertItem(_T("FakeApp.ico"), 2, 2, hRes);
	m_wndFileView.InsertItem(_T("FakeApp.rc2"), 2, 2, hRes);
	m_wndFileView.InsertItem(_T("FakeAppDoc.ico"), 2, 2, hRes);
	m_wndFileView.InsertItem(_T("FakeToolbar.bmp"), 2, 2, hRes);

	m_wndFileView.Expand(hRoot, TVE_EXPAND);
	m_wndFileView.Expand(hSrc, TVE_EXPAND);
	m_wndFileView.Expand(hInc, TVE_EXPAND);
#else
    TreeItem* ti = new TreeItem;
    ti->ItemId = nullptr;

#if 0
    HRESULT    hr = NOERROR;
    hr = SHGetDesktopFolder(&ti->Parent);
    CString name = GetCLSIDName(TEXT("{00021400-0000-0000-C000-000000000046}"));
    if (name.IsEmpty())
        name = _T("Desktop");
#else
    HRESULT    hr = NOERROR;
    hr = SHGetSpecialFolderLocation(GetSafeHwnd(), CSIDL_DRIVES, &m_pRootPidl);
    ti->Parent = GetParentFolder(m_pRootPidl);
    ti->ItemId = ILClone(ILFindLastID(m_pRootPidl));
    ILRemoveLastID(m_pRootPidl);
    CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
#endif

    {
        SHChangeNotifyEntry cne;
        cne.fRecursive = TRUE;
        cne.pidl = m_pRootPidl;

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

    TVINSERTSTRUCT tvis = {};
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvis.item.pszText = (LPTSTR) (LPCTSTR) name;
    tvis.item.iImage = SHMapPIDLToSystemImageListIndex(ti->Parent, ti->ItemId, &tvis.item.iSelectedImage);
    //tvis.item.iSelectedImage = tvis.item.iImage;
    tvis.item.cChildren = IsFolder(ti->Parent, ti->ItemId) ? 1 : 0;
    tvis.item.lParam = (LPARAM) ti;

    HTREEITEM hRoot = m_wndFileView.InsertItem(&tvis);
    m_wndFileView.Expand(hRoot, TVE_EXPAND);
#endif
}

void CFileView::InsertChildren(HTREEITEM hNode, TreeItem* ti)
{
    if (m_wndFileView.GetChildItem(hNode) == NULL)
    {
        CComPtr<IShellFolder>    Folder;
        if (ti->ItemId == nullptr)
            Folder = ti->Parent;
        else
            ti->Parent->BindToObject(ti->ItemId, 0, IID_IShellFolder, (LPVOID *) &Folder);

        if (!!Folder)
            InsertChildren(Folder, hNode);
    }
}

void CFileView::InsertChildren(CComPtr<IShellFolder>& Folder, HTREEITEM hParent)
{
    CWaitCursor wc;

    bool children = false;
    CComPtr<IEnumIDList>    EnumIDList;
    HRESULT hr = Folder->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &EnumIDList);

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
                InsertChild(hParent, Folder, ItemId);
            }
        }
    }

    if (!children)
    {
        TVITEM item = {};
        item.hItem = hParent;
        item.mask = TVIF_CHILDREN;
        m_wndFileView.SetItem(&item);
    }
}

HTREEITEM CFileView::FindItem(HTREEITEM hParentItem, PCITEMID_CHILD pidl)
{
    ASSERT(ILIsChild(pidl));

    HTREEITEM hItem = m_wndFileView.GetChildItem(hParentItem);
    while (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

#ifdef _DEBUG
        CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
#endif

        if (ILIsEqual(ti->ItemId, pidl))
            return hItem;

        hItem = m_wndFileView.GetNextSiblingItem(hItem);
    }

    return NULL;
}

HTREEITEM CFileView::FindItem(PCIDLIST_RELATIVE pidl, BOOL bExpandChildren)
{
    HTREEITEM hNode = m_wndFileView.GetRootItem();
    TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hNode);
    if (ILIsParent(ti->ItemId, pidl, FALSE))
    {
        LPITEMIDLIST parentpidl = ILClone(ti->ItemId);
        HTREEITEM hChild = m_wndFileView.GetChildItem(hNode);
        while (hChild != NULL)
        {
            /*TreeItem**/ ti = (TreeItem*) m_wndFileView.GetItemData(hChild);

#ifdef _DEBUG
            CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
#endif

            LPITEMIDLIST childpidl = ILCombine(parentpidl, ti->ItemId);
            if (ILIsParent(childpidl, pidl, FALSE))
            {
                if (bExpandChildren)
                    m_wndFileView.Expand(hChild, TVE_EXPAND);

                hNode = hChild;
                hChild = m_wndFileView.GetChildItem(hNode);
                ILFree(parentpidl);
                parentpidl = childpidl;

                if (ILIsEqual(parentpidl, pidl))
                {
                    ILFree(parentpidl);
                    return hNode;
                }
            }
            else
            {
                ILFree(childpidl);

                hChild = m_wndFileView.GetNextSiblingItem(hChild);
            }
        }
        ILFree(parentpidl);
    }

    return NULL;
}

HTREEITEM CFileView::FindParentItem(PCIDLIST_RELATIVE pidl)
{
    PIDLIST_RELATIVE pparentidl = ILClone(pidl);
    ILRemoveLastID(pparentidl);

    HTREEITEM hNode = FindItem(pparentidl, FALSE);

    ILFree(pparentidl);

    return hNode;
}

HTREEITEM CFileView::InsertChild(HTREEITEM hParent, CComPtr<IShellFolder>& folder, PITEMID_CHILD ItemId)
{
    ASSERT(ILIsChild(ItemId));

    TreeItem* ti = new TreeItem;
    ti->Parent = folder;
    ti->ItemId = ItemId;

    CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);

    TVINSERTSTRUCT tvis = {};
    tvis.hParent = hParent;
    tvis.hInsertAfter = FindSortedPos(hParent, ti);
    //tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvis.item.pszText = (LPTSTR) (LPCTSTR) name;
    tvis.item.iImage = SHMapPIDLToSystemImageListIndex(ti->Parent, ti->ItemId, &tvis.item.iSelectedImage);
    //tvis.item.iSelectedImage = tvis.item.iImage;
    tvis.item.cChildren = IsFolder(ti->Parent, ti->ItemId) ? 1 : 0;
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

        if (Compare(til->Parent, til->ItemId, tir->ItemId, m_Malloc) > 0)
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

void CFileView::OnDeleteItem(PCIDLIST_RELATIVE pidls)
{
    HTREEITEM hItem = FindItem(pidls, FALSE);
    if (hItem != NULL)
    {
        HTREEITEM hParentItem = m_wndFileView.GetParentItem(hItem);
        m_wndFileView.DeleteItem(hItem);

        if (hParentItem != NULL && m_wndFileView.GetChildItem(hParentItem) == NULL)
        {
            TVITEM item;
            ZeroMemory(&item, sizeof(item));
            item.hItem = hParentItem;
            item.mask = TVIF_CHILDREN;
            item.cChildren = 0;
            m_wndFileView.SetItem(&item);
        }
    }
}

void CFileView::OnRenameItem(PCIDLIST_RELATIVE pidls, PCIDLIST_RELATIVE new_pidls)
{
    HTREEITEM hItem = FindItem(pidls, FALSE);
    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);

        ILFree(ti->ItemId);
        ti->ItemId = ILClone(ILFindLastID(new_pidls));

        CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);

        TVITEM item;
        ZeroMemory(&item, sizeof(item));
        item.hItem = hItem;
        item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        item.pszText = (LPWSTR) name.GetString();
        item.iImage = SHMapPIDLToSystemImageListIndex(ti->Parent, ti->ItemId, &item.iSelectedImage);
        //item.iSelectedImage = item.iImage;
        m_wndFileView.SetItem(&item);

        HTREEITEM hParentItem = m_wndFileView.GetParentItem(hItem);
        if (hParentItem != NULL)
            SortChildren(hParentItem);
    }
}

void CFileView::OnAddItem(PCIDLIST_RELATIVE pidls)
{
    HTREEITEM hParentItem = FindParentItem(pidls);
    if (hParentItem)
    {
        TVITEM item;
        ZeroMemory(&item, sizeof(item));
        item.hItem = hParentItem;
        item.mask = TVIF_CHILDREN;
        if (hParentItem != TVI_ROOT)
            m_wndFileView.GetItem(&item);

        if (item.cChildren == 0 || (item.cChildren != 0 && m_wndFileView.GetChildItem(hParentItem) != NULL))
        {
            TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hParentItem);
            CComPtr<IShellFolder> Folder = GetFolder(ti->Parent, ti->ItemId);

            InsertChild(hParentItem, Folder, ILClone(ILFindLastID(pidls)));

            if (hParentItem != TVI_ROOT)
            {
                item.cChildren = 1;
                m_wndFileView.SetItem(&item);
            }

            if (hParentItem != TVI_ROOT)
                m_wndFileView.Expand(hParentItem, TVE_EXPAND);
        }
    }
}

void CFileView::OnUpdateItem(PCIDLIST_RELATIVE pidls)
{
    HTREEITEM hParentItem = FindParentItem(pidls);
    if (hParentItem)
    {
        TVITEM item;
        ZeroMemory(&item, sizeof(item));
        item.hItem = hParentItem;
        item.mask = TVIF_CHILDREN;
        if (hParentItem != TVI_ROOT)
            m_wndFileView.GetItem(&item);

        if (item.cChildren == 0 || (item.cChildren != 0 && m_wndFileView.GetChildItem(hParentItem) != NULL))
        {
            TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hParentItem);
            CComPtr<IShellFolder> Folder = GetFolder(ti->Parent, ti->ItemId);

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
                            hItem = InsertChild(hParentItem, Folder, ILClone(ItemId));

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

                TVITEM item;
                ZeroMemory(&item, sizeof(item));
                item.hItem = hParentItem;
                item.mask = TVIF_CHILDREN;
                item.cChildren = m_wndFileView.GetChildItem(hParentItem) == NULL ? 0 : 1;
                m_wndFileView.SetItem(&item);
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
            /*HRESULT hr =*/ GetUIObjectOf(ti->Parent, ti->ItemId, TheContextMenu);
            if (!!TheContextMenu)
            {
                int Flags = /*CMF_EXPLORE |*/ CMF_NORMAL | CMF_CANRENAME | CMF_ITEMMENU;
                if (GetKeyState(VK_SHIFT) < 0)
                    Flags |= CMF_EXTENDEDVERBS;

                SFGAOF AttrFlags = SHCIDS_BITMASK;
                GetAttributesOf(ti->Parent, ti->ItemId, &AttrFlags);
                BOOL bCanView = (AttrFlags & SFGAO_FILESYSTEM) && !(AttrFlags & SFGAO_FOLDER);

                ::DoContextMenu(this, TheContextMenu, Flags, ptTree.x, ptTree.y, bCanView);
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
        CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
#endif
        CComPtr<IContextMenu>    TheContextMenu;
        /*HRESULT hr =*/ GetUIObjectOf(ti->Parent, ti->ItemId, TheContextMenu);
        if (!!TheContextMenu)
        {
            CMINVOKECOMMANDINFOEX    Command;
            ZeroMemory(&Command, sizeof(Command));
            Command.cbSize = sizeof(Command);
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
    pCmdUI->Enable(pDoc != nullptr);
}

void CFileView::OnSync()
{
    CDocument* pDoc = CRadDocManager::GetActiveDocument();
    if (pDoc != nullptr)
    {
        const CString& path = pDoc->GetPathName();
        LPITEMIDLIST pidl = nullptr;
        /*HRESULT hr =*/ SHParseDisplayName(path, NULL, &pidl, 0, 0);

        HTREEITEM hNode = FindItem(pidl, TRUE);
        if (hNode != NULL)
        {
            m_wndFileView.Select(hNode, TVGN_CARET);
            m_wndFileView.EnsureVisible(hNode);
        }

        ILFree(pidl);
    }
}

void CFileView::OnEditRename()
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();
    if (hItem != NULL)
    {
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hItem);
#ifdef _DEBUG
        CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
#endif
        SFGAOF Flags = SHCIDS_COLUMNMASK;
        GetAttributesOf(ti->Parent, ti->ItemId, &Flags);
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
        CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc, SHGDN_FORPARSING);
        theApp.OpenDocumentFile(name);
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
    if (ti->ItemId != nullptr)
        ILFree(ti->ItemId);
    delete ti;
    *pResult = 0;
}

void CFileView::OnBeginLabelEdit(NMHDR* pHdr, LRESULT* pResult)
{
    LPNMTVDISPINFO ntdi = (LPNMTVDISPINFO) pHdr;
    TreeItem* ti = (TreeItem*) ntdi->item.lParam;
#ifdef _DEBUG
    CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
#endif

    SFGAOF Flags = SHCIDS_COLUMNMASK;
    GetAttributesOf(ti->Parent, ti->ItemId, &Flags);
    if (Flags & SFGAO_CANRENAME)
    {
        CEdit* pEdit = m_wndFileView.GetEditControl();
        if (pEdit != nullptr)
        {
            pEdit->SetWindowText(GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc, SHGDN_FOREDITING));
        }
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
        LPITEMIDLIST pnewidls = nullptr;
        ti->Parent->SetNameOf(GetSafeHwnd(), ti->ItemId, ntdi->item.pszText, SHGDN_FOREDITING, &pnewidls);
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
        CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc, SHGDN_FORPARSING);
        theApp.OpenDocumentFile(name);
    }
    *pResult = 0;
}

LRESULT CFileView::OnShellChange(WPARAM wParam, LPARAM lParam)
{
    PIDLIST_ABSOLUTE* pidls = nullptr;
    LONG wEventId = 0;
    HANDLE hLock = SHChangeNotification_Lock((HANDLE) wParam, (DWORD) lParam, &pidls, &wEventId);

    PCIDLIST_RELATIVE child_pidl = m_pRootPidl ? ILFindChild(m_pRootPidl, pidls[0]) : pidls[0];
    if (child_pidl != nullptr)
    {
#ifdef _DEBUG
        CComPtr<IShellFolder>    Desktop;
        SHGetDesktopFolder(&Desktop);
        CString Name = GetDisplayNameOf(Desktop, pidls[0], m_Malloc, SHGDN_FORPARSING);
#endif

        if (wEventId & SHCNE_DELETE || wEventId & SHCNE_RMDIR)
        {
            // TODO What to do if deleting root
            OnDeleteItem(child_pidl);
        }

        if (wEventId & SHCNE_RENAMEITEM || wEventId & SHCNE_RENAMEFOLDER)
        {
            // TODO What to do if renaming root
            PCIDLIST_RELATIVE new_child_pidl = m_pRootPidl ? ILFindChild(m_pRootPidl, pidls[1]) : pidls[1];
            OnRenameItem(child_pidl, new_child_pidl);
        }

        if (wEventId & SHCNE_CREATE || wEventId & SHCNE_MKDIR)
        {
            OnAddItem(child_pidl);
        }

        if (wEventId & SHCNE_UPDATEITEM || wEventId & SHCNE_UPDATEDIR)
        {
            OnUpdateItem(child_pidl);
        }
    }

    SHChangeNotification_Unlock(hLock);
    return 0;
}


BOOL CFileView::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
    {
        HACCEL hAccel = m_hAccel;
        return hAccel != NULL &&  ::TranslateAccelerator(m_hWnd, hAccel, pMsg);
    }

    return CDockablePane::PreTranslateMessage(pMsg);
}
