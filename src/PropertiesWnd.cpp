
#include "stdafx.h"

#include "PropertiesWnd.h"
#include "MainFrm.h"
#include "RadNotepad.h"
#include "NewExtensionDlg.h"

#include "..\resource.h"

#include <algorithm>
#include <iterator>

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

template <class T>
class span
{
public:
    static span null()
    {
        return span();
    }

    span(const T* begin, const T* end)
        : m_begin(begin)
        , m_end(end)
    {
        ASSERT(begin != nullptr);
        ASSERT(end >= begin);
    }

    span(const std::initializer_list<T>& items)
        : span(items.begin(), items.end())
    {
    }

    template<size_t Size>
    span(const T(&items)[Size])
        : span(std::begin(items), std::end(items))
    {
    }

    span(const std::vector<T>& items)
        : span(items.data(), items.data() + items.size())
    {
    }

    const T* begin() const { return m_begin; }
    const T* end() const { return m_end; }

    size_t size() const
    {
        return m_end - m_begin;
    }

    bool empty() const
    {
        return m_begin == m_end;
    }

    const T& operator[](size_t i) const
    {
        ASSERT(i < size());
        return m_begin[i];
    }

    span sub(size_t begin) const
    {
        ASSERT(begin < size());
        return span(m_begin + begin, m_end);
    }

private:
    span()
        : m_begin(nullptr)
        , m_end(nullptr)
    {
    }

    const T* m_begin;
    const T* m_end;
};

template<class T, size_t Size>
span<T> to_span(const T(&v)[Size])
{
    return span<T>(v);
}

class PropertyBase
{
public:
    virtual ~PropertyBase()
    {
    }

    virtual void SetProperty(CMFCPropertyGridProperty* pProp) = 0;
    virtual bool IsDefault(const PropertyBase* propdef) const = 0;
    virtual void Refresh(CMFCPropertyGridProperty* pPropChild, const PropertyBase* propdef) const = 0;
};

static const LPCTSTR g_stylesnames[] = { _T("Default"), _T("Invisible"), _T("Line"), _T("Block") };
static const int g_stylesvalues[] = { -1, CARETSTYLE_INVISIBLE, CARETSTYLE_LINE, CARETSTYLE_BLOCK };
static const LPCTSTR g_indentguidesnames[] = { _T("Default"), _T("None"), _T("Real"), _T("Look Forward"), _T("Look Both") };
static const int g_indentguidesvalues[] = { -1, SC_IV_NONE, SC_IV_REAL, SC_IV_LOOKFORWARD, SC_IV_LOOKBOTH };
static const LPCTSTR g_encodingnames[] = { _T("ANSI"), _T("UTF-16"), _T("UTF-16 BE"), _T("UTF-8") };
static const LPCTSTR g_lineendingnames[] = { _T("Windows (CRLF)"), _T("Unix (LF)"), _T("Macintosh (CR)") };
static const LPCTSTR g_bool3names[] = { _T("Default"), _T("True"), _T("False") };
static const Bool3 g_bool3values[] = { Bool3::B3_UNDEFINED, Bool3::B3_TRUE, Bool3::B3_FALSE };

template <class E>
class SimpleProperty : public PropertyBase
{
public:
    SimpleProperty(E* value)
        : m_value(value)
    {
        ASSERT(value != nullptr);
    }

    void SetProperty(CMFCPropertyGridProperty* pProp) override
    {
        SetProperty(*m_value, pProp);
    }

    bool IsDefault(const PropertyBase* /*propdefbase*/) const override
    {
        return false;
    }

    void Refresh(CMFCPropertyGridProperty* /*pPropChild*/, const PropertyBase* /*propdefbase*/) const
    {
        ASSERT(false);
    }

protected:
    E* GetValue() const
    {
        return m_value;
    }

private:
    static void SetProperty(CString& value, CMFCPropertyGridProperty* pProp)
    {
        ASSERT(pProp->GetValue().vt == VT_BSTR);
        value = pProp->GetValue().bstrVal;
    }

    static void SetProperty(bool& value, CMFCPropertyGridProperty* pProp)
    {
        ASSERT(pProp->GetValue().vt == VT_BOOL);
        value = pProp->GetValue().boolVal != VARIANT_FALSE;
    }

    static void SetProperty(INT& value, CMFCPropertyGridProperty* pProp)
    {
        ASSERT(pProp->GetValue().vt == VT_INT || pProp->GetValue().vt == VT_I4);
        value = pProp->GetValue().intVal;
    }

    static void SetProperty(UINT& value, CMFCPropertyGridProperty* pProp)
    {
        ASSERT(pProp->GetValue().vt == VT_UINT);
        value = pProp->GetValue().uintVal;
    }

    static void SetProperty(COLORREF& value, CMFCPropertyGridProperty* pProp)
    {
        ASSERT(static_cast<CMFCPropertyGridColorProperty*>(pProp) != nullptr);
        CMFCPropertyGridColorProperty* p = static_cast<CMFCPropertyGridColorProperty*>(pProp);
        AFXASSUME(pProp != nullptr);
        COLORREF c = p->GetColor();
        value = c;
    }

    static void SetProperty(LOGFONT& value, CMFCPropertyGridProperty* pProp)
    {
        ASSERT(static_cast<CMFCPropertyGridFontProperty*>(pProp) != nullptr);
        CMFCPropertyGridFontProperty* p = static_cast<CMFCPropertyGridFontProperty*>(pProp);
        AFXASSUME(pProp != nullptr);
        PLOGFONT f = p->GetLogFont();
        value = *f;
    }

    E* m_value;
};

template <class E, class F = E>
class IndexedProperty : public PropertyBase
{
public:
    IndexedProperty(E* value, span<F> values)
        : m_value(value), m_values(values)
    {
        ASSERT(value != nullptr);
        //ASSERT(values != nullptr);
    }

    void SetProperty(CMFCPropertyGridProperty* pProp) override
    {
        int i = GetOptionIndex(pProp);
        SetProperty<std::is_enum<E>::value>(*m_value, i, m_values);
    }

    bool IsDefault(const PropertyBase* /*propdefbase*/) const override
    {
        return false;
    }

    void Refresh(CMFCPropertyGridProperty* /*pPropChild*/, const PropertyBase* /*propdefbase*/) const
    {
        ASSERT(false);
    }

private:
    static inline int GetOptionIndex(CMFCPropertyGridProperty* pProp)
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

    template <bool IsEnum, class E, class F>
    static void SetProperty(E& value, int i, span<F> values)
    {
        AFXASSUME(!values.empty());
        value = values[i];
    }

    template <>
    static void SetProperty<true>(E& value, int i, span<F> values)
    {
        if (values.begin() == nullptr)
            value = static_cast<E>(i);
        else
            value = values[i];
    }

    template <>
    static void SetProperty<false>(int& value, int i, span<int> values)
    {
        if (values.empty())
            value = i;
        else
            value = values[i];
    }

    E* m_value;
    span<F> m_values;
};

template <class E> 
class SimplePropertyWithDefaults : public SimpleProperty<E>
{
public:
    SimplePropertyWithDefaults(E* c, std::vector<const E*> def)
        : SimpleProperty(c)
        , m_defaults(std::move(def))
    {
        ASSERT(std::find(m_defaults.begin(), m_defaults.end(), nullptr) == m_defaults.end());
    }

    void Init(CMFCPropertyGridProperty* pPropChild)
    {
        Init(pPropChild, m_defaults);
    }

    void SetProperty(CMFCPropertyGridProperty* pProp) override
    {
        FixProperty(pProp, m_defaults);
        SimpleProperty<E>::SetProperty(pProp);
    }

    bool IsDefault(const PropertyBase* propdefbase) const override
    {
        const SimplePropertyWithDefaults* propdef = dynamic_cast<const SimplePropertyWithDefaults*>(propdefbase);

        if (propdef == nullptr)
            return false;

        for (const E* def : m_defaults)
        {
            if (def == propdef->GetValue())
                return true;
        }
        return false;
    }

    void Refresh(CMFCPropertyGridProperty* pProp, const PropertyBase* propdefbase) const override
    {
        const SimplePropertyWithDefaults* propdef = dynamic_cast<const SimplePropertyWithDefaults*>(propdefbase);
        AFXASSUME(propdef != nullptr);
        DoRefresh(pProp, *propdef->GetValue());
        pProp->Redraw();
    }

private:
    static void Init(CMFCPropertyGridProperty* pProp, const std::vector<const COLORREF*>& defaults)
    {
        auto it = std::find_if(defaults.begin(), defaults.end(), [](const COLORREF* pColor) { return *pColor != COLOR_NONE; });
        if (it != defaults.end())
            DoRefresh(pProp, **it);
        ASSERT(static_cast<CMFCPropertyGridColorProperty*>(pProp) != nullptr);
        CMFCPropertyGridColorProperty* p = static_cast<CMFCPropertyGridColorProperty*>(pProp);
        AFXASSUME(p != nullptr);
        p->EnableOtherButton(_T("More Colors..."));
    }

    static void DoRefresh(CMFCPropertyGridProperty* pProp, COLORREF value)
    {
        ASSERT(static_cast<CMFCPropertyGridColorProperty*>(pProp) != nullptr);
        CMFCPropertyGridColorProperty* p = static_cast<CMFCPropertyGridColorProperty*>(pProp);
        AFXASSUME(p != nullptr);
        p->EnableAutomaticButton(_T("Default"), value);
    }

    static void FixProperty(CMFCPropertyGridProperty* /*pProp*/, const std::vector<const COLORREF*>& /*defaults*/)
    {
    }

    static void Init(CMFCPropertyGridProperty* pProp, const std::vector<const LOGFONT*>& defaults)
    {
        FixProperty(pProp, defaults);
    }

    static void DoRefresh(CMFCPropertyGridProperty* /*pPropChild*/, const LOGFONT& /*value*/)
    {
    }

    static void FixProperty(CMFCPropertyGridProperty* pProp, const std::vector<const LOGFONT*>& defaults)
    {
        ASSERT(static_cast<CMFCPropertyGridFontProperty*>(pProp) != nullptr);
        CMFCPropertyGridFontProperty* p = static_cast<CMFCPropertyGridFontProperty*>(pProp);
        AFXASSUME(p != nullptr);
        PLOGFONT f = p->GetLogFont();

        if (f->lfFaceName[0] == _T('\0'))
            wcscpy_s(f->lfFaceName, _T("Default"));
        else
        {
            auto it = std::find_if(defaults.begin(), defaults.end(), [f](const LOGFONT* pFont) { return wcscmp(pFont->lfFaceName, f->lfFaceName) == 0; });
            if (it != defaults.end())
                wcscpy_s(f->lfFaceName, _T("Default"));
        }
        {
            auto it = std::find_if(defaults.begin(), defaults.end(), [f](const LOGFONT* pFont) { return pFont->lfHeight == f->lfHeight; });
            if (it != defaults.end())
                f->lfHeight = 0;
        }
        {
            auto it = std::find_if(defaults.begin(), defaults.end(), [f](const LOGFONT* pFont) { return pFont->lfWeight == f->lfWeight; });
            if (it != defaults.end())
                f->lfWeight = 0;
        }
        // TODO What to do with italic and underline
    }

    std::vector<const E*> m_defaults;
};

template<class E>
CMFCPropertyGridProperty* CreateProperty(const CString& strName, E* pValue)
{
    return new CMFCPropertyGridProperty(strName, (_variant_t) *pValue, nullptr, (DWORD_PTR) new SimpleProperty<E>(pValue));
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, INT* pInt, INT nMin, INT nMax)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) *pInt, nullptr, (DWORD_PTR) new SimpleProperty<INT>(pInt));
    p->EnableSpinControl(TRUE, nMin, nMax);
    return p;
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, UINT* pInt, UINT nMin, UINT nMax)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) *pInt, nullptr, (DWORD_PTR) new SimpleProperty<UINT>(pInt));
    p->EnableSpinControl(TRUE, nMin, nMax);
    return p;
}

CMFCPropertyGridColorProperty* CreateProperty(const CString& strName, COLORREF* pColor, std::vector<const COLORREF*> def)
{
    SimplePropertyWithDefaults<COLORREF>* pProp = new SimplePropertyWithDefaults<COLORREF>(pColor, std::move(def));
    CMFCPropertyGridColorProperty* p = new CMFCPropertyGridColorProperty(strName, *pColor, nullptr, nullptr, (DWORD_PTR) pProp);
    pProp->Init(p);
    return p;
}

CMFCPropertyGridFontProperty* CreateProperty(const CString& strName, LOGFONT* pFont, std::vector<const LOGFONT*> def)
{
    SimplePropertyWithDefaults<LOGFONT>* pProp = new SimplePropertyWithDefaults<LOGFONT>(pFont, std::move(def));
    CMFCPropertyGridFontProperty* p = new CMFCPropertyGridFontProperty(strName, *pFont, CF_EFFECTS | CF_SCREENFONTS, nullptr, (DWORD_PTR) pProp);
    pProp->Init(p);
    return p;
}

template <class E>
CMFCPropertyGridProperty* CreateProperty(const CString& strName, E* pIndex, span<LPCTSTR> items)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) items[to_underlying(*pIndex)], nullptr, (DWORD_PTR) new IndexedProperty<E>(pIndex, span<E>::null()));
    for (LPCTSTR i : items)
        p->AddOption(i);
    p->AllowEdit(FALSE);
    return p;
}

CMFCPropertyGridProperty* CreateProperty(const CString& strName, const CString& strValueName, CString* pStrValue, span<LPCTSTR> names, span<LPCTSTR> values)
{
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) strValueName, nullptr, (DWORD_PTR) new IndexedProperty<CString, LPCTSTR>(pStrValue, values));
    for (LPCTSTR i : names)
        p->AddOption(i);
    p->AllowEdit(FALSE);
    return p;
}

template <class E>
CMFCPropertyGridProperty* CreateProperty(const CString& strName, E* pIndex, span<LPCTSTR> items, span<E> values)
{
    ASSERT(items.size() == values.size());
    CMFCPropertyGridProperty* p = new CMFCPropertyGridProperty(strName, (_variant_t) *std::find(std::begin(values), std::end(values), *pIndex), nullptr, (DWORD_PTR) new IndexedProperty<E>(pIndex, values));
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
        auto it = std::find(std::begin(g_bool3values), std::end(g_bool3values), *pValue);
        ASSERT(it != std::end(g_bool3values));
        p = new CMFCPropertyGridProperty(strName, (_variant_t) g_bool3names[std::distance(std::begin(g_bool3values), it)], nullptr, (DWORD_PTR) new IndexedProperty<Bool3>(pValue, g_bool3values));
        for (LPCTSTR item : g_bool3names)
            p->AddOption(item);
        p->AllowEdit(FALSE);
    }
    else
    {
        span<LPCTSTR> names = span<LPCTSTR>(g_bool3names).sub(1);
        span<Bool3> values = span<Bool3>(g_bool3values).sub(1);
        auto it = std::find(std::begin(values), std::end(values), *pValue);
        ASSERT(it != std::end(values));
        p = new CMFCPropertyGridProperty(strName, (_variant_t) names[std::distance(std::begin(values), it)], nullptr, (DWORD_PTR) new IndexedProperty<Bool3>(pValue, values));
        for (LPCTSTR item : names)
            p->AddOption(item);
        p->AllowEdit(FALSE);
    }
    return p;
}

static void CleanUp(CMFCPropertyGridProperty* pProp)
{
    for (int i = 0; i < pProp->GetSubItemsCount(); ++i)
        CleanUp(pProp->GetSubItem(i));
    PropertyBase* prop = (PropertyBase*) pProp->GetData();
    if (prop != nullptr)
        delete prop;
}

template<class T, class E>
static inline std::vector<const E*> transform(const std::vector<const T*>& vec, const E T::* pItem)
{
    std::vector<const E*> vecItem;
    vecItem.reserve(vec.size());
    std::transform(vec.begin(), vec.end(), std::back_inserter(vecItem), [pItem](const T* pVecItem) { return &(pVecItem->*pItem); });
    return vecItem;
}

class CMFCThemeProperty : public CMFCPropertyGridProperty
{
public:
    CMFCThemeProperty(const CString& strGroupName, const CString* strClassName, ThemeItem* pTheme, std::vector<const ThemeItem*> vecDefaultTheme)
        : CMFCPropertyGridProperty(strGroupName, 0, TRUE)
        , m_pDefaultTheme(std::move(vecDefaultTheme))
        , m_pBackground(CreateProperty(_T("Background"), &pTheme->back, transform(m_pDefaultTheme, &ThemeItem::back)))
        , m_pForeground(CreateProperty(_T("Foreground"), &pTheme->fore, transform(m_pDefaultTheme, &ThemeItem::fore)))
        , m_pFont(CreateProperty(_T("Font"), &pTheme->font, transform(m_pDefaultTheme, &ThemeItem::font)))
    {
        ASSERT(std::find(m_pDefaultTheme.begin(), m_pDefaultTheme.end(), nullptr) == m_pDefaultTheme.end());
        if (strClassName != nullptr)
        {
            CMFCPropertyGridProperty* pClass = new CMFCPropertyGridProperty(_T("Class"), COleVariant((LPCTSTR) *strClassName));
            pClass->Enable(FALSE);
            AddSubItem(pClass);
        }

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
        // TODO Verify this is still needed
        LOGFONT f = *m_pFont->GetLogFont();
        if (f.lfFaceName[0] == _T('\0') || wcscmp(f.lfFaceName, _T("Default")) == 0)
        {
            auto it = std::find_if(m_pDefaultTheme.begin(), m_pDefaultTheme.end(), [](const ThemeItem* pTheme) { return pTheme->font.lfFaceName[0] != _T('\0') || wcscmp(pTheme->font.lfFaceName, _T("Default")) != 0; });
            if (it != m_pDefaultTheme.end())
                wcscpy_s(f.lfFaceName, (*it)->font.lfFaceName);
        }
        if (f.lfHeight == 0)
        {
            auto it = std::find_if(m_pDefaultTheme.begin(), m_pDefaultTheme.end(), [](const ThemeItem* pTheme) { return pTheme->font.lfHeight != 0; });
            if (it != m_pDefaultTheme.end())
                f.lfHeight = (*it)->font.lfHeight;
        }
        if (f.lfWeight == 0)
        {
            auto it = std::find_if(m_pDefaultTheme.begin(), m_pDefaultTheme.end(), [](const ThemeItem* pTheme) { return pTheme->font.lfWeight != 0; });
            if (it != m_pDefaultTheme.end())
                f.lfWeight = (*it)->font.lfWeight;
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
    const std::vector<const ThemeItem*> m_pDefaultTheme;
    CMFCPropertyGridColorProperty* m_pBackground;
    CMFCPropertyGridColorProperty* m_pForeground;
    CMFCPropertyGridFontProperty* m_pFont;
};

CMFCPropertyGridProperty* CreateProperty(const CString& strName, const CString* strClassName, ThemeItem* pTheme, std::vector<const ThemeItem*> vecDefaultTheme)
{
    std::_Erase_remove(vecDefaultTheme, nullptr);
    return new CMFCThemeProperty(strName, strClassName, pTheme, std::move(vecDefaultTheme));
}

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar

CPropertiesWnd::CPropertiesWnd()
{
	m_nComboHeight = 0;
    m_pSettings = &theApp.m_Settings;
    m_pExtGroup = nullptr;
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
    ON_COMMAND(ID_PROPERTIES_NEW, OnPropertiesNew)
    ON_UPDATE_COMMAND_UI(ID_PROPERTIES_NEW, OnUpdatePropertiesNew)
    ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
    ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, OnPropertyChanged)
    ON_WM_DESTROY()
    ON_CBN_SELCHANGE(ID_OBJECT_COMBO, OnComboSelChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CPropertiesWnd::InitLanguages()
{
    const std::vector<Language>& vecLanguage = m_pSettings->user.vecLanguage;
    std::vector<const Language*> vecSortLanguage; // = vecLanguage;
    for (const Language& rLanguage : vecLanguage)
        vecSortLanguage.push_back(&rLanguage);
    std::sort(vecSortLanguage.begin(), vecSortLanguage.end(), CompareLanguageTitle);

    for (const Language* pLanguage : vecSortLanguage)
    {
        CString name = pLanguage->internal ? _T("Output: ") + pLanguage->title : _T("Language: ") + pLanguage->title;
        int i = m_wndObjectCombo.AddString(name);
        m_wndObjectCombo.SetItemData(i, (DWORD_PTR) pLanguage);
    }

    m_LanguageValues.push_back(_T(""));
    m_LanguageNames.push_back(_T("None"));
    for (const Language* pLanguage : vecSortLanguage)
    {
        m_LanguageValues.push_back(pLanguage->name);
        m_LanguageNames.push_back(pLanguage->title);
    }

    if (m_pExtGroup != nullptr && m_pExtGroup->GetSubItemsCount() == 0)
        FillExtensions();
}

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
    for (int i = 0; i < m_wndPropList.GetPropertyCount(); ++i)
        CleanUp(m_wndPropList.GetProperty(i));
    m_wndPropList.RemoveAll();
    InitPropList();
}

void CPropertiesWnd::OnUpdatePropertiesReset(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_pSettings->user != m_pSettings->default);
}

void CPropertiesWnd::OnPropertiesNew()
{
    CNewExtensionDlg dlgNewExtension(m_pSettings);
    if (dlgNewExtension.DoModal() == IDOK)
    {
        if (m_pSettings->user.mapExt.find(dlgNewExtension.m_strExtension) != m_pSettings->user.mapExt.end())
            AfxMessageBox(_T("Extension already exists"), MB_OK | MB_ICONERROR);
        else
        {
            auto it = m_pSettings->user.mapExt.insert(std::map<CString, CString>::value_type(dlgNewExtension.m_strExtension, _T(""))).first;
            //CMFCPropertyGridProperty* pProp = CreateProperty(it->first, &it->second);
            CMFCPropertyGridProperty* pProp = CreateProperty(it->first, m_LanguageNames[0], &it->second, m_LanguageNames, m_LanguageValues);
            m_pExtGroup->AddSubItem(pProp);
            m_wndPropList.SetCurSel(pProp);
            m_wndPropList.EnsureVisible(pProp, TRUE);
        }
    }
}

void CPropertiesWnd::OnUpdatePropertiesNew(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_wndObjectCombo.GetCurSel() == 0);
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
#pragma warning(suppress:4456)
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Caret"), 0, TRUE);
                pGroup->AllowEdit(FALSE);
                pGroup->AddSubItem(CreateProperty(_T("Foreground"), &pLanguage->editor.cCaretFG, { &pTheme->editor.cCaretFG, &m_pSettings->user.tDefault.fore }));
                pGroup->AddSubItem(CreateProperty(_T("Style"), &pLanguage->editor.nCaretStyle, g_stylesnames, to_span(g_stylesvalues)));
                pGroup->AddSubItem(CreateProperty(_T("Width"), &pLanguage->editor.nCaretWidth, 0, 4));
                pParent->AddSubItem(pGroup);
            }
            if (!pLanguage->internal) pGroup->AddSubItem(CreateProperty(_T("Use Tabs"), &pLanguage->editor.bUseTabs, &pTheme->editor.bUseTabs));
            pGroup->AddSubItem(CreateProperty(_T("Tab Width"), &pLanguage->editor.nTabWidth, 0, 100));
            if (!pLanguage->internal) pGroup->AddSubItem(CreateProperty(_T("Indent Guides"), &pLanguage->editor.nIndentGuideType, g_indentguidesnames, to_span(g_indentguidesvalues)));
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
                pGroup->AddSubItem(CreateProperty(s.name, pn(os, description), &s.theme, { pn(os, theme), &pTheme->tDefault }));
            }
            for (GroupStyle& gs : pLanguage->vecGroupStyle)
            {
                CMFCPropertyGridProperty* pParent = pGroup;
#pragma warning(suppress:4456)
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(gs.name);
                for (Style& s : gs.vecStyle)
                {
                    const StyleClass* os = GetStyleClass(pTheme, s.sclass);
                    pGroup->AddSubItem(CreateProperty(s.name, pn(os, description), &s.theme, { pn(os, theme), &pTheme->tDefault }));
                }
                pParent->AddSubItem(pGroup);
            }
            if (!pLanguage->vecBase.empty())
            {
                CMFCPropertyGridProperty* pParent = pGroup;
#pragma warning(suppress:4456)
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Common"));
                for (Style& s : pLanguage->vecBase)
                {
                    const Style* os = GetKey(pTheme->vecBase, s.id);
                    CString sclass = Merge(s.sclass, pn(os, sclass), CString(), CString());
                    const StyleClass* osc = GetStyleClass(pTheme, sclass);
                    pGroup->AddSubItem(CreateProperty(s.name, pn(osc, description), &s.theme, { pn(os, theme), pn(osc, theme), &pTheme->tDefault }));
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
        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("General"));
            pGroup->AddSubItem(CreateProperty(_T("Empty File on Startup"), &m_pSettings->bEmptyFileOnStartup));
            pGroup->AddSubItem(CreateProperty(_T("Number of Recently Used Files"), &m_pSettings->nMaxMRU, 1, 10));
            pGroup->AddSubItem(CreateProperty(_T("Default Encoding"), &m_pSettings->DefaultEncoding, g_encodingnames));
            pGroup->AddSubItem(CreateProperty(_T("Default Line Ending"), &m_pSettings->DefaultLineEnding, g_lineendingnames));
            m_wndPropList.AddProperty(pGroup);
        }
        {
            m_pExtGroup = new CMFCPropertyGridProperty(_T("Extensions"));
            FillExtensions();
            m_wndPropList.AddProperty(m_pExtGroup);
        }
    }
    else if (i == 1)    // Editor
    {
        Theme* pTheme = &m_pSettings->user;
        {
            CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Editor"));
            {
                CMFCPropertyGridProperty* pParent = pGroup;
#pragma warning(suppress:4456)
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Caret"), 0, TRUE);
                pGroup->AllowEdit(FALSE);
                pGroup->AddSubItem(CreateProperty(_T("Foreground"), &pTheme->editor.cCaretFG, { &m_pSettings->user.tDefault.fore }));
                pGroup->AddSubItem(CreateProperty(_T("Style"), &pTheme->editor.nCaretStyle, to_span(g_stylesnames).sub(1)));
                pGroup->AddSubItem(CreateProperty(_T("Width"), &pTheme->editor.nCaretWidth, 1, 4));
                pParent->AddSubItem(pGroup);
            }
            pGroup->AddSubItem(CreateProperty(_T("Use Tabs"), &pTheme->editor.bUseTabs, nullptr));
            pGroup->AddSubItem(CreateProperty(_T("Tab Width"), &pTheme->editor.nTabWidth, 1, 100));
            pGroup->AddSubItem(CreateProperty(_T("Indent Guides"), &pTheme->editor.nIndentGuideType, to_span(g_indentguidesnames).sub(1)));
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
            pGroup1->AddSubItem(CreateProperty(_T("Default"), nullptr, &pTheme->tDefault, {}));
            for (StyleClass& sc : pTheme->vecStyleClass)
            {
                if (!sc.description.IsEmpty())
                    pGroup1->AddSubItem(CreateProperty(sc.description, nullptr, &sc.theme, { &pTheme->tDefault }));
            }
            {
                CMFCPropertyGridProperty* pParent = pGroup1;
                CMFCPropertyGridProperty* pGroup = new CMFCPropertyGridProperty(_T("Common"));
                for (Style& s : pTheme->vecBase)
                {
                    const StyleClass* osc = GetStyleClass(pTheme, s.sclass);
                    pGroup->AddSubItem(CreateProperty(s.name, nullptr, &s.theme, { pn(osc, theme), &pTheme->tDefault }));
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
                pMarkerGroup->AddSubItem(CreateProperty(_T("Fore"), &marker.fore));
                pMarkerGroup->AddSubItem(CreateProperty(_T("Back"), &marker.back));
                pGroup->AddSubItem(pMarkerGroup);
            }
            m_wndPropList.AddProperty(pGroup);
        }
    }
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

static void Refresh(CMFCPropertyGridProperty* pPropChild, const PropertyBase* propdef)
{
    for (int i = 0; i < pPropChild->GetSubItemsCount(); ++i)
        Refresh(pPropChild->GetSubItem(i), propdef);

    const PropertyBase* pPropProp = (const PropertyBase*) pPropChild->GetData();
    if (pPropProp != nullptr && pPropProp->IsDefault(propdef))
        pPropProp->Refresh(pPropChild, propdef);
}

LRESULT CPropertiesWnd::OnPropertyChanged(WPARAM /*wParam*/, LPARAM lParam)
{
    CMFCPropertyGridProperty* pProp = reinterpret_cast<CMFCPropertyGridProperty*>(lParam);
    PropertyBase* prop = (PropertyBase*) pProp->GetData();
    if (prop != nullptr)
        prop->SetProperty(pProp);

    for (int i = 0; i < m_wndPropList.GetPropertyCount(); ++i)
        Refresh(m_wndPropList.GetProperty(i), prop);

    theApp.NotifySettingsChanged();

    return 0;
}

void CPropertiesWnd::OnComboSelChange()
{
    for (int i = 0; i < m_wndPropList.GetPropertyCount(); ++i)
        CleanUp(m_wndPropList.GetProperty(i));
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

void CPropertiesWnd::FillExtensions()
{
    for (auto& ext : m_pSettings->user.mapExt)
    {
        const Language* pLanguage = GetLanguage(&m_pSettings->user, ext.second);
        m_pExtGroup->AddSubItem(CreateProperty(ext.first, pLanguage != nullptr ? pLanguage->title : m_LanguageNames[0], &ext.second, m_LanguageNames, m_LanguageValues));
    }
    m_pExtGroup->Expand(FALSE);
}

void CPropertiesWnd::OnDestroy()
{
    CDockablePane::OnDestroy();

    for (int i = 0; i < m_wndPropList.GetPropertyCount(); ++i)
        CleanUp(m_wndPropList.GetProperty(i));
}
