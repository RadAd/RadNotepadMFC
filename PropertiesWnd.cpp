
#include "stdafx.h"

#include "PropertiesWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "RadNotepad.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// TODO
// A CMFCPropertyGridColorProperty which knows common color names
// Selecting and pasting into edit color (and maybe others) appends text instead
// CMFCPropertyGridProperty for int with a default value
// Add something to theme item to say which properties are used

#define ID_OBJECT_COMBO 100
#define DEF_LENGTH 3

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
    Property(E* i, const E* j, const int* values)
        : nType(PROP_INDEX)
        , valInt(reinterpret_cast<INT*>(i))
        , defInt { reinterpret_cast<const INT*>(j) }
        , vecValues(values)
    {
    }

    Property(COLORREF* c, const std::initializer_list<const COLORREF*>& def)
        : nType(PROP_COLOR)
        , valColor(c)
        , defColor {}
    {
        ASSERT(def.size() <= DEF_LENGTH);
        std::copy(def.begin(), def.end(), defColor);
    }

    Property(LOGFONT* f, const std::initializer_list<const LOGFONT*>& def)
        : nType(PROP_FONT)
        , valFont(f)
        , defFont {}
    {
        ASSERT(def.size() <= DEF_LENGTH);
        std::copy(def.begin(), def.end(), defFont);
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
        const INT* defInt[DEF_LENGTH];
        const COLORREF* defColor[DEF_LENGTH];
        const LOGFONT* defFont[DEF_LENGTH];
    };
    const int* vecValues;
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

CMFCPropertyGridColorProperty* CreateProperty(const CString& strName, COLORREF* pColor, const std::initializer_list<const COLORREF*>& def)
{
    CMFCPropertyGridColorProperty* p = new CMFCPropertyGridColorProperty(strName, *pColor, nullptr, nullptr, (DWORD_PTR) new Property(pColor, def));
    for (const COLORREF* pDefaultColor : def)
    {
        if (pDefaultColor != nullptr && *pDefaultColor != COLOR_NONE)
        {
            p->EnableAutomaticButton(_T("Default"), *pDefaultColor);
            break;
        }
    }
    p->EnableOtherButton(_T("More Colors..."));
    return p;
}

CMFCPropertyGridFontProperty* CreateProperty(const CString& strName, LOGFONT* pFont, const std::initializer_list<const LOGFONT*>& def)
{
    CMFCPropertyGridFontProperty* p = new CMFCPropertyGridFontProperty(strName, *pFont, CF_EFFECTS | CF_SCREENFONTS, nullptr, (DWORD_PTR) new Property(pFont, def));
    PLOGFONT f = p->GetLogFont();
    if (f->lfFaceName[0] == _T('\0'))
        wcscpy_s(f->lfFaceName, _T("Default"));
    return p;
}

template <class E>
CMFCPropertyGridProperty* CreateProperty(const CString& strName, E* pIndex, const std::initializer_list<LPCTSTR>& items)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) items.begin()[*pIndex], nullptr, (DWORD_PTR) new Property(pIndex, (E*) nullptr, nullptr));
    for (LPCTSTR i : items)
        p->AddOption(i);
    p->AllowEdit(FALSE);
    return p;
}

static inline int GetIndex(int find, const int* values, int size)
{
    for (int i = 0; i < size; ++i)
        if (values[i] == find)
            return i;
    ASSERT(FALSE);
    return -1;
}

template <class E>
CMFCPropertyGridProperty* CreateProperty(const CString& strName, E* pIndex, const std::initializer_list<LPCTSTR>& items, const std::initializer_list<int>& values)
{
    ASSERT(items.size() == values.size());
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) items.begin()[GetIndex(*pIndex, values.begin(), (int) values.size())], nullptr, (DWORD_PTR) new Property(pIndex, (E*) nullptr, values.begin()));
    for (LPCTSTR i : items)
        p->AddOption(i);
    p->AllowEdit(FALSE);
    return p;
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, Bool3* pValue, const Bool3* pBase)
{
    CMFCPropertyGridProperty* p = nullptr;
    if (pBase != nullptr)
    {
        LPCTSTR items[] = { _T("Default"), _T("True"), _T("False") };
        static const int values[] = { B3_UNDEFINED, B3_TRUE, B3_FALSE };
        p = new CMFCPropertyGridProperty(strName, (_variant_t) items[GetIndex(*pValue, values, ARRAYSIZE(values))], nullptr, (DWORD_PTR) new Property(pValue, pBase, values));
        for (LPCTSTR item : items)
            p->AddOption(item);
        p->AllowEdit(FALSE);
    }
    else
    {
        LPCTSTR items[] = { _T("True"), _T("False") };
        static const int values[] = { B3_TRUE, B3_FALSE };
        p = new CMFCPropertyGridProperty(strName, (_variant_t) items[GetIndex(*pValue, values, ARRAYSIZE(values))], nullptr, (DWORD_PTR) new Property(pValue, pBase, values));
        for (LPCTSTR item : items)
            p->AddOption(item);
        p->AllowEdit(FALSE);
    }
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
    // TODO How to transform std::initializer_list<const ThemeItem*> into std::initializer_list<const COLORREF*>
    //CMFCThemeProperty(const CString& strGroupName, ThemeItem* pTheme, const std::initializer_list<const ThemeItem*>& pDefaultTheme)
    CMFCThemeProperty(const CString& strGroupName, ThemeItem* pTheme, const ThemeItem* pDefaultTheme1, const ThemeItem* pDefaultTheme2, const ThemeItem* pDefaultTheme3)
        : CMFCPropertyGridProperty(strGroupName, 0, TRUE)
        , m_pDefaultTheme { pDefaultTheme1, pDefaultTheme2, pDefaultTheme3 }
        , m_pBackground(CreateProperty(_T("Background"), &pTheme->back, { pn(pDefaultTheme1, back), pn(pDefaultTheme2, back), pn(pDefaultTheme3, back) }))
        , m_pForeground(CreateProperty(_T("Foreground"), &pTheme->fore, { pn(pDefaultTheme1, fore), pn(pDefaultTheme2, fore), pn(pDefaultTheme3, fore) }))
        , m_pFont(CreateProperty(_T("Font"), &pTheme->font, { pn(pDefaultTheme1, font), pn(pDefaultTheme2, font), pn(pDefaultTheme3, font) }))
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
        if (f.lfFaceName[0] == _T('\0') || wcscmp(f.lfFaceName, _T("Default")) == 0)
        {
            for (const ThemeItem* pDefaultTheme : m_pDefaultTheme)
            {
                if (pDefaultTheme != nullptr && (pDefaultTheme->font.lfFaceName[0] != _T('\0') || wcscmp(pDefaultTheme->font.lfFaceName, _T("Default")) != 0))
                {
                    wcscpy_s(f.lfFaceName, pDefaultTheme->font.lfFaceName);
                    break;
                }
            }
        }
        if (f.lfHeight == 0)
        {
            for (const ThemeItem* pDefaultTheme : m_pDefaultTheme)
            {
                if (pDefaultTheme != nullptr && pDefaultTheme->font.lfHeight != 0)
                {
                    f.lfHeight = pDefaultTheme->font.lfHeight;
                    break;
                }
            }
        }
        if (f.lfWeight == 0)
        {
            for (const ThemeItem* pDefaultTheme : m_pDefaultTheme)
            {
                if (pDefaultTheme != nullptr && pDefaultTheme->font.lfWeight != 0)
                {
                    f.lfWeight = pDefaultTheme->font.lfWeight;
                    break;
                }
            }
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
        if (c == (COLORREF) -1)
        {
            for (const ThemeItem* pDefaultTheme : m_pDefaultTheme)
            {
                if (pDefaultTheme != nullptr && pDefaultTheme->back != (COLORREF) -1)
                {
                    c = pDefaultTheme->back;
                    break;
                }
            }
        }
        return c;
    }

    COLORREF GetForegroundColor() const
    {
        COLORREF c = m_pForeground->GetColor();
        if (c == (COLORREF) -1)
        {
            for (const ThemeItem* pDefaultTheme : m_pDefaultTheme)
            {
                if (pDefaultTheme != nullptr && pDefaultTheme->fore != (COLORREF) -1)
                {
                    c = pDefaultTheme->fore;
                    break;
                }
            }
        }
        return c;
    }
private:
    const ThemeItem* m_pDefaultTheme[DEF_LENGTH];
    CMFCPropertyGridColorProperty* m_pBackground;
    CMFCPropertyGridColorProperty* m_pForeground;
    CMFCPropertyGridFontProperty* m_pFont;
};

CMFCPropertyGridProperty* CreateProperty(const CString& strName, ThemeItem* pTheme, const ThemeItem* pDefaultTheme1, const ThemeItem* pDefaultTheme2, const ThemeItem* pDefaultTheme3)
{
    return new CMFCThemeProperty(strName, pTheme, pDefaultTheme1, pDefaultTheme2, pDefaultTheme3);
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
        ASSERT(pProp->GetValue().vt == VT_INT || pProp->GetValue().vt == VT_I4);
        *prop->valInt = pProp->GetValue().intVal;
        break;

    case PROP_UINT:
        ASSERT(pProp->GetValue().vt == VT_INT);
        *prop->valUInt = pProp->GetValue().intVal;
        break;

    case PROP_INDEX:
        {
            int i = GetOptionIndex(pProp);
            if (prop->vecValues != nullptr)
                i = prop->vecValues[i];
            *prop->valInt = i;
        }
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
            for (const LOGFONT* defFont : prop->defFont)
            {
                if (defFont != nullptr && wcscmp(defFont->lfFaceName, f->lfFaceName) == 0)
                {
                    wcscpy_s(f->lfFaceName, _T("Default"));
                    break;
                }
            }
            for (const LOGFONT* defFont : prop->defFont)
            {
                if (defFont != nullptr && defFont->lfHeight == f->lfHeight)
                {
                    f->lfHeight = 0;
                    break;
                }
            }
            for (const LOGFONT* defFont : prop->defFont)
            {
                if (defFont != nullptr && defFont->lfWeight == f->lfWeight)
                {
                    f->lfWeight = 0;
                    break;
                }
            }
            // TODO What to do with italic and underline
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
	ON_COMMAND(ID_PROPERTIES_RESET, OnPropertiesReset)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES_RESET, OnUpdatePropertiesReset)
	ON_COMMAND(ID_PROPERTIES2, OnProperties2)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
    ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
    ON_WM_DESTROY()
    ON_CBN_SELCHANGE(ID_OBJECT_COMBO, OnComboSelChange)
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

	m_wndObjectCombo.SetWindowPos(NULL, rectClient.left, rectClient.top, rectClient.Width(), m_nComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndToolBar.SetWindowPos(NULL, rectClient.left, rectClient.top + m_nComboHeight, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndPropList.SetWindowPos(NULL, rectClient.left, rectClient.top + m_nComboHeight + cyTlb, rectClient.Width(), rectClient.Height() -(m_nComboHeight+cyTlb), SWP_NOACTIVATE | SWP_NOZORDER);
}

int CPropertiesWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create combo:
	const DWORD dwViewStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL;

	if (!m_wndObjectCombo.Create(dwViewStyle, rectDummy, this, ID_OBJECT_COMBO))
	{
		TRACE0("Failed to create Properties Combo \n");
		return -1;      // fail to create
	}

	m_wndObjectCombo.AddString(_T("Application"));
	m_wndObjectCombo.AddString(_T("Editor"));
    {
        std::vector<Language>& vecLanguage = theApp.m_Settings.user.vecLanguage;
        std::vector<Language*> vecSortLanguage; // = vecLanguage;
        for (Language& rLanguage : vecLanguage)
            vecSortLanguage.push_back(&rLanguage);
        std::sort(vecSortLanguage.begin(), vecSortLanguage.end(), CompareLanguageTitle);

        for (Language* pLanguage : vecSortLanguage)
        {
            CString name = pLanguage->internal ? _T("Output: ") + pLanguage->title : _T("Language: ") + pLanguage->title;
            int i = m_wndObjectCombo.AddString(name);
            m_wndObjectCombo.SetItemData(i, (DWORD_PTR) pLanguage);
        }
    }
	m_wndObjectCombo.SetCurSel(0);

	CRect rectCombo;
	m_wndObjectCombo.GetClientRect(&rectCombo);

	m_nComboHeight = rectCombo.Height();

	if (!m_wndPropList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_PROPERTIES);
	m_wndToolBar.LoadToolBar(IDR_PROPERTIES, 0, 0, TRUE /* Is locked */);

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

void CPropertiesWnd::OnPropertiesReset()
{
    m_pSettings->user = m_pSettings->default;
    m_wndPropList.RemoveAll();
    InitPropList();
}

void CPropertiesWnd::OnUpdatePropertiesReset(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pSettings->user != m_pSettings->default);
}

void CPropertiesWnd::OnProperties2()
{
	// TODO: Add your command handler code here
    theApp.NotifySettingsChanged();
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

    int i = m_wndObjectCombo.GetCurSel();
    Language* pLanguage = (Language*) m_wndObjectCombo.GetItemDataPtr(i);

    if (pLanguage != nullptr)
    {
        Theme* pTheme = &m_pSettings->user;
        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Editor"));
            {
                CMFCPropertyGridProperty* pParent = pGroup;
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Caret"), 0, TRUE);
                pGroup->AllowEdit(FALSE);
                pGroup->AddSubItem(CreateProperty(_T("Foreground"), &pLanguage->editor.cCaretFG, { &pTheme->editor.cCaretFG, &m_pSettings->user.tDefault.fore }));
                pGroup->AddSubItem(CreateProperty(_T("Style"), &pLanguage->editor.nCaretStyle, { _T("Default"), _T("Invisible"), _T("Line"), _T("Block") }, { -1, CARETSTYLE_INVISIBLE, CARETSTYLE_LINE, CARETSTYLE_BLOCK }));
                pGroup->AddSubItem(CreateProperty(_T("Width"), &pLanguage->editor.nCaretWidth, 0, 4));
                pParent->AddSubItem(pGroup);
            }
            if (!pLanguage->internal) pGroup->AddSubItem(CreateProperty(_T("Use Tabs"), &pLanguage->editor.bUseTabs, &pTheme->editor.bUseTabs));
            pGroup->AddSubItem(CreateProperty(_T("Tab Width"), &pLanguage->editor.nTabWidth, 0, 100));
            if (!pLanguage->internal) pGroup->AddSubItem(CreateProperty(_T("Indent Guides"), &pLanguage->editor.nIndentGuideType, { _T("Default"), _T("None"), _T("Real"), _T("Look Forward"), _T("Look Both") }, { -1, SC_IV_NONE, SC_IV_REAL, SC_IV_LOOKFORWARD, SC_IV_LOOKBOTH }));
            if (!pLanguage->internal) pGroup->AddSubItem(CreateProperty(_T("Highlight Matching Braces"), &pLanguage->editor.bHighlightMatchingBraces, &pTheme->editor.bHighlightMatchingBraces));
            if (!pLanguage->internal) pGroup->AddSubItem(CreateProperty(_T("Auto-Indent"), &pLanguage->editor.bAutoIndent, &pTheme->editor.bAutoIndent));
            pGroup->AddSubItem(CreateProperty(_T("Show Whitespace"), &pLanguage->editor.bShowWhitespace, &pTheme->editor.bShowWhitespace));
            // TODO pGroup->AddSubItem(CreateProperty(_T("Whitespace"), &pLanguage->editor.nWhitespaceMode, &pTheme->editor.nWhitespaceMode));
            // TODO pGroup->AddSubItem(CreateProperty(_T("Tab"), &pLanguage->editor.nTabDrawMode, &pTheme->editor.nTabDrawMode));
            // TODO pGroup->AddSubItem(CreateProperty(_T("Whitespace Size"), &pLanguage->editor.nWhitespaceSize, &pTheme->editor.nWhitespaceSize));
            pGroup->AddSubItem(CreateProperty(_T("Show EOL"), &pLanguage->editor.bShowEOL, &pTheme->editor.bShowEOL));
            pGroup->AddSubItem(CreateProperty(_T("Word Wrap"), &pLanguage->editor.bWordWrap, &pTheme->editor.bWordWrap));
            m_wndPropList.AddProperty(pGroup);
        }

        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Margins"));
            for (Margin& margin : pLanguage->vecMargin)
            {
                const Margin* pBaseMargin = GetKey(pTheme->vecMargin, margin.id);
                pGroup->AddSubItem(CreateProperty(margin.name, &margin.show, pn(pBaseMargin, show)));
            }
            m_wndPropList.AddProperty(pGroup);
        }

        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Styles"));
            for (Style& s : pLanguage->vecStyle)
            {
                const StyleClass* os = GetStyleClass(pTheme, s.sclass);
                pGroup->AddSubItem(CreateProperty(s.name, &s.theme, pn(os, theme), &pTheme->tDefault, nullptr));
            }
            for (GroupStyle& gs : pLanguage->vecGroupStyle)
            {
                CMFCPropertyGridProperty* pParent = pGroup;
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(gs.name);
                for (Style& s : gs.vecStyle)
                {
                    const StyleClass* os = GetStyleClass(pTheme, s.sclass);
                    pGroup->AddSubItem(CreateProperty(s.name, &s.theme, pn(os, theme), &pTheme->tDefault, nullptr));
                }
                pParent->AddSubItem(pGroup);
            }
            if (!pLanguage->vecBase.empty())
            {
                CMFCPropertyGridProperty* pParent = pGroup;
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Common"));
                for (Style& s : pLanguage->vecBase)
                {
                    const Style* os = GetKey(pTheme->vecBase, s.id);
                    CString sclass = Merge(s.sclass, pn(os, sclass), CString(), CString());
                    const StyleClass* osc = GetStyleClass(pTheme, sclass);
                    pGroup->AddSubItem(CreateProperty(s.name, &s.theme, pn(os, theme), pn(osc, theme), &pTheme->tDefault));
                }
                pParent->AddSubItem(pGroup);
            }
            m_wndPropList.AddProperty(pGroup);
        }

        if (!pLanguage->vecMarker.empty())
        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Markers"));
            for (Marker& marker : pLanguage->vecMarker)
            {
                const Marker* pBaseMarker = GetKey(pTheme->vecMarker, marker.id);
                CMFCPropertyGridProperty* pMarkerGroup = new CMFCPropertyGridProperty(marker.name, 0, TRUE);
                pMarkerGroup->AllowEdit(FALSE);
                pMarkerGroup->AddSubItem(CreateProperty(_T("Type"), &marker.type, /*SC_MARK_CIRCLE*/ -1, SC_MARK_BOOKMARK));
                pMarkerGroup->AddSubItem(CreateProperty(_T("Fore"), &marker.fore, { pn(pBaseMarker, fore) }));
                pMarkerGroup->AddSubItem(CreateProperty(_T("Back"), &marker.back, { pn(pBaseMarker, back) }));
                pGroup->AddSubItem(pMarkerGroup);
            }
            m_wndPropList.AddProperty(pGroup);
        }
    }
    else if (i == 0)    // Application
    {
        CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("General"));
        pGroup->AddSubItem(CreateProperty(_T("Empty File on Startup"), &m_pSettings->bEmptyFileOnStartup));
        pGroup->AddSubItem(CreateProperty(_T("Number of Recently Used Files"), &m_pSettings->nMaxMRU, 1, 10));
        pGroup->AddSubItem(CreateProperty(_T("Default Encoding"), &m_pSettings->DefaultEncoding, { _T("ANSI"), _T("UTF-16"), _T("UTF-16 BE"), _T("UTF-8") }));
        pGroup->AddSubItem(CreateProperty(_T("Default Line Ending"), &m_pSettings->DefaultLineEnding, { _T("Windows (CRLF)"), _T("Unix (LF)"), _T("Macintosh (CR)") }));
        m_wndPropList.AddProperty(pGroup);
    }
    else if (i == 1)    // Editor
    {
        Theme* pTheme = &m_pSettings->user;
        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Editor"));
            {
                CMFCPropertyGridProperty* pParent = pGroup;
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Caret"), 0, TRUE);
                pGroup->AllowEdit(FALSE);
                pGroup->AddSubItem(CreateProperty(_T("Foreground"), &pTheme->editor.cCaretFG, { &m_pSettings->user.tDefault.fore }));
                pGroup->AddSubItem(CreateProperty(_T("Style"), &pTheme->editor.nCaretStyle, { _T("Invisible"), _T("Line"), _T("Block") }));
                pGroup->AddSubItem(CreateProperty(_T("Width"), &pTheme->editor.nCaretWidth, 1, 4));
                pParent->AddSubItem(pGroup);
            }
            pGroup->AddSubItem(CreateProperty(_T("Use Tabs"), &pTheme->editor.bUseTabs, nullptr));
            pGroup->AddSubItem(CreateProperty(_T("Tab Width"), &pTheme->editor.nTabWidth, 1, 100));
            pGroup->AddSubItem(CreateProperty(_T("Indent Guides"), &pTheme->editor.nIndentGuideType, { _T("None"), _T("Real"), _T("Look Forward"), _T("Look Both") }));
            pGroup->AddSubItem(CreateProperty(_T("Highlight Matching Braces"), &pTheme->editor.bHighlightMatchingBraces, nullptr));
            pGroup->AddSubItem(CreateProperty(_T("Auto-Indent"), &pTheme->editor.bAutoIndent, nullptr));
            pGroup->AddSubItem(CreateProperty(_T("Show Whitespace"), &pTheme->editor.bShowWhitespace, nullptr));
            // TODO pGroup->AddSubItem(CreateProperty(_T("Whitespace"), &pTheme->editor.nWhitespaceMode, nullptr));
            // TODO pGroup->AddSubItem(CreateProperty(_T("Tab"), &pTheme->editor.nTabDrawMode, nullptr));
            // TODO pGroup->AddSubItem(CreateProperty(_T("Whitespace Size"), &pTheme->editor.nWhitespaceSize, nullptr));
            pGroup->AddSubItem(CreateProperty(_T("Show EOL"), &pTheme->editor.bShowEOL, nullptr));
            pGroup->AddSubItem(CreateProperty(_T("Word Wrap"), &pTheme->editor.bWordWrap, nullptr));
            m_wndPropList.AddProperty(pGroup);
        }

        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Margins"));
            for (Margin& margin : pTheme->vecMargin)
            {
                pGroup->AddSubItem(CreateProperty(margin.name, &margin.show, nullptr));
            }
            m_wndPropList.AddProperty(pGroup);
        }

        {
            CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Styles"));
            pGroup1->AddSubItem(CreateProperty(_T("Default"), &pTheme->tDefault, nullptr, nullptr, nullptr));
            for (StyleClass& sc : pTheme->vecStyleClass)
            {
                if (!sc.description.IsEmpty())
                    pGroup1->AddSubItem(CreateProperty(sc.description, &sc.theme, &pTheme->tDefault, nullptr, nullptr));
            }
            {
                CMFCPropertyGridProperty* pParent = pGroup1;
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Common"));
                for (Style& s : pTheme->vecBase)
                {
                    const StyleClass* osc = GetStyleClass(pTheme, s.sclass);
                    pGroup->AddSubItem(CreateProperty(s.name, &s.theme, pn(osc, theme), &pTheme->tDefault, nullptr));
                }
                pParent->AddSubItem(pGroup);
            }
            m_wndPropList.AddProperty(pGroup1);
        }

        if (!pTheme->vecMarker.empty())
        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Markers"));
            for (Marker& marker : pTheme->vecMarker)
            {
                CMFCPropertyGridProperty* pMarkerGroup = new CMFCPropertyGridProperty(marker.name, 0, TRUE);
                pMarkerGroup->AllowEdit(FALSE);
                pMarkerGroup->AddSubItem(CreateProperty(_T("Type"), &marker.type, SC_MARK_CIRCLE, SC_MARK_BOOKMARK));
                pMarkerGroup->AddSubItem(CreateProperty(_T("Fore"), &marker.fore, {} ));
                pMarkerGroup->AddSubItem(CreateProperty(_T("Back"), &marker.back, {} ));
                pGroup->AddSubItem(pMarkerGroup);
            }
            m_wndPropList.AddProperty(pGroup);
        }
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
    if (prop != nullptr && propdef != nullptr && prop->nType == propdef->nType)
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
            for (const COLORREF* defColor : prop->defColor)
            {
                if (defColor == propdef->valColor)
                {
                    CMFCPropertyGridColorProperty* p = static_cast<CMFCPropertyGridColorProperty*>(pProp);
                    p->EnableAutomaticButton(_T("Default"), *defColor);
                    pProp->Redraw();
                    break;
                }
            }
            break;

        case PROP_FONT:
            for (const LOGFONT* defFont : prop->defFont)
            {
                if (defFont == propdef->valFont)
                {
                    pProp->Redraw();
                    break;
                }
            }
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

    theApp.NotifySettingsChanged();

    return 0;
}

void CPropertiesWnd::OnComboSelChange()
{
    m_wndPropList.RemoveAll();
    InitPropList();
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
	m_wndObjectCombo.SetFont(&m_fntPropList);
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
