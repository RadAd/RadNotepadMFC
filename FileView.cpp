
#include "stdafx.h"
#include "mainfrm.h"
#include "FileView.h"
#include "Resource.h"
#include "RadNotepad.h"
#include "RadDocManager.h"
#include "MainFrm.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// TODO
// Detect filesystem changes
// Add item to context menu to open in RadNotepad - view

#define ID_FILE_VIEW_TREE 4

struct TreeItem
{
    CComPtr<IShellFolder>     Parent;
    LPITEMIDLIST    ItemId;
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
    if (parent_pidl && parent_pidl->mkid.cb)
    {
        Desktop->BindToObject(parent_pidl, 0, IID_IShellFolder, (LPVOID *) &Parent);
    }
    else
    {
        Parent = Desktop;
    }
    if (parent_pidl)
        ILFree(parent_pidl);

    return Parent;
}

static inline HRESULT GetAttributesOf(const CComPtr<IShellFolder>& Parent, LPCITEMIDLIST ItemId, SFGAOF *Flags)
{
    HRESULT    hr = NOERROR;
    LPCITEMIDLIST    IdList[1] = { ItemId };
    hr = Parent->GetAttributesOf(1, IdList, Flags);
    return hr;
}

static inline bool IsFolder(const CComPtr<IShellFolder>& Parent, LPCITEMIDLIST ItemId)
{
    SFGAOF Flags = SHCIDS_BITMASK;
    GetAttributesOf(Parent, ItemId, &Flags);
    return ((Flags & SFGAO_FOLDER) || (Flags & SFGAO_HASSUBFOLDER));
}

static inline CString GetStr(const CComPtr<IMalloc>& Malloc, LPCITEMIDLIST pidl, STRRET Str)
{
    // TODO Use StrRetToBuf
    CString ret;
    switch (Str.uType)
    {
    case STRRET_CSTR:
        //ret = ToTString(Str.cStr);
        ret = Str.cStr;
        break;

    case STRRET_OFFSET:
        //ret = ToTString((char *) pidl + Str.uOffset);
        ret = (char *) pidl + Str.uOffset;
        break;

    case STRRET_WSTR:
        {
            //ret = ToTString(Str.pOleStr);
            ret = Str.pOleStr;
            Malloc->Free(Str.pOleStr);
        }
        break;

    default:
        break;
    }
    return ret;
}

static inline CString GetDisplayNameOf(const CComPtr<IShellFolder>& Folder, LPCITEMIDLIST ItemId, const CComPtr<IMalloc>& Malloc, SHGDNF Flags = SHGDN_NORMAL)
{
    STRRET    Name = {};
    HRESULT hr = Folder->GetDisplayNameOf(ItemId, Flags, &Name);
    if (SUCCEEDED(hr))
        return GetStr(Malloc, ItemId, Name);
    else
        return CString();
}

template <class I>
HRESULT GetUIObjectOf(const CComPtr<IShellFolder>& Folder, LPCITEMIDLIST ItemId, CComPtr<I>& Inteface)
{
    LPCITEMIDLIST    IdList[1] = { ItemId };
    HRESULT hr = Folder->GetUIObjectOf(NULL, 1, IdList, __uuidof(Inteface), 0, (LPVOID *) &Inteface);
    return hr;
}

static inline int Compare(const CComPtr<IShellFolder>& Parent, LPCITEMIDLIST ItemId1, LPCITEMIDLIST ItemId2, const CComPtr<IMalloc>& Malloc)
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

#define MIN_SHELL_ID 1
#define MAX_SHELL_ID 2000
#define ID_EDIT_RENAME (1000)

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

static void DoContextMenu(CWnd* pWnd, CComPtr<IContextMenu>& TheContextMenu, int Flags, int x, int y)
{
    CMenu    Menu;
    Menu.CreatePopupMenu();

    TheContextMenu->QueryContextMenu(Menu, 0, MIN_SHELL_ID, MAX_SHELL_ID, Flags);

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

    if (!!TheContextMenu2 || !!TheContextMenu3)
        RemoveWindowSubclass(pWnd->GetSafeHwnd(), ContextMenuHookWndProc, 0);
    g_pIContext2 = 0;
    g_pIContext3 = 0;
}


/////////////////////////////////////////////////////////////////////////////
// CFileView

CFileView::CFileView()
{
    HRESULT    hr = NOERROR;
    hr = SHGetMalloc(&m_Malloc);
    HIMAGELIST ImlLarge, ImlSmall;
    Shell_GetImageLists(&ImlLarge, &ImlSmall);
    m_FileViewImages.Attach(ImlSmall);
}

CFileView::~CFileView()
{
    m_FileViewImages.Detach();
}

BEGIN_MESSAGE_MAP(CFileView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_PROPERTIES, OnProperties)
    ON_COMMAND(ID_SYNC, OnSync)
    ON_COMMAND(ID_OPEN, OnFileOpen)
	ON_COMMAND(ID_OPEN_WITH, OnFileOpenWith)
	ON_COMMAND(ID_DUMMY_COMPILE, OnDummyCompile)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
    ON_COMMAND(ID_EDIT_RENAME, OnEditRename)
    ON_WM_PAINT()
	ON_WM_SETFOCUS()
    ON_NOTIFY(TVN_ITEMEXPANDING, ID_FILE_VIEW_TREE, OnItemExpanding)
    ON_NOTIFY(TVN_DELETEITEM, ID_FILE_VIEW_TREE, OnDeleteItem)
    ON_NOTIFY(TVN_BEGINLABELEDIT, ID_FILE_VIEW_TREE, OnBeginLabelEdit)
    ON_NOTIFY(TVN_ENDLABELEDIT, ID_FILE_VIEW_TREE, OnEndLabelEdit)
    ON_NOTIFY(NM_DBLCLK, ID_FILE_VIEW_TREE, OnDblClick)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CFileView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

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
    LPITEMIDLIST    pIdl = nullptr;
    hr = SHGetSpecialFolderLocation(GetSafeHwnd(), CSIDL_DRIVES, &pIdl);
    ti->Parent = GetParentFolder(pIdl);
    ti->ItemId = ILClone(ILFindLastID(pIdl));
    CString name = GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc);
    ILFree(pIdl);
#endif

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

HTREEITEM CFileView::InsertChild(HTREEITEM hParent, CComPtr<IShellFolder>& folder, LPITEMIDLIST ItemId)
{
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
                ::DoContextMenu(this, TheContextMenu, Flags, ptTree.x, ptTree.y);
            }
        }
    }
    //pWndTree->SetFocus();
    //theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EXPLORER, point.x, point.y, this, TRUE);
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
    AfxMessageBox(_T("Properties...."));
}

void CFileView::OnSync()
{
    CDocument* pDoc = CRadDocManager::GetActiveDocument();
    if (pDoc != nullptr)
    {
        const CString& path = pDoc->GetPathName();
        LPITEMIDLIST pidl = nullptr;
        /*HRESULT hr =*/ SHParseDisplayName(path, NULL, &pidl, 0, 0);

        HTREEITEM hNode = m_wndFileView.GetRootItem();
        TreeItem* ti = (TreeItem*) m_wndFileView.GetItemData(hNode);
        if (ILIsParent(ti->ItemId, pidl, FALSE))
        {
            LPITEMIDLIST parentpidl = ILClone(ti->ItemId);
            HTREEITEM hChild = m_wndFileView.GetChildItem(hNode);
            while (hChild != NULL)
            {
                /*TreeItem**/ ti = (TreeItem*) m_wndFileView.GetItemData(hChild);
                LPITEMIDLIST childpidl = ILCombine(parentpidl, ti->ItemId);
                if (ILIsParent(childpidl, pidl, FALSE))
                {
                    m_wndFileView.Expand(hChild, TVE_EXPAND);
                    //InsertChildren(hNode, ti);
                    hNode = hChild;
                    hChild = m_wndFileView.GetChildItem(hNode);
                    parentpidl = childpidl;

                    if (hChild == NULL && ILIsEqual(parentpidl, pidl))
                    {
                        m_wndFileView.Select(hNode, TVGN_CARET);
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

        ILFree(pidl);
    }
}

void CFileView::OnFileOpen()
{
	// TODO: Add your command handler code here
}

void CFileView::OnFileOpenWith()
{
	// TODO: Add your command handler code here
}

void CFileView::OnDummyCompile()
{
	// TODO: Add your command handler code here
}

void CFileView::OnEditCut()
{
	// TODO: Add your command handler code here
}

void CFileView::OnEditCopy()
{
	// TODO: Add your command handler code here
}

void CFileView::OnEditClear()
{
	// TODO: Add your command handler code here
}

void CFileView::OnEditRename()
{
    HTREEITEM hItem = m_wndFileView.GetSelectedItem();
    if (hItem != NULL)
        m_wndFileView.EditLabel(hItem);
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
    CEdit* pEdit = m_wndFileView.GetEditControl();
    if (pEdit != nullptr)
    {
        pEdit->SetWindowText(GetDisplayNameOf(ti->Parent, ti->ItemId, m_Malloc, SHGDN_FOREDITING));
    }
    *pResult = 0;
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
