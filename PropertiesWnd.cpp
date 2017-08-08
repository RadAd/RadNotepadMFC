
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

enum PropType
{
    PROP_BOOL,
    PROP_INT,
    PROP_COLOR,
    PROP_FONT,
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

    Property(COLORREF* c)
        : nType(PROP_COLOR)
        , valColor(c)
    {
    }

    Property(LOGFONT* f)
        : nType(PROP_FONT)
        , valFont(f)
    {
    }

    PropType nType;
    union
    {
        bool* valBool;
        INT* valInt;
        COLORREF* valColor;
        LOGFONT* valFont;
    };
};

// TODO new Property aren't been deleted

CMFCPropertyGridProperty* CreateProperty(const CString& strName, bool* pBool)
{
    return new CMFCPropertyGridProperty(strName, (_variant_t) *pBool, nullptr, (DWORD_PTR) new Property(pBool));
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, int* pInt)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) *pInt, nullptr, (DWORD_PTR) new Property(pInt));
    p->EnableSpinControl(TRUE, 1, 100);
    return p;
}

CMFCPropertyGridColorProperty* CreateProperty(const CString& strName, COLORREF* pColor, const COLORREF* pDefaultColor = nullptr)
{
    CMFCPropertyGridColorProperty* p = new CMFCPropertyGridColorProperty(strName, *pColor, nullptr, nullptr, (DWORD_PTR) new Property(pColor));
    if (pDefaultColor != nullptr)
        p->EnableAutomaticButton(_T("Default"), *pDefaultColor);
    p->EnableOtherButton(_T("More Colors..."));
    return p;
}

CMFCPropertyGridFontProperty* CreateProperty(const CString& strName, LOGFONT* pFont)
{
    return new CMFCPropertyGridFontProperty(strName, *pFont, CF_EFFECTS | CF_SCREENFONTS, nullptr, (DWORD_PTR) new Property(pFont));
}

class CMFCThemeProperty : public CMFCPropertyGridProperty
{
public:
    CMFCThemeProperty(const CString& strGroupName, Theme* pTheme, const Theme* pDefaultTheme)
        : CMFCPropertyGridProperty(strGroupName, 0, TRUE)
        , m_pDefaultTheme(pDefaultTheme)
        , m_pBackground(CreateProperty(_T("Background"), &pTheme->back, pDefaultTheme != nullptr ? &pDefaultTheme->back : nullptr))
        , m_pForeground(CreateProperty(_T("Foreground"), &pTheme->fore, pDefaultTheme != nullptr ? &pDefaultTheme->fore : nullptr))
        , m_pFont(CreateProperty(_T("Font"), &pTheme->font))
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

        CFont* pOldFont = NULL;
        if (IsModified() && m_pWndList->IsMarkModifiedProperties())
        {
            pOldFont = pDC->SelectObject(&m_pWndList->GetBoldFont());
        }
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
    const Theme* m_pDefaultTheme;
    CMFCPropertyGridColorProperty* m_pBackground;
    CMFCPropertyGridColorProperty* m_pForeground;
    CMFCPropertyGridFontProperty* m_pFont;
};

CMFCPropertyGridProperty* CreateProperty(const CString& strName, Theme* pTheme, const Theme* pDefaultTheme)
{
    // TODO Override rendering
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
        *prop->valBool = pProp->GetValue() != VARIANT_FALSE;
        break;

    case PROP_INT:
        *prop->valInt = pProp->GetValue().intVal;
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
        m_wndPropList.AddProperty(pGroup);
    }

    {
        CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("What???"));
        pGroup->AddSubItem(CreateProperty(_T("Use Tabs"), &m_pSettings->bUseTabs));
        pGroup->AddSubItem(CreateProperty(_T("Tab Width"), &m_pSettings->nTabWidth));
        m_wndPropList.AddProperty(pGroup);
    }

    {
        CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Margins"));
        pGroup->AddSubItem(CreateProperty(_T("Line Numbers"), &m_pSettings->PropShowLineNumbers));
        pGroup->AddSubItem(CreateProperty(_T("Bookmarks"), &m_pSettings->PropShowBookmarks));
        pGroup->AddSubItem(CreateProperty(_T("Folds"), &m_pSettings->PropShowFolds));
        {
            //pGroup->AddSubItem(CreateProperty(_T("Fold Marker"), &m_pSettings->nMarkerType));
            const wchar_t* n[] = { _T("Arrow"), _T("Plus/Minus"), _T("Circle"), _T("Box") };
            CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(_T("Fold Marker"), (_variant_t) n[m_pSettings->nFoldType], nullptr);
            for (int i = 0; i < ARRAYSIZE(n); ++i)
                p->AddOption(n[i]);
            p->AllowEdit(FALSE);
            pGroup->AddSubItem(p);
        }
        pGroup->AddSubItem(CreateProperty(_T("Fold Background"), &m_pSettings->cFoldBG));
        pGroup->AddSubItem(CreateProperty(_T("Fold Foreground"), &m_pSettings->cFoldFG));

        m_wndPropList.AddProperty(pGroup);
    }

    {
        CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Styles"));
        pGroup1->AddSubItem(CreateProperty(_T("Default"), &m_pSettings->tDefault, nullptr));
        for (int i = 0; i < ARRAYSIZE(Settings::vecTheme); ++i)
            pGroup1->AddSubItem(CreateProperty(m_pSettings->vecTheme[i].name, &m_pSettings->vecTheme[i].theme, &m_pSettings->tDefault));
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

LRESULT CPropertiesWnd::OnPropertyChanged(WPARAM /*wParam*/, LPARAM lParam)
{
    CMFCPropertyGridProperty* pProp = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
    Property* prop = (Property*) pProp->GetData();
    if (prop != nullptr)
        ::SetProperty(pProp, prop);
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
