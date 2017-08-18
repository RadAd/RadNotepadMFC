
#include "stdafx.h"

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "RadNotepad.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// TODO A CMFCPropertyGridColorProperty which knows common color names

enum PropType
{
    PROP_BOOL,
    PROP_INT,
    PROP_UINT,
    PROP_COLOR,
    PROP_FONT,
    PROP_INDEX,
};

struct Property
{
    Property(bool* b)
        : nType(PROP_BOOL)
        , valBool(b)
    {
    }

    Property(INT* i)
        : nType(PROP_INT)
        , valInt(i)
    {
    }

    Property(UINT* i)
        : nType(PROP_UINT)
        , valUInt(i)
    {
    }

    template <class E>
    Property(E* i, LPCTSTR* /*items*/)
        : nType(PROP_INDEX)
        , valInt(reinterpret_cast<INT*>(i))
    {
    }

    Property(COLORREF* c, const COLORREF* def)
        : nType(PROP_COLOR)
        , valColor(c)
        , defColor(def)
    {
    }

    Property(LOGFONT* f, const LOGFONT* def)
        : nType(PROP_FONT)
        , valFont(f)
        , defFont(def)
    {
    }

    PropType nType;
    union
    {
        bool* valBool;
        INT* valInt;
        UINT* valUInt;
        COLORREF* valColor;
        LOGFONT* valFont;
    };
    union
    {
        const COLORREF* defColor;
        const LOGFONT* defFont;
    };
};

CMFCPropertyGridProperty* CreateProperty(const CString& strName, bool* pBool)
{
    return new CMFCPropertyGridProperty(strName, (_variant_t) *pBool, nullptr, (DWORD_PTR) new Property(pBool));
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, INT* pInt, INT nMin, INT nMax)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) *pInt, nullptr, (DWORD_PTR) new Property(pInt));
    p->EnableSpinControl(TRUE, nMin, nMax);
    return p;
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, UINT* pInt, UINT nMin, UINT nMax)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) *pInt, nullptr, (DWORD_PTR) new Property(pInt));
    p->EnableSpinControl(TRUE, nMin, nMax);
    return p;
}

CMFCPropertyGridColorProperty* CreateProperty(const CString& strName, COLORREF* pColor, const COLORREF* pDefaultColor = nullptr)
{
    CMFCPropertyGridColorProperty* p = new CMFCPropertyGridColorProperty(strName, *pColor, nullptr, nullptr, (DWORD_PTR) new Property(pColor, pDefaultColor));
    if (pDefaultColor != nullptr)
        p->EnableAutomaticButton(_T("Default"), *pDefaultColor);
    p->EnableOtherButton(_T("More Colors..."));
    return p;
}

CMFCPropertyGridFontProperty* CreateProperty(const CString& strName, LOGFONT* pFont, const LOGFONT* pFontDef)
{
    CMFCPropertyGridFontProperty* p = new CMFCPropertyGridFontProperty(strName, *pFont, CF_EFFECTS | CF_SCREENFONTS, nullptr, (DWORD_PTR) new Property(pFont, pFontDef));
    PLOGFONT f = p->GetLogFont();
    if (f->lfFaceName[0] == _T('\0'))
        wcscpy_s(f->lfFaceName, _T("Default"));
    return p;
}

template <class E>
CMFCPropertyGridProperty* CreateProperty(const CString& strName, E* pIndex, LPCTSTR* items, int nItemCount)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) items[*pIndex], nullptr, (DWORD_PTR) new Property(pIndex, items));
    for (int i = 0; i < nItemCount; ++i)
        p->AddOption(items[i]);
    p->AllowEdit(FALSE);
    return p;
}


int GetOptionIndex(CMFCPropertyGridProperty* pProp)
{
    const COleVariant& v = pProp->GetValue();
    if (v.vt == VT_BSTR)
    {
        for (int i = 0; i < pProp->GetOptionCount(); ++i)
        {
            if (wcscmp(pProp->GetOption(i), v.bstrVal) == 0)
                return i;
        }
    }
    return -1;
}

class CMFCThemeProperty : public CMFCPropertyGridProperty
{
public:
    CMFCThemeProperty(const CString& strGroupName, ThemeItem* pTheme, const ThemeItem* pDefaultTheme)
        : CMFCPropertyGridProperty(strGroupName, 0, TRUE)
        , m_pDefaultTheme(pDefaultTheme)
        , m_pBackground(CreateProperty(_T("Background"), &pTheme->back, pDefaultTheme != nullptr ? &pDefaultTheme->back : nullptr))
        , m_pForeground(CreateProperty(_T("Foreground"), &pTheme->fore, pDefaultTheme != nullptr ? &pDefaultTheme->fore : nullptr))
        , m_pFont(CreateProperty(_T("Font"), &pTheme->font, pDefaultTheme != nullptr ? &pDefaultTheme->font : nullptr))
    {
        AddSubItem(m_pBackground);
        AddSubItem(m_pForeground);
        AddSubItem(m_pFont);
        AllowEdit(FALSE);
    }

    virtual void OnDrawValue(CDC* pDC, CRect rect)
    {
        ASSERT_VALID(this);
        ASSERT_VALID(pDC);
        ASSERT_VALID(m_pWndList);

        if ((IsGroup() && !m_bIsValueList) || !HasValueField())
        {
            return;
        }

#if 0
        CFont* pOldFont = NULL;
        if (IsModified() && m_pWndList->IsMarkModifiedProperties())
        {
            pOldFont = pDC->SelectObject(&m_pWndList->GetBoldFont());
        }
#else
        LOGFONT f = *m_pFont->GetLogFont();
        if (m_pDefaultTheme != nullptr)
        {
            if (f.lfFaceName[0] == _T('\0') || wcscmp(f.lfFaceName, _T("Default")) == 0)
                wcscpy_s(f.lfFaceName, m_pDefaultTheme->font.lfFaceName);
            if (f.lfHeight == 0)
                f.lfHeight = m_pDefaultTheme->font.lfHeight;
            if (f.lfWeight == 0)
                f.lfWeight = m_pDefaultTheme->font.lfWeight;
        }
        // TODO What to do with italic and underline
        CFont font;
        font.CreateFontIndirectW(&f);
        CFont* pOldFont = pDC->SelectObject(&font);
#endif
        COLORREF cOldBg = pDC->SetBkColor(GetBackgroundColor());
        int nOldMode = pDC->SetBkMode(OPAQUE);
        COLORREF cOldFg = pDC->SetTextColor(GetForegroundColor());

        CString strVal = FormatProperty();

        rect.DeflateRect(AFX_TEXT_MARGIN, 0);

        pDC->DrawText(strVal, rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);

        m_bValueIsTruncated = pDC->GetTextExtent(strVal).cx > rect.Width();

        if (pOldFont != NULL)
        {
            pDC->SelectObject(pOldFont);
        }
        pDC->SetBkColor(cOldBg);
        pDC->SetBkMode(nOldMode);
        pDC->SetTextColor(cOldFg);
    }

    virtual CString FormatProperty()
    {
        return GetName();
    }

    virtual BOOL OnEdit(LPPOINT lptClick)
    {
        //Expand();
        return CMFCPropertyGridProperty::OnEdit(lptClick);
    }

    virtual BOOL OnClickValue(UINT uiMsg, CPoint point)
    {
        if (uiMsg == WM_LBUTTONDOWN)
            Expand();
        return CMFCPropertyGridProperty::OnClickValue(uiMsg, point);
    }

    COLORREF GetBackgroundColor() const
    {
        COLORREF c = m_pBackground->GetColor();
        if (c == (COLORREF) -1 && m_pDefaultTheme != nullptr)
            c = m_pDefaultTheme->back;
        return c;
    }

    COLORREF GetForegroundColor() const
    {
        COLORREF c = m_pForeground->GetColor();
        if (c == (COLORREF) -1 && m_pDefaultTheme != nullptr)
            c = m_pDefaultTheme->fore;
        return c;
    }
private:
    const ThemeItem* m_pDefaultTheme;
    CMFCPropertyGridColorProperty* m_pBackground;
    CMFCPropertyGridColorProperty* m_pForeground;
    CMFCPropertyGridFontProperty* m_pFont;
};

CMFCPropertyGridProperty* CreateProperty(const CString& strName, ThemeItem* pTheme, const ThemeItem* pDefaultTheme)
{
#if 0
    CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(strName, 0, TRUE);
    pGroup->AddSubItem(CreateProperty(_T("Background"), &pTheme->back, pDefaultTheme != nullptr ? &pDefaultTheme->back : nullptr));
    pGroup->AddSubItem(CreateProperty(_T("Foreground"), &pTheme->fore, pDefaultTheme != nullptr ? &pDefaultTheme->fore : nullptr));
    pGroup->AddSubItem(CreateProperty(_T("Font"), &pTheme->font));
    return pGroup;
#else
    return new CMFCThemeProperty(strName, pTheme, pDefaultTheme);
#endif
}

void SetProperty(CMFCPropertyGridProperty* pProp, Property* prop)
{
    switch (prop->nType)
    {
    case PROP_BOOL:
        ASSERT(pProp->GetValue().vt == VT_BOOL);
        *prop->valBool = pProp->GetValue().boolVal != VARIANT_FALSE;;
        break;

    case PROP_INT:
        ASSERT(pProp->GetValue().vt == VT_INT);
        *prop->valInt = pProp->GetValue().intVal;
        break;

    case PROP_UINT:
        ASSERT(pProp->GetValue().vt == VT_INT);
        *prop->valUInt = pProp->GetValue().intVal;
        break;

    case PROP_INDEX:
        *prop->valInt = GetOptionIndex(pProp);
        break;

    case PROP_COLOR:
        {
            CMFCPropertyGridColorProperty* p = static_cast<CMFCPropertyGridColorProperty*>(pProp);
            COLORREF c = p->GetColor();
            *prop->valColor = c;
        }
        break;

    case PROP_FONT:
        {
            CMFCPropertyGridFontProperty* p = static_cast<CMFCPropertyGridFontProperty*>(pProp);
            PLOGFONT f = p->GetLogFont();
            if (prop->defFont != nullptr)
            {
                if (wcscmp(prop->defFont->lfFaceName, f->lfFaceName) == 0)
                    wcscpy_s(f->lfFaceName, _T("Default"));
                if (prop->defFont->lfHeight == f->lfHeight)
                    f->lfHeight = 0;
                if (prop->defFont->lfWeight == f->lfWeight)
                    f->lfWeight = 0;
                // TODO What to do with italic and underline
            }
            *prop->valFont = *f;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CPropertiesWnd::CPropertiesWnd()
{
	m_nComboHeight = 0;
    m_pSettings = &theApp.m_Settings;
}

CPropertiesWnd::~CPropertiesWnd()
{
}

BEGIN_MESSAGE_MAP(CPropertiesWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_COMMAND(ID_PROPERTIES1, OnProperties1)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
	ON_COMMAND(ID_PROPERTIES2, OnProperties2)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
    ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
    ON_WM_DESTROY()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CPropertiesWnd::AdjustLayout()
{
	if (GetSafeHwnd () == NULL || (AfxGetMainWnd() != NULL && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	//m_wndObjectCombo.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), m_nComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top + m_nComboHeight, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + m_nComboHeight + cyTlb, rectClient.Width(), rectClient.Height() -(m_nComboHeight+cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

#if 0
	// Create combo:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | CBS_SORT | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (!m_wndObjectCombo.Create(dwViewStyle, rectDummy, this, 1))
	{
		TRACE0("Failed to create Properties Combo \n");
		return -1;      // fail to create
	}

	m_wndObjectCombo.AddString(_T("Application"));
	m_wndObjectCombo.AddString(_T("Properties Window"));
	m_wndObjectCombo.SetCurSel(0);

	CRect rectCombo;
	m_wndObjectCombo.GetClientRect (&rectCombo);

	m_nComboHeight = rectCombo.Height();
#endif

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* Is locked */);
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_PROPERTIES_HC : IDR_PROPERTIES, 0, 0, TRUE /* Locked */);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	AdjustLayout();
	return 0;
}

void CPropertiesWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPropertiesWnd::OnExpandAllProperties()
{
	m_wndPropList.ExpandAll();
}

void CPropertiesWnd::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CPropertiesWnd::OnSortProperties()
{
	m_wndPropList.SetAlphabeticMode(!m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPropList.IsAlphabeticMode());
}

void CPropertiesWnd::OnProperties1()
{
	// TODO: Add your command handler code here
    // TODO Temporary to refresh properties
    m_wndPropList.RemoveAll();
    InitPropList();
}

void CPropertiesWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CPropertiesWnd::OnProperties2()
{
	// TODO: Add your command handler code here
}

void CPropertiesWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CPropertiesWnd::InitPropList()
{
	SetPropListFont();

	m_wndPropList.EnableHeaderCtrl(FALSE);
	m_wndPropList.EnableDescriptionArea();
	m_wndPropList.SetVSDotNetLook();
	m_wndPropList.MarkModifiedProperties();

    {
        CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("General"));
        pGroup->AddSubItem(CreateProperty(_T("Empty File on Startup"), &m_pSettings->bEmptyFileOnStartup));
        pGroup->AddSubItem(CreateProperty(_T("Number of Recetly Used Files"), &m_pSettings->nMaxMRU, 1, 10));
        m_wndPropList.AddProperty(pGroup);
    }

    {
        CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Editor"));
        pGroup->AddSubItem(CreateProperty(_T("Use Tabs"), &m_pSettings->editor.bUseTabs));
        pGroup->AddSubItem(CreateProperty(_T("Tab Width"), &m_pSettings->editor.nTabWidth, 1, 100));
        pGroup->AddSubItem(CreateProperty(_T("Show Indent Guides"), &m_pSettings->editor.bShowIndentGuides));
        pGroup->AddSubItem(CreateProperty(_T("Highlight Matching Braces"), &m_pSettings->editor.bHighlightMatchingBraces));
        pGroup->AddSubItem(CreateProperty(_T("Auto-Indent"), &m_pSettings->editor.bAutoIndent));
        m_wndPropList.AddProperty(pGroup);
    }

    {
        CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Margins"));
        pGroup->AddSubItem(CreateProperty(_T("Line Numbers"), &m_pSettings->editor.bShowLineNumbers));
        pGroup->AddSubItem(CreateProperty(_T("Bookmarks"), &m_pSettings->editor.bShowBookmarks));
        {
            CMFCPropertyGridProperty* pParent = pGroup;
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Fold Marker"), 0, TRUE);
            pGroup->AddSubItem(CreateProperty(_T("Enabled"), &m_pSettings->editor.bShowFolds));
            LPCTSTR n[] = { _T("Arrow"), _T("Plus/Minus"), _T("Circle"), _T("Box") };
            pGroup->AddSubItem(CreateProperty(_T("Style"), &m_pSettings->editor.nFoldType, n, ARRAYSIZE(n)));
            pGroup->AddSubItem(CreateProperty(_T("Fold Background"), &m_pSettings->editor.cFoldBG));
            pGroup->AddSubItem(CreateProperty(_T("Fold Foreground"), &m_pSettings->editor.cFoldFG));
            pParent->AddSubItem(pGroup);
        }

        m_wndPropList.AddProperty(pGroup);
    }

    {
        Theme* pTheme = &m_pSettings->editor.rTheme;
        CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Styles"));
        pGroup1->AddSubItem(CreateProperty(_T("Default"), &pTheme->tDefault, nullptr));
        for (StyleClass& sc : pTheme->vecStyleClass)
        {
            if (!sc.description.IsEmpty())
                pGroup1->AddSubItem(CreateProperty(sc.description, &sc.theme, &pTheme->tDefault));
        }
        m_wndPropList.AddProperty(pGroup1);
    }

#if 0
    CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Appearance"));

	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("3D Look"), (_variant_t) false, _T("Specifies the window's font will be non-bold and controls will have a 3D border")));

	CMFCPropertyGridProperty* pProp = new CMFCPropertyGridProperty(_T("Border"), _T("Dialog Frame"), _T("One of: None, Thin, Resizable, or Dialog Frame"));
	pProp->AddOption(_T("None"));
	pProp->AddOption(_T("Thin"));
	pProp->AddOption(_T("Resizable"));
	pProp->AddOption(_T("Dialog Frame"));
	pProp->AllowEdit(FALSE);

	pGroup1->AddSubItem(pProp);
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("Caption"), (_variant_t) _T("About"), _T("Specifies the text that will be displayed in the window's title bar")));

	m_wndPropList.AddProperty(pGroup1);

	CMFCPropertyGridProperty* pSize = new CMFCPropertyGridProperty(_T("Window Size"), 0, TRUE);

	pProp = new CMFCPropertyGridProperty(_T("Height"), (_variant_t) 250l, _T("Specifies the window's height"));
	pProp->EnableSpinControl(TRUE, 50, 300);
	pSize->AddSubItem(pProp);

	pProp = new CMFCPropertyGridProperty( _T("Width"), (_variant_t) 150l, _T("Specifies the window's width"));
	pProp->EnableSpinControl(TRUE, 50, 200);
	pSize->AddSubItem(pProp);

	m_wndPropList.AddProperty(pSize);

	CMFCPropertyGridProperty* pGroup2 = new CMFCPropertyGridProperty(_T("Font"));

	LOGFONT lf;
	CFont* font = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	font->GetLogFont(&lf);

	_tcscpy_s(lf.lfFaceName, _T("Arial"));

	pGroup2->AddSubItem(new CMFCPropertyGridFontProperty(_T("Font"), lf, CF_EFFECTS | CF_SCREENFONTS, _T("Specifies the default font for the window")));
	pGroup2->AddSubItem(new CMFCPropertyGridProperty(_T("Use System Font"), (_variant_t) true, _T("Specifies that the window uses MS Shell Dlg font")));

	m_wndPropList.AddProperty(pGroup2);

	CMFCPropertyGridProperty* pGroup3 = new CMFCPropertyGridProperty(_T("Misc"));
	pProp = new CMFCPropertyGridProperty(_T("(Name)"), _T("Application"));
	pProp->Enable(FALSE);
	pGroup3->AddSubItem(pProp);

	CMFCPropertyGridColorProperty* pColorProp = new CMFCPropertyGridColorProperty(_T("Window Color"), RGB(210, 192, 254), NULL, _T("Specifies the default window color"));
	pColorProp->EnableOtherButton(_T("Other..."));
	pColorProp->EnableAutomaticButton(_T("Default"), ::GetSysColor(COLOR_3DFACE));
	pGroup3->AddSubItem(pColorProp);

	static const TCHAR szFilter[] = _T("Icon Files(*.ico)|*.ico|All Files(*.*)|*.*||");
	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Icon"), TRUE, _T(""), _T("ico"), 0, szFilter, _T("Specifies the window icon")));

	pGroup3->AddSubItem(new CMFCPropertyGridFileProperty(_T("Folder"), _T("c:\\")));

	m_wndPropList.AddProperty(pGroup3);

	CMFCPropertyGridProperty* pGroup4 = new CMFCPropertyGridProperty(_T("Hierarchy"));

	CMFCPropertyGridProperty* pGroup41 = new CMFCPropertyGridProperty(_T("First sub-level"));
	pGroup4->AddSubItem(pGroup41);

	CMFCPropertyGridProperty* pGroup411 = new CMFCPropertyGridProperty(_T("Second sub-level"));
	pGroup41->AddSubItem(pGroup411);

	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 1"), (_variant_t) _T("Value 1"), _T("This is a description")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 2"), (_variant_t) _T("Value 2"), _T("This is a description")));
	pGroup411->AddSubItem(new CMFCPropertyGridProperty(_T("Item 3"), (_variant_t) _T("Value 3"), _T("This is a description")));

	pGroup4->Expand(FALSE);
	m_wndPropList.AddProperty(pGroup4);
#endif
}

void CPropertiesWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndPropList.SetFocus();
}

void CPropertiesWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
	SetPropListFont();
}

static void Refresh(CMFCPropertyGridProperty* pProp, Property* propdef)
{
    for (int i = 0; i < pProp->GetSubItemsCount(); ++i)
        Refresh(pProp->GetSubItem(i), propdef);
    Property* prop = (Property*) pProp->GetData();
    if (prop != nullptr && prop->nType == propdef->nType)
    {
        switch (prop->nType)
        {
        case PROP_BOOL:
            break;

        case PROP_INT:
            break;

        case PROP_INDEX:
            break;

        case PROP_COLOR:
            if (prop->defColor == propdef->valColor)
            {
                CMFCPropertyGridColorProperty* p = static_cast<CMFCPropertyGridColorProperty*>(pProp);
                p->EnableAutomaticButton(_T("Default"), *prop->defColor);
                pProp->Redraw();
            }
            break;

        case PROP_FONT:
            if (prop->defFont == propdef->valFont)
                pProp->Redraw();
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }
}

LRESULT CPropertiesWnd::OnPropertyChanged(WPARAM /*wParam*/, LPARAM lParam)
{
    CMFCPropertyGridProperty* pProp = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
    Property* prop = (Property*) pProp->GetData();
    if (prop != nullptr)
        ::SetProperty(pProp, prop);

    for (int i = 0; i < m_wndPropList.GetPropertyCount(); ++i)
        Refresh(m_wndPropList.GetProperty(i), prop);

    return 0;
}

void CPropertiesWnd::SetPropListFont()
{
	::DeleteObject(m_fntPropList.Detach());

	LOGFONT lf;
	afxGlobalData.fontRegular.GetLogFont(&lf);

	NONCLIENTMETRICS info;
	info.cbSize = sizeof(info);

	afxGlobalData.GetNonClientMetrics(info);

	lf.lfHeight = info.lfMenuFont.lfHeight;
	lf.lfWeight = info.lfMenuFont.lfWeight;
	lf.lfItalic = info.lfMenuFont.lfItalic;

	m_fntPropList.CreateFontIndirect(&lf);

	m_wndPropList.SetFont(&m_fntPropList);
	//m_wndObjectCombo.SetFont(&m_fntPropList);
}

static void CleanUp(CMFCPropertyGridProperty* pProp)
{
    for (int i = 0; i < pProp->GetSubItemsCount(); ++i)
        CleanUp(pProp->GetSubItem(i));
    Property* prop = (Property*) pProp->GetData();
    if (prop != nullptr)
        delete prop;
}

void CPropertiesWnd::OnDestroy()
{
    CDockablePane::OnDestroy();

    for (int i = 0; i < m_wndPropList.GetPropertyCount(); ++i)
        CleanUp(m_wndPropList.GetProperty(i));
}
