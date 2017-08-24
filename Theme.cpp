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

    if (pLanguage != nullptr)
    {
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
                // TODO Check for no other attributes
                else
                {
                    StyleClass* pStyleClass = Get(pTheme->vecStyleClass, name);

                    ThemeItem rThemeItem;
                    if (name == L"default")
                        rThemeItem = pTheme->tDefault;
                    else if (pStyleClass != nullptr)
                        rThemeItem = pStyleClass->theme;

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

                if (isnull(name) || ((LPCTSTR) name)[0] == '\0')
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else if (isnull(key))
                {
                    CString msg;
                    msg.Format(_T("Missing key: %s"), (LPCTSTR) name);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                // TODO Check for no other attributes
                else
                {
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
                            vecStyles.push_back({ name, nKey, sclass, rThemeItem });
                        else
                        {
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
                // TODO Check for no other attributes
                if (vecGroupStyles == nullptr)
                {
                    CString msg;
                    msg.Format(_T("Subgroup not possible: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                else
                {
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
                    pLanguage->vecKeywords[_wtoi(key)] = { name, sclass };
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
            }
            else if (bstrName == L"comments")
            {
                // TODO
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

    PathCombine(full, strDirectory, _T("scheme.master"));
    if (PathFileExists(full))
        LoadScheme(full, pTheme, vecBaseLanguage);

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

bool IsEmpty(MSXML2::IXMLDOMElementPtr pXMLNode)
{
    MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLNode->GetchildNodes());
    return pXMLChildren->Getlength() <= 0;
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

            pStyleClass->setAttribute(_T("id"), s.id);
            SaveTheme(pStyleClass, s.theme, os->theme);
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
        const GroupStyle* ogs = Get(vecDefaultStyle, gs.name);
        if (ogs != nullptr)
        {
            SaveTheme(pDoc, pParent, gs.vecStyle, ogs->vecStyle);
        }
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
        // ignore mapExt, mapExtFilter
        for (const Language& l : pTheme->vecLanguage)
        {
            const Language* ol = Get(pDefaultTheme->vecLanguage, l.name);
            if (ol != nullptr)
            {
                // ignore mapProperties, vecKeywords
                // ignore title and lexer
                // TODO SaveTheme(pDoc, pParent, .vecStyle, ol->vecStyle);
                // TODO SaveTheme(pDoc, pParent, l.vecGroupStyle, ol->vecGroupStyle);
            }
        }

        if (IsEmpty(pStyleClasses))
            pRootNode->removeChild(pStyleClasses);
        if (IsEmpty(pBaseOptions))
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
