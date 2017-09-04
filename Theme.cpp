#include "stdafx.h"
#include "Theme.h"
#include <SciLexer.h>

static inline LOGFONT Font(int size, LPCWSTR face, bool bold = false)
{
    LOGFONT lf = {};
    lf.lfHeight = size;
    if (face != nullptr)
        wcscpy_s(lf.lfFaceName, face);
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    return lf;
}

template<class T>
static inline typename T::const_pointer Get(const T& vec, LPCTSTR name)
{
    for (T::const_reference v : vec)
    {
        if (_wcsicmp(v.name, name) == 0)
            return &v;
    }
    return nullptr;
}

template<class T>
static inline typename T::pointer Get(T& vec, LPCTSTR name)
{
    for (T::reference v : vec)
    {
        if (_wcsicmp(v.name, name) == 0)
            return &v;
    }
    return nullptr;
}

template<class T>
static inline typename T::const_pointer GetKey(const T& vec, int id)
{
    for (T::const_reference v : vec)
    {
        if (v.id == id)
            return &v;
    }
    return nullptr;
}

template<class T>
static inline typename T::pointer GetKey(T& vec, int id)
{
    for (T::reference v : vec)
    {
        if (v.id == id)
            return &v;
    }
    return nullptr;
}

static inline const ThemeItem* GetThemeItem(LPCTSTR strItem, const Theme* pTheme)
{
    if (_wcsicmp(strItem, _T("default")) == 0)
        return &pTheme->tDefault;
    else
    {
        const StyleClass* pStyleClass = Get(pTheme->vecStyleClass, strItem);
        return pStyleClass == nullptr ? nullptr : &pStyleClass->theme;
    }
};

static inline void ApplyThemeItem(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme)
{
    if (rTheme.fore != COLOR_NONE)
        rCtrl.StyleSetFore(nStyle, rTheme.fore);
    if (rTheme.back != COLOR_NONE)
        rCtrl.StyleSetBack(nStyle, rTheme.back);
    if (rTheme.font.lfHeight != 0)
    {
        CWindowDC dc(&rCtrl);
        int nLogY = dc.GetDeviceCaps(LOGPIXELSY);
        if (nLogY != 0)
        {
            int pt = MulDiv(72, -rTheme.font.lfHeight, nLogY);
            rCtrl.StyleSetSize(nStyle, pt);
        }
    }
    if (rTheme.font.lfFaceName[0] != _T('\0'))
        rCtrl.StyleSetFont(nStyle, rTheme.font.lfFaceName);
    rCtrl.StyleSetCharacterSet(nStyle, rTheme.font.lfCharSet);
    if (rTheme.font.lfWeight >= FW_BOLD)
        rCtrl.StyleSetBold(nStyle, TRUE);
    if (rTheme.font.lfItalic)
        rCtrl.StyleSetItalic(nStyle, TRUE);
    if (rTheme.font.lfUnderline)
        rCtrl.StyleSetUnderline(nStyle, TRUE);
    if (rTheme.eolfilled)
        rCtrl.StyleSetEOLFilled(nStyle, TRUE);
    if (rTheme.hotspot)
        rCtrl.StyleSetHotSpot(nStyle, TRUE);
}

const Language* GetLanguage(const Theme* pTheme, LPCTSTR strName)
{
    return Get(pTheme->vecLanguage, strName);
}

const Language* GetLanguageForExt(const Theme* pTheme, LPCTSTR strExt)
{
    std::map<CString, CString>::const_iterator it = pTheme->mapExt.find(CString(strExt).MakeLower());
    if (it == pTheme->mapExt.end())
        return nullptr;
    else
        return GetLanguage(pTheme, it->second);
}

const StyleClass* GetStyleClass(const Theme* pTheme, LPCTSTR strName)
{
    return Get(pTheme->vecStyleClass, strName);
}

void AddExt(Theme* pTheme, const CString& ext, const CString& lexer)
{
    pTheme->mapExt[ext] = lexer;

    CString& filter = pTheme->mapExtFilter[lexer];
    if (!filter.IsEmpty())
        filter += _T(';');
    filter += ext;
}

static inline void ApplyStyle(CScintillaCtrl& rCtrl, const Style& style, const Theme* pTheme)
{
    const ThemeItem* pThemeItem = GetThemeItem(style.sclass, pTheme);
    if (pThemeItem != nullptr)
        ApplyThemeItem(rCtrl, style.id, *pThemeItem);
    ApplyThemeItem(rCtrl, style.id, style.theme);
}

static inline void ApplyMarker(CScintillaCtrl& rCtrl, const Marker& marker)
{
    rCtrl.MarkerDefine(marker.id, marker.type);
    rCtrl.MarkerSetFore(marker.id, marker.fore);
    rCtrl.MarkerSetBack(marker.id, marker.back);
}

void ApplyMargin(CScintillaCtrl& rCtrl, const Margin& margin)
{
    int w = 0;
    if (margin.show)
    {
        if (!margin.width_text.IsEmpty())
            w = rCtrl.TextWidth(STYLE_LINENUMBER, margin.width_text);
        else
            w = margin.width;
    }
    rCtrl.SetMarginWidthN(margin.id, w);
    rCtrl.SetMarginSensitiveN(margin.id, margin.sensitive);
    if (margin.type >= 0)
        rCtrl.SetMarginTypeN(margin.id, margin.type);
    if (margin.mask != 0)
        rCtrl.SetMarginMaskN(margin.id, margin.mask);
}

void Apply(CScintillaCtrl& rCtrl, const Language* pLanguage, const Theme* pTheme)
{
    if (pLanguage != nullptr)
        rCtrl.SetLexerLanguage(pLanguage->lexer);
    else
        rCtrl.SetLexer(SCLEX_NULL);

    ApplyThemeItem(rCtrl, STYLE_DEFAULT, pTheme->tDefault);
    rCtrl.StyleClearAll();
    for (const Style& style : pTheme->vecBase)
        ApplyStyle(rCtrl, style, pTheme);
    for (const Marker& marker : pTheme->vecMarker)
        ApplyMarker(rCtrl, marker);
    for (const Margin& margin : pTheme->vecMargin)
        ApplyMargin(rCtrl, margin);

    if (pLanguage != nullptr)
    {
        if (!pLanguage->strWordChars.IsEmpty())
            rCtrl.SetWordChars(pLanguage->strWordChars);
        else
            rCtrl.SetCharsDefault();

        for (int i = 0; i < KEYWORDSET_MAX; ++i)
        {
            const CString sclass = pLanguage->vecKeywords[i].sclass;
            const KeywordClass* pKeywordClass = Get(pTheme->vecKeywordClass, sclass);
            if (pKeywordClass != nullptr)
                rCtrl.SetKeyWords(i, pKeywordClass->keywords);
        }

        for (const auto& prop : pLanguage->mapProperties)
            rCtrl.SetProperty(prop.first, prop.second);
        for (const Style& style : pLanguage->vecStyle)
            ApplyStyle(rCtrl, style, pTheme);
        for (const GroupStyle& groupstyle : pLanguage->vecGroupStyle)
        {
            for (const Style& style : groupstyle.vecStyle)
                ApplyStyle(rCtrl, style, pTheme);
        }
    }
}


#import <MSXML6.dll> exclude("ISequentialStream", "_FILETIME")

inline _bstr_t GetAttribute(MSXML2::IXMLDOMNode* pXMLNode, LPCWSTR name)
{
    MSXML2::IXMLDOMNamedNodeMapPtr pXMLAttributes(pXMLNode->Getattributes());

    MSXML2::IXMLDOMNodePtr pXMLAttr(pXMLAttributes->getNamedItem(const_cast<LPWSTR>(name)));

    _bstr_t bstrAttrValue;
    if (pXMLAttr)
        bstrAttrValue = pXMLAttr->Gettext();
    //pXMLAttr->GetnodeValue();   // TODO Use this instead???

    return bstrAttrValue;
}

inline int IsEmpty(MSXML2::IXMLDOMNode* pXMLNode, DOMNodeType findtype)
{
    int count = 0;
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == findtype)
            ++count;
    }
    return count == 0;
}


inline bool operator==(const _bstr_t& s1, LPCWSTR s2)
{
    return wcscmp(s1, s2) == 0;
}

inline bool isnull(LPCWSTR s)
{
    return s == nullptr;
}

inline COLORREF ToColor(LPCWSTR s)
{
    unsigned long o = wcstoul(s, nullptr, 16);
    // TODO Support Alpha?
    return RGB(GetBValue(o), GetGValue(o), GetRValue(o));
}

void LoadThemeItem(MSXML2::IXMLDOMNodePtr pXMLNode, ThemeItem& rThemeItem)
{
    MSXML2::IXMLDOMNamedNodeMapPtr pXMLAttributes(pXMLNode->Getattributes());
    long length = pXMLAttributes->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLAttributes->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ATTRIBUTE)
        {
            _bstr_t bstrName = pXMLChildNode->nodeName;
            if (bstrName == _T("name"))
                ; // ignore
            else if (bstrName == _T("inherit-style"))
                ; // ignore
            else if (bstrName == _T("description"))
                ; // ignore
            else if (bstrName == _T("key"))
                ; // ignore
            else if (bstrName == _T("class"))
                ; // ignore
            else if (bstrName == _T("fore"))
                rThemeItem.fore = ToColor(pXMLChildNode->text);
            else if (bstrName == _T("back"))
                rThemeItem.back = ToColor(pXMLChildNode->text);
            else if (bstrName == _T("face"))
                wcscpy_s(rThemeItem.font.lfFaceName, pXMLChildNode->text);
            else if (bstrName == _T("size"))
            {
                HDC hDC = ::GetWindowDC(NULL);
                rThemeItem.font.lfHeight = -MulDiv(_wtoi(pXMLChildNode->text), GetDeviceCaps(hDC, LOGPIXELSY), 72);
                ::ReleaseDC(NULL, hDC);
            }
            else if (bstrName == _T("bold"))
                rThemeItem.font.lfWeight = pXMLChildNode->text == _T("true") ? FW_BOLD : FW_NORMAL;
            else if (bstrName == _T("italic"))
                rThemeItem.font.lfItalic = pXMLChildNode->text == _T("true");
            else if (bstrName == _T("hotspot"))
                rThemeItem.hotspot = pXMLChildNode->text == _T("true");
            else if (bstrName == _T("eolfilled"))
                rThemeItem.eolfilled = pXMLChildNode->text == _T("true");
            else
            {
                CString msg;
                msg.Format(_T("Unknown attribute: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessStyleClasses(MSXML2::IXMLDOMNodePtr pXMLNode, Theme* pTheme)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"style-class")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t description = GetAttribute(pXMLChildNode, _T("description"));
                _bstr_t inherit_style = GetAttribute(pXMLChildNode, _T("inherit-style"));

                if (isnull(name) || ((LPCTSTR) name)[0] == '\0')
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                    {
                        CString msg;
                        msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    // TODO Check for no other attributes

                    StyleClass* pStyleClass = Get(pTheme->vecStyleClass, name);

                    ThemeItem rThemeItem;
                    if (name == L"default")
                        rThemeItem = pTheme->tDefault;

                    if (!isnull(inherit_style))
                    {
                        const ThemeItem* pThemItem = GetThemeItem(inherit_style, pTheme);
                        rThemeItem = *pThemItem;
                    }

                    LoadThemeItem(pXMLChildNode, rThemeItem);

                    if (name == L"default")
                        pTheme->tDefault = rThemeItem;
                    else if (pStyleClass == nullptr)
                        pTheme->vecStyleClass.push_back({ name, description, rThemeItem });
                    else
                    {
                        if (!isnull(description))
                            pStyleClass->description = (LPCTSTR) description;
                        pStyleClass->theme = rThemeItem;
                    }
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessStyles(MSXML2::IXMLDOMNodePtr pXMLNode, std::vector<Style>& vecStyles, std::vector<GroupStyle>* vecGroupStyles)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"style")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t key = GetAttribute(pXMLChildNode, _T("key"));
                _bstr_t sclass = GetAttribute(pXMLChildNode, _T("class"));

                if (isnull(key))
                {
                    CString msg;
                    msg.Format(_T("Missing key: %s"), (LPCTSTR) name);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                    {
                        CString msg;
                        msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    // TODO Check for no other attributes

                    int nKey = _wtoi(key);
                    Style* pStyle = GetKey(vecStyles, nKey);
                    if (pStyle != nullptr && pStyle->id != nKey)
                    {
                        CString msg;
                        msg.Format(_T("Mismatch key: %s"), (LPCTSTR) name);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    else
                    {
                        ThemeItem rThemeItem;
                        if (pStyle != nullptr)
                            rThemeItem = pStyle->theme;
                        LoadThemeItem(pXMLChildNode, rThemeItem);

                        if (pStyle == nullptr)
                        {
                            if (isnull(name) || ((LPCTSTR) name)[0] == '\0')
                            {
                                CString msg;
                                msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                            }
                            vecStyles.push_back({ name, nKey, sclass, rThemeItem });
                        }
                        else
                        {
                            if (!isnull(name))
                                pStyle->name = (LPCTSTR) name;
                            if (!isnull(sclass))
                                pStyle->sclass = (LPCTSTR) sclass;
                            pStyle->theme = rThemeItem;
                        }
                    }
                }
            }
            else if (bstrName == L"group")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t description = GetAttribute(pXMLChildNode, _T("description"));
                _bstr_t sclass = GetAttribute(pXMLChildNode, _T("class"));

                if (isnull(name))
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                if (vecGroupStyles == nullptr)
                {
                    CString msg;
                    msg.Format(_T("Subgroup not possible: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    // TODO Check for no other attributes

                    GroupStyle* pGroupStyle = Get(*vecGroupStyles, name);
                    if (pGroupStyle == nullptr)
                    {
                        vecGroupStyles->push_back(GroupStyle());
                        pGroupStyle = &vecGroupStyles->back();
                        pGroupStyle->name = (LPCTSTR) name;
                    }
                    ProcessStyles(pXMLChildNode, pGroupStyle->vecStyle, nullptr);
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessMargins(MSXML2::IXMLDOMNodePtr pXMLNode, std::vector<Margin>& vecMargins)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"margin")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t key = GetAttribute(pXMLChildNode, _T("key"));
                _bstr_t show = GetAttribute(pXMLChildNode, _T("show"));
                _bstr_t width = GetAttribute(pXMLChildNode, _T("width"));
                _bstr_t width_text = GetAttribute(pXMLChildNode, _T("width-text"));
                _bstr_t sensitive = GetAttribute(pXMLChildNode, _T("sensitive"));
                _bstr_t stype = GetAttribute(pXMLChildNode, _T("type"));
                _bstr_t mask = GetAttribute(pXMLChildNode, _T("mask"));

                if (isnull(key))
                {
                    CString msg;
                    msg.Format(_T("Missing key: %s"), (LPCTSTR) name);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                    {
                        CString msg;
                        msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    // TODO Check for no other attributes

                    int nKey = _wtoi(key);
                    Margin* pMargin = GetKey(vecMargins, nKey);
                    if (pMargin == nullptr)
                    {
                        if (isnull(name) || ((LPCTSTR) name)[0] == '\0')
                        {
                            CString msg;
                            msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                            AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                        }
                        vecMargins.push_back({ name, nKey });
                        pMargin = &vecMargins.back();
                        pMargin->type = -1;
                    }

                    if (!isnull(name))
                        pMargin->name = (LPCTSTR) name;
                    if (!isnull(show))
                        pMargin->show = show == _T("true");
                    if (!isnull(width))
                        pMargin->width = _wtoi(width);
                    if (!isnull(width_text))
                        pMargin->width_text = (LPCTSTR) width_text;
                    if (!isnull(sensitive))
                        pMargin->sensitive = sensitive == _T("true");
                    if (!isnull(stype))
                        pMargin->type = _wtoi(stype);
                    if (!isnull(mask))
                        pMargin->mask = wcstoul(mask, nullptr, 16);
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessMarkers(MSXML2::IXMLDOMNodePtr pXMLNode, std::vector<Marker>& vecMarkers)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"marker")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t key = GetAttribute(pXMLChildNode, _T("key"));
                _bstr_t stype = GetAttribute(pXMLChildNode, _T("type"));
                _bstr_t fore = GetAttribute(pXMLChildNode, _T("fore"));
                _bstr_t back = GetAttribute(pXMLChildNode, _T("back"));

                if (isnull(key))
                {
                    CString msg;
                    msg.Format(_T("Missing key: %s"), (LPCTSTR) name);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                    {
                        CString msg;
                        msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    // TODO Check for no other attributes

                    int nKey = _wtoi(key);
                    Marker* pMarker = GetKey(vecMarkers, nKey);
                    if (pMarker == nullptr)
                    {
                        if (isnull(name) || ((LPCTSTR) name)[0] == '\0')
                        {
                            CString msg;
                            msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                            AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                        }
                        vecMarkers.push_back({ name, nKey });
                        pMarker = &vecMarkers.back();
                    }

                    if (!isnull(name))
                        pMarker->name = (LPCTSTR) name;
                    if (!isnull(stype))
                        pMarker->type = _wtoi(stype);
                    if (!isnull(fore))
                        pMarker->fore = ToColor(fore);
                    if (!isnull(back))
                        pMarker->back = ToColor(back);
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessKeywordClassesInclude(MSXML2::IXMLDOMNodePtr pXMLNode, Theme* pTheme, KeywordClass* pKeywordClass)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"include-class")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));

                if (isnull(name))
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                    {
                        CString msg;
                        msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    // TODO Check for no other attributes

                    KeywordClass* pIncludeKeywordClass = Get(pTheme->vecKeywordClass, name);
                    if (pIncludeKeywordClass != nullptr)
                    {
                        pKeywordClass->keywords += _T(" ");
                        pKeywordClass->keywords += pIncludeKeywordClass->keywords;
                    }
                    else
                    {
                        CString msg;
                        msg.Format(_T("Cannot find include-class: %s"), (LPCTSTR) name);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessKeywordClasses(MSXML2::IXMLDOMNodePtr pXMLNode, Theme* pTheme)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"keyword-class")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t keywords = pXMLChildNode->text;

                if (isnull(name))
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else if (isnull(keywords))
                {
                    CString msg;
                    msg.Format(_T("Missing keywords: %s"), (LPCTSTR) name);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    // TODO Check for no other attributes

                    KeywordClass* pKeywordClass = Get(pTheme->vecKeywordClass, name);
                    if (pKeywordClass == nullptr)
                    {
                        pTheme->vecKeywordClass.push_back({ name, keywords });
                        pKeywordClass = &pTheme->vecKeywordClass.back();
                    }
                    else
                        pKeywordClass->keywords = (LPCTSTR) keywords;

                    ProcessKeywordClassesInclude(pXMLChildNode, pTheme, pKeywordClass);
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessKeywords(MSXML2::IXMLDOMNodePtr pXMLNode, Language* pLanguage)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"keyword")
            {
                _bstr_t key = GetAttribute(pXMLChildNode, _T("key"));
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t sclass = GetAttribute(pXMLChildNode, _T("class"));

                if (isnull(key))
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else if (isnull(name))
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) key);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
                    if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                    {
                        CString msg;
                        msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                    }
                    // TODO Check for no other attributes

                    pLanguage->vecKeywords[_wtoi(key)] = { name, sclass };
                }
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessLanguage(MSXML2::IXMLDOMNodePtr pXMLNode, Language* pLanguage)
{
    {
        _bstr_t title = GetAttribute(pXMLNode, _T("title"));
        if (!isnull(title))
            pLanguage->title = (LPCTSTR) title;
        _bstr_t wordchars = GetAttribute(pXMLNode, _T("wordchars"));
        if (!isnull(wordchars))
            pLanguage->strWordChars = (LPCTSTR) wordchars;
        _bstr_t internal = GetAttribute(pXMLNode, _T("internal"));
        if (!isnull(internal))
            pLanguage->internal = internal == _T("true");
        // TODO _bstr_t casesensitive = GetAttribute(pXMLChildNode, _T("casesensitive"));
        // TODO _bstr_t usetabs = GetAttribute(pXMLChildNode, _T("usetabs"));
    }

    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"lexer")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                pLanguage->lexer = (LPCTSTR) name;

                if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                {
                    CString msg;
                    msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                // TODO Check for no other attributes
            }
            else if (bstrName == L"use-styles")
            {
                ProcessStyles(pXMLChildNode, pLanguage->vecStyle, &pLanguage->vecGroupStyle);
            }
            else if (bstrName == L"use-keywords")
            {
                ProcessKeywords(pXMLChildNode, pLanguage);
            }
            else if (bstrName == L"property")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t value = GetAttribute(pXMLChildNode, _T("value"));
                pLanguage->mapProperties[(LPCTSTR) name] = (LPCTSTR) value;

                if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                {
                    CString msg;
                    msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                // TODO Check for no other attributes
            }
            else if (bstrName == L"comments")
            {
                // TODO ...

                if (!IsEmpty(pXMLChildNode, NODE_ELEMENT))
                {
                    CString msg;
                    msg.Format(_T("Extra elements: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                // TODO Check for no other attributes
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void ProcessScheme(MSXML2::IXMLDOMNodePtr pXMLNode, Theme* pTheme, std::vector<Language>& vecBaseLanguage)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    long length = pXMLChildren->Getlength();
    for (int i = 0; i < length; ++i)
    {
        MSXML2::IXMLDOMNodePtr pXMLChildNode(pXMLChildren->Getitem(i));
        MSXML2::DOMNodeType type = pXMLChildNode->GetnodeType();

        if (type == NODE_ELEMENT)
        {
            _bstr_t bstrName = pXMLChildNode->GetbaseName();

            if (bstrName == L"style-classes")
            {
                ProcessStyleClasses(pXMLChildNode, pTheme);
            }
            else if (bstrName == L"base-options")
            {
                ProcessStyles(pXMLChildNode, pTheme->vecBase, nullptr);
            }
            else if (bstrName == L"keyword-classes")
            {
                ProcessKeywordClasses(pXMLChildNode, pTheme);
            }
            else if (bstrName == L"margins")
            {
                ProcessMargins(pXMLChildNode, pTheme->vecMargin);
            }
            else if (bstrName == L"markers")
            {
                ProcessMarkers(pXMLChildNode, pTheme->vecMarker);
            }
            else if (bstrName == L"base-language")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                Language* pLanguage = Get(vecBaseLanguage, name);
                if (pLanguage == nullptr)
                {
                    vecBaseLanguage.push_back(Language(name));
                    pLanguage = &vecBaseLanguage.back();
                }
                ProcessLanguage(pXMLChildNode, pLanguage);
            }
            else if (bstrName == L"language")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t base = GetAttribute(pXMLChildNode, _T("base"));

                const Language* pBaseLanguage = isnull(base) ? nullptr : Get(vecBaseLanguage, base);
                if (!isnull(base) && pBaseLanguage == nullptr)
                {
                    CString msg;
                    msg.Format(_T("Cannot find base language: %s"), (LPCTSTR) base);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }

                Language* pLanguage = Get(pTheme->vecLanguage, name);
                if (pLanguage == nullptr)
                {
                    pTheme->vecLanguage.push_back(Language(name, pBaseLanguage));
                    pLanguage = &pTheme->vecLanguage.back();
                }
                else if (pBaseLanguage != nullptr)
                {
                    CString msg;
                    msg.Format(_T("Cannot use base language: %s"), (LPCTSTR) base);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                ProcessLanguage(pXMLChildNode, pLanguage);
            }
            else
            {
                CString msg;
                msg.Format(_T("Unknown element: %s"), (LPCTSTR) bstrName);
                AfxMessageBox(msg, MB_ICONERROR | MB_OK);
            }
        }
    }
}

void LoadScheme(LPCTSTR pFilename, Theme* pTheme, std::vector<Language>& vecBaseLanguage)
{
    MSXML2::IXMLDOMDocumentPtr pDoc(__uuidof(MSXML2::DOMDocument60));
    pDoc->Putasync(VARIANT_FALSE);
    pDoc->PutvalidateOnParse(VARIANT_FALSE);
    pDoc->PutresolveExternals(VARIANT_FALSE);
    pDoc->PutpreserveWhiteSpace(VARIANT_TRUE);

    VARIANT_BOOL varStatus = pDoc->load(pFilename);
    if (varStatus == VARIANT_TRUE)
    {
        MSXML2::IXMLDOMElementPtr pXMLRoot(pDoc->GetdocumentElement());
        ProcessScheme(pXMLRoot, pTheme, vecBaseLanguage);
    }
    else
    {
        MSXML2::IXMLDOMParseErrorPtr pXMLErr(pDoc->GetparseError());

        CString msg;
        msg.Format(_T("%s(%ld, %ld): Code: 0x%x - %s\n"), pFilename, pXMLErr->line, pXMLErr->linepos, pXMLErr->errorCode, (LPCTSTR) pXMLErr->reason);
        AfxMessageBox(msg, MB_ICONERROR | MB_OK);
    }
}

void LoadSchemeDirectory(LPCTSTR strDirectory, Theme* pTheme, std::vector<Language>& vecBaseLanguage)
{
    TCHAR full[_MAX_PATH];

    PathCombine(full, strDirectory, _T("*.scheme"));
    WIN32_FIND_DATA fd = {};
    HANDLE hFind;
    if ((hFind = FindFirstFile(full, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            PathCombine(full, strDirectory, fd.cFileName);
            LoadScheme(full, pTheme, vecBaseLanguage);
        } while (FindNextFile(hFind, &fd));
    }

    PathCombine(full, strDirectory, _T("extmap.dat"));
    if (PathFileExists(full))
    {
        TCHAR line[1024];
        FILE* f = nullptr;
        if (_wfopen_s(&f, full, L"r") == 0)
        {
            while (fgetws(line, ARRAYSIZE(line), f))
            {
                const TCHAR* equals = wcschr(line, _T('='));
                if (equals != nullptr)
                    AddExt(pTheme, CString(line, (int) (equals - line)).Trim().MakeLower(), CString(equals + 1).Trim().MakeLower());
            }
            fclose(f);
        }
    }
}

struct EnumResData
{
    Theme* pTheme;
    std::vector<Language>* pvecBaseLanguage;
};

BOOL CALLBACK EnumResSchemeProc(
    HMODULE  hModule,
    LPCTSTR  lpszType,
    LPTSTR   lpszName,
    LONG_PTR lParam
)
{
    EnumResData* pData = (EnumResData*) lParam;

    HRSRC hResInfo = FindResource(hModule, lpszName, lpszType);
    HGLOBAL hGlobal = LoadResource(hModule, hResInfo);
    const char* str = (const char*) LockResource(hGlobal);
    if (str)
    {
        DWORD nSize = SizeofResource(hModule, hResInfo);
        if (strncmp(str, "\xEF\xBB\xBF", 3) == 0)
        {   // UTF8 bom
            str += 3;
            nSize -= 3;
        }
        std::string xml(str, nSize); // Needed so it is null-terminated

        MSXML2::IXMLDOMDocumentPtr pDoc(__uuidof(MSXML2::DOMDocument60));
        pDoc->Putasync(VARIANT_FALSE);
        pDoc->PutvalidateOnParse(VARIANT_FALSE);
        pDoc->PutresolveExternals(VARIANT_FALSE);
        pDoc->PutpreserveWhiteSpace(VARIANT_TRUE);

        VARIANT_BOOL varStatus = pDoc->loadXML(xml.c_str());
        if (varStatus == VARIANT_TRUE)
        {
            MSXML2::IXMLDOMElementPtr pXMLRoot(pDoc->GetdocumentElement());
            ProcessScheme(pXMLRoot, pData->pTheme, *pData->pvecBaseLanguage);
        }
        else
        {
            MSXML2::IXMLDOMParseErrorPtr pXMLErr(pDoc->GetparseError());

            CString msg;
            msg.Format(_T("RES(%ld, %ld): Code: 0x%x - %s\n"), pXMLErr->line, pXMLErr->linepos, pXMLErr->errorCode, (LPCTSTR) pXMLErr->reason);
            AfxMessageBox(msg, MB_ICONERROR | MB_OK);
        }
    }
    FreeResource(hGlobal);
    return TRUE;
}

BOOL CALLBACK EnumResExtMapProc(
    HMODULE  hModule,
    LPCTSTR  lpszType,
    LPTSTR   lpszName,
    LONG_PTR lParam
)
{
    EnumResData* pData = (EnumResData*) lParam;

    HRSRC hResInfo = FindResource(hModule, lpszName, lpszType);
    HGLOBAL hGlobal = LoadResource(hModule, hResInfo);
    const char* str = (const char*) LockResource(hGlobal);
    if (str)
    {
        DWORD nSize = SizeofResource(hModule, hResInfo);

        while (nSize > 0)
        {
            const char* begin = str;
            const char* equals = strchr(begin, _T('='));
            const char* end = strchr(equals != nullptr ? equals : begin, _T('\n'));

            if (equals != nullptr)
                AddExt(pData->pTheme, CString(begin, (int) (equals - begin)).Trim().MakeLower(), CString(equals + 1, (int) (end - equals - 1)).Trim().MakeLower());

            nSize -= (int) (end - str + 1);
            str = end + 1;
        }
    }
    FreeResource(hGlobal);
    return TRUE;
}

#include "Resource.h"

void LoadTheme(Theme* pTheme, Theme* pDefaultTheme)
{
    std::vector<Language> vecBaseLanguage;

    EnumResData data;
    data.pTheme = pTheme;
    data.pvecBaseLanguage = &vecBaseLanguage;
    HMODULE hModule = NULL;
    EnumResourceNames(hModule, _T("SCHEME"), EnumResSchemeProc, (LONG_PTR) &data);
    EnumResourceNames(hModule, _T("EXTMAP"), EnumResExtMapProc, (LONG_PTR) &data);

    TCHAR exepath[_MAX_PATH];

    GetModuleFileName(hModule, exepath, MAX_PATH);
    PathFindFileName(exepath)[0] = _T('\0');
    LoadSchemeDirectory(exepath, pTheme, vecBaseLanguage);

    TCHAR szPath[_MAX_PATH];
    PathCombine(szPath, exepath, _T("schemes"));
    LoadSchemeDirectory(szPath, pTheme, vecBaseLanguage);

#if 0
    GetCurrentDirectory(MAX_PATH, path);
    if (wcscmp(szPath, exepath) != 0)
    {
        LoadSchemeDirectory(szPath, pTheme, vecBaseLanguage);

        PathCombine(szPath, szPath, _T("schemes"));
        LoadSchemeDirectory(path, pTheme, vecBaseLanguage);
    }
#endif

    *pDefaultTheme = *pTheme;

    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath)))
    {
        PathAppend(szPath, _T("RadSoft\\RadNotepad"));
        PathAppend(szPath, _T("scheme.user"));
        if (PathFileExists(szPath))
            LoadScheme(szPath, pTheme, vecBaseLanguage);
    }
}

const VARIANT vtnull = { VT_NULL };

_bstr_t ToBStr(COLORREF c)
{
    WCHAR str[10];
    wsprintf(str, L"%02x%02x%02x", GetRValue(c), GetGValue(c), GetBValue(c));
    return str;
}

void SaveTheme(MSXML2::IXMLDOMElementPtr pNode, const ThemeItem& ti, const ThemeItem& dti)
{
    if (ti.fore != dti.fore)
        pNode->setAttribute(_T("fore"), ToBStr(ti.fore));
    if (ti.back != dti.back)
        pNode->setAttribute(_T("back"), ToBStr(ti.back));
    if (ti.font.lfHeight != dti.font.lfHeight)
        pNode->setAttribute(_T("size"), ti.font.lfHeight);    // TODO Convert MulDiv
    if (wcscmp(ti.font.lfFaceName, dti.font.lfFaceName) != 0)
        pNode->setAttribute(_T("face"), ti.font.lfFaceName);
    if (ti.font.lfWeight != dti.font.lfWeight)
        pNode->setAttribute(_T("bold"), ti.font.lfWeight == FW_BOLD);
    if (ti.font.lfItalic != dti.font.lfItalic)
        pNode->setAttribute(_T("italic"), ti.font.lfItalic);
    if (ti.eolfilled != dti.eolfilled)
        pNode->setAttribute(_T("eolfilled"), ti.eolfilled);
    if (ti.hotspot != dti.hotspot)
        pNode->setAttribute(_T("hotspot"), ti.hotspot);
}

void SaveTheme(MSXML2::IXMLDOMDocumentPtr pDoc, MSXML2::IXMLDOMElementPtr pParent, const std::vector<Style>& vecStyle, const std::vector<Style>& vecDefaultStyle)
{
    for (const Style& s : vecStyle)
    {
        // ignore name and class
        const Style* os = GetKey(vecDefaultStyle, s.id);
        if (os != nullptr && s.theme != os->theme)
        {
            MSXML2::IXMLDOMElementPtr pStyleClass = pDoc->createElement(L"style");
            pParent->insertBefore(pStyleClass, vtnull);

            pStyleClass->setAttribute(_T("key"), s.id);
            SaveTheme(pStyleClass, s.theme, os->theme);
        }
    }
}

void SaveTheme(MSXML2::IXMLDOMDocumentPtr pDoc, MSXML2::IXMLDOMElementPtr pParent, const std::vector<Margin>& vecMargin, const std::vector<Margin>& vecDefaultMargin)
{
    for (const Margin& m : vecMargin)
    {
        // ignore name
        const Margin* om = GetKey(vecDefaultMargin, m.id);
        if (om != nullptr && m != *om)
        {
            MSXML2::IXMLDOMElementPtr pMargin = pDoc->createElement(L"margin");
            pParent->insertBefore(pMargin, vtnull);

            pMargin->setAttribute(_T("key"), m.id);
            if (m.show != om->show)
                pMargin->setAttribute(_T("show"), m.show);
            if (m.show != om->show)
                pMargin->setAttribute(_T("width"), m.width);
            if (m.show != om->show)
                pMargin->setAttribute(_T("width-text"), m.width_text.GetString());
            if (m.show != om->show)
                pMargin->setAttribute(_T("sensitive"), m.sensitive);
            if (m.show != om->show)
                pMargin->setAttribute(_T("type"), m.type);
            if (m.show != om->show)
                pMargin->setAttribute(_T("mask"), m.mask);  // TODO Convert to hex string
        }
    }
}

void SaveTheme(MSXML2::IXMLDOMDocumentPtr pDoc, MSXML2::IXMLDOMElementPtr pParent, const std::vector<Marker>& vecMarker, const std::vector<Marker>& vecDefaultMarker)
{
    for (const Marker& m : vecMarker)
    {
        // ignore name
        const Marker* om = GetKey(vecDefaultMarker, m.id);
        if (om != nullptr && m != *om)
        {
            MSXML2::IXMLDOMElementPtr pMarker = pDoc->createElement(L"marker");
            pParent->insertBefore(pMarker, vtnull);

            pMarker->setAttribute(_T("key"), m.id);
            if (m.type != om->type)
                pMarker->setAttribute(_T("type"), m.type);
            if (m.fore != om->fore)
                pMarker->setAttribute(_T("fore"), ToBStr(m.fore));
            if (m.back != om->back)
                pMarker->setAttribute(_T("back"), ToBStr(m.back));
        }
    }
}

void SaveTheme(MSXML2::IXMLDOMDocumentPtr pDoc, MSXML2::IXMLDOMElementPtr pParent, const std::vector<StyleClass>& vecStyle, const std::vector<StyleClass>& vecDefaultStyle)
{
    for (const StyleClass& sc : vecStyle)
    {
        // ignore description
        const StyleClass* osc = Get(vecDefaultStyle, sc.name);
        if (osc != nullptr && sc.theme != osc->theme)
        {
            MSXML2::IXMLDOMElementPtr pStyleClass = pDoc->createElement(L"style-class");
            pParent->insertBefore(pStyleClass, vtnull);

            _bstr_t name = sc.name;
            pStyleClass->setAttribute(_T("name"), name);
            SaveTheme(pStyleClass, sc.theme, osc->theme);
        }
    }
}

void SaveTheme(MSXML2::IXMLDOMDocumentPtr pDoc, MSXML2::IXMLDOMElementPtr pParent, const std::vector<GroupStyle>& vecStyle, const std::vector<GroupStyle>& vecDefaultStyle)
{
    for (const GroupStyle& gs : vecStyle)
    {
        MSXML2::IXMLDOMElementPtr pGroup = pDoc->createElement(L"group");
        pGroup->setAttribute(_T("name"), gs.name.GetString());
        pParent->insertBefore(pGroup, vtnull);

        const GroupStyle* ogs = Get(vecDefaultStyle, gs.name);
        if (ogs != nullptr)
        {
            SaveTheme(pDoc, pGroup, gs.vecStyle, ogs->vecStyle);
        }

        if (IsEmpty(pGroup, NODE_ELEMENT))
            pParent->removeChild(pGroup);
    }
}

void SaveTheme(LPTSTR pFilename, const Theme* pTheme, const Theme* pDefaultTheme)
{
    try
    {
        MSXML2::IXMLDOMDocumentPtr pDoc(__uuidof(MSXML2::DOMDocument60));
        pDoc->Putasync(VARIANT_FALSE);
        pDoc->PutvalidateOnParse(VARIANT_FALSE);
        pDoc->PutresolveExternals(VARIANT_FALSE);
        pDoc->PutpreserveWhiteSpace(VARIANT_TRUE);
        MSXML2::IXMLDOMProcessingInstructionPtr pProcInstr = pDoc->createProcessingInstruction(L"xml", L"version=\"1.0\" encoding=\"UTF-8\"");
        pDoc->insertBefore(pProcInstr, vtnull);
        MSXML2::IXMLDOMElementPtr pRootNode = pDoc->createElement(L"Scheme");
        pDoc->insertBefore(pRootNode, vtnull);
        MSXML2::IXMLDOMElementPtr pStyleClasses = pDoc->createElement(L"style-classes");
        pRootNode->insertBefore(pStyleClasses, vtnull);
        MSXML2::IXMLDOMElementPtr pBaseOptions = pDoc->createElement(L"base-options");
        pRootNode->insertBefore(pBaseOptions, vtnull);
        MSXML2::IXMLDOMElementPtr pMargins = pDoc->createElement(L"margins");
        pRootNode->insertBefore(pMargins, vtnull);
        MSXML2::IXMLDOMElementPtr pMarkers = pDoc->createElement(L"markers");
        pRootNode->insertBefore(pMarkers, vtnull);

        if (pTheme->tDefault != pDefaultTheme->tDefault)
        {
            MSXML2::IXMLDOMElementPtr pStyleClass = pDoc->createElement(L"style-class");
            pStyleClasses->insertBefore(pStyleClass, vtnull);

            _bstr_t name = L"default";
            pStyleClass->setAttribute(_T("name"), name);
            SaveTheme(pStyleClass, pTheme->tDefault, pDefaultTheme->tDefault);
        }
        SaveTheme(pDoc, pStyleClasses, pTheme->vecStyleClass, pDefaultTheme->vecStyleClass);
        SaveTheme(pDoc, pBaseOptions, pTheme->vecBase, pDefaultTheme->vecBase);
        // ignore vecKeywordClass
        SaveTheme(pDoc, pMargins, pTheme->vecMargin, pDefaultTheme->vecMargin);
        SaveTheme(pDoc, pBaseOptions, pTheme->vecMarker, pDefaultTheme->vecMarker);
        // ignore mapExt, mapExtFilter
        for (const Language& l : pTheme->vecLanguage)
        {
            MSXML2::IXMLDOMElementPtr pLanguage = pDoc->createElement(L"language");
            pLanguage->setAttribute(_T("name"), l.name.GetString());
            pRootNode->insertBefore(pLanguage, vtnull);
            MSXML2::IXMLDOMElementPtr pUseStyles = pDoc->createElement(L"use-styles");
            pLanguage->insertBefore(pUseStyles, vtnull);
            const Language* ol = Get(pDefaultTheme->vecLanguage, l.name);
            if (ol != nullptr)
            {
                // ignore mapProperties, vecKeywords
                // ignore title and lexer
                SaveTheme(pDoc, pUseStyles, l.vecStyle, ol->vecStyle);
                SaveTheme(pDoc, pUseStyles, l.vecGroupStyle, ol->vecGroupStyle);
            }
            if (IsEmpty(pUseStyles, NODE_ELEMENT))
                pLanguage->removeChild(pUseStyles);
            if (IsEmpty(pLanguage, NODE_ELEMENT))
                pRootNode->removeChild(pLanguage);
        }

        if (IsEmpty(pStyleClasses, NODE_ELEMENT))
            pRootNode->removeChild(pStyleClasses);
        if (IsEmpty(pBaseOptions, NODE_ELEMENT))
            pRootNode->removeChild(pBaseOptions);

        // TODO Save formatted
        //if (!IsEmpty(pRootNode))
        {
            variant_t name = pFilename;
            pDoc->save(name);
        }
    }
    catch (const _com_error& e)
    {
        AfxMessageBox(e.ErrorMessage(), MB_ICONERROR | MB_OK);
    }
}

void SaveTheme(const Theme* pTheme, const Theme* pDefaultTheme)
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, szPath)))
    {
        PathAppend(szPath, _T("RadSoft\\RadNotepad"));
        if (!PathFileExists(szPath))
            SHCreateDirectory(NULL, szPath);

        PathAppend(szPath, _T("scheme.user"));
        SaveTheme(szPath, pTheme, pDefaultTheme);
    }
}
