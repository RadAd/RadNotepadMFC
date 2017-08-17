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
        if (v.name == name)
            return &v;
    }
    return nullptr;
}

template<class T>
static inline typename T::pointer Get(T& vec, LPCTSTR name)
{
    for (T::reference v : vec)
    {
        if (v.name == name)
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

static inline int GetThemeItemIndex(LPCTSTR strItem, const Theme* pTheme)
{
    if (_wcsicmp(strItem, _T("default")) == 0)
        return -2;
    for (int i = 0; i < pTheme->vecStyleClass.size(); ++i)
    {
        if (_wcsicmp(strItem, pTheme->vecStyleClass[i].name) == 0)
            return i;
    }
    return -1;
};

static inline const ThemeItem* GetThemeItem(LPCTSTR strItem, const Theme* pTheme)
{
    if (_wcsicmp(strItem, _T("default")) == 0)
        return &pTheme->tDefault;
    for (const StyleClass& sc : pTheme->vecStyleClass)
    {
        if (_wcsicmp(strItem, sc.name) == 0)
            return &sc.theme;
    }
    return nullptr;
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

void InitTheme(Theme* pTheme)
{
    pTheme->tDefault = { COLOR_BLACK, COLOR_WHITE, Font(-13, _T("Consolas")) };
    {
        pTheme->vecStyleClass.push_back({ _T("comment"),      _T("#Comment"),            { COLOR_LT_GREEN,     COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("number"),       _T("#Number"),             { COLOR_LT_CYAN,      COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("keyword"),      _T("#Word"),               { COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) } });
        pTheme->vecStyleClass.push_back({ _T("keyword2"),     _T("#Type"),               { COLOR_LT_CYAN,      COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("string"),       _T("#String"),             { COLOR_LT_MAGENTA,   COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("Identifier"),   _T("#Identifier"),         { COLOR_BLACK,        COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("preprocessor"), _T("#Preprocessor"),       { COLOR_LT_RED,       COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("preprocessor"), _T("#Operator"),           { COLOR_LT_YELLOW,    COLOR_NONE } });
        pTheme->vecStyleClass.push_back({ _T("error"),        _T("#Error"),              { COLOR_WHITE,        COLOR_LT_RED } });
    }
    {
        pTheme->vecBase.push_back({ _T("Indent Guide"), STYLE_INDENTGUIDE, _T("indentguide"), { COLOR_NONE,     COLOR_NONE } });  // TODO Should I add to scheme.master
        // TODO THEME_CONTROLCHAR Looks like only the font is used ?
        // TODO STYLE_INDENTGUIDE Looks like the font isn't used ?
        // TODO
        // STYLE_CALLTIP
        // STYLE_FOLDDISPLAYTEXT
    }
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

                if (isnull(name))
                {
                    CString msg;
                    msg.Format(_T("Missing name: %s"), (LPCTSTR) bstrName);
                    AfxMessageBox(msg, MB_ICONERROR | MB_OK);
                }
                // TODO Check for no other attributes
                else
                {
                    ThemeItem* pThemItemOrig = const_cast<ThemeItem*>(GetThemeItem(name, pTheme));

                    ThemeItem rThemeItem;
                    if (pThemItemOrig != nullptr)
                        rThemeItem = *pThemItemOrig;

                    if (!isnull(inherit_style))
                    {
                        const ThemeItem* pThemItem = GetThemeItem(inherit_style, pTheme);
                        rThemeItem = *pThemItem;
                    }

                    LoadThemeItem(pXMLChildNode, rThemeItem);

                    if (pThemItemOrig != nullptr)
                    {
                        if (!isnull(description))
                        {
                            int n = GetThemeItemIndex(name, pTheme);
                            if (n >= 0)
                                pTheme->vecStyleClass[n].description = (LPCTSTR) description;
                        }
                        *pThemItemOrig = rThemeItem;
                    }
                    else
                        pTheme->vecStyleClass.push_back({ name, description, rThemeItem });
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

                if (isnull(name))
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
                        pTheme->vecKeywordClass.push_back({ name, keywords });
                    else
                        pKeywordClass->keywords = (LPCTSTR) keywords;
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
                // TODO
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
                // TODO Error if can't find base

                Language* pLanguage = Get(pTheme->vecLanguage, name);
                if (pLanguage == nullptr)
                {
                    pTheme->vecLanguage.push_back(Language(name, pBaseLanguage));
                    pLanguage = &pTheme->vecLanguage.back();
                }
                // TODO else if (pBaseLanguage != nullptr) -- How to apply base now?
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
                    pTheme->mapExt[CString(line, (int) (equals - line)).Trim().MakeLower()] = CString(equals + 1).Trim().MakeLower();
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
                pData->pTheme->mapExt[CString(begin, (int) (equals - begin)).Trim().MakeLower()] = CString(equals + 1, (int) (end - equals - 1)).Trim().MakeLower();

            nSize -= (int) (end - str + 1);
            str = end + 1;
        }
    }
    FreeResource(hGlobal);
    return TRUE;
}

#include "Resource.h"

void LoadTheme(Theme* pTheme)
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

    TCHAR path[_MAX_PATH];
    PathCombine(path, exepath, _T("schemes"));
    LoadSchemeDirectory(path, pTheme, vecBaseLanguage);

#if 0
    GetCurrentDirectory(MAX_PATH, path);
    if (wcscmp(path, exepath) != 0)
    {
        LoadSchemeDirectory(path, pTheme, vecBaseLanguage);

        PathCombine(path, path, _T("schemes"));
        LoadSchemeDirectory(path, pTheme, vecBaseLanguage);
    }
#endif
}
