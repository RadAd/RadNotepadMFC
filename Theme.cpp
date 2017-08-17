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

static inline const Language* GetLanguage(const std::vector<Language>& vecLanguage, LPCTSTR name)
{
    for (const Language& rLanguage : vecLanguage)
    {
        if (rLanguage.name == name)
            return &rLanguage;
    }
    return nullptr;
}

static inline const KeywordClass* GetKeywordClass(const std::vector<KeywordClass>& vecKeywordClass, LPCTSTR name)
{
    for (const KeywordClass& rKeywordClass : vecKeywordClass)
    {
        if (rKeywordClass.name == name)
            return &rKeywordClass;
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
}

const Language* GetLanguageForExt(const Theme* pTheme, LPCTSTR strExt)
{
    std::map<CString, CString>::const_iterator it = pTheme->mapExt.find(CString(strExt).MakeLower());
    if (it == pTheme->mapExt.end())
        return nullptr;
    else
        return GetLanguage(pTheme->vecLanguage, it->second);
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
            const KeywordClass* pKeywordClass = GetKeywordClass(pTheme->vecKeywordClass, sclass);
            if (pKeywordClass != nullptr)
                rCtrl.SetKeyWords(i, pKeywordClass->keywords);
        }

        for (const Style& style : pLanguage->vecStyle)
            ApplyStyle(rCtrl, style, pTheme);
    }
}


#import <MSXML6.dll> exclude("ISequentialStream", "_FILETIME")

inline MSXML2::IXMLDOMDocumentPtr LoadXml(PCWSTR wszFile)
{
    MSXML2::IXMLDOMDocumentPtr pXMLDom(__uuidof(MSXML2::DOMDocument60));
    pXMLDom->Putasync(VARIANT_FALSE);
    pXMLDom->PutvalidateOnParse(VARIANT_FALSE);
    pXMLDom->PutresolveExternals(VARIANT_FALSE);
    pXMLDom->PutpreserveWhiteSpace(VARIANT_TRUE);

    _variant_t varFileName = wszFile;
    VARIANT_BOOL varStatus = pXMLDom->load(varFileName);

    if (varStatus == VARIANT_TRUE)
    {
        return pXMLDom;
    }
    else
    {
        MSXML2::IXMLDOMParseErrorPtr pXMLErr(pXMLDom->GetparseError());
        throw pXMLErr;
    }
}

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

void LoadThemeItem(MSXML2::IXMLDOMNodePtr pXMLChildNode, ThemeItem& rThemeItem)
{
    _bstr_t fore = GetAttribute(pXMLChildNode, _T("fore"));
    _bstr_t back = GetAttribute(pXMLChildNode, _T("back"));
    _bstr_t face = GetAttribute(pXMLChildNode, _T("face"));
    _bstr_t size = GetAttribute(pXMLChildNode, _T("size"));
    _bstr_t bold = GetAttribute(pXMLChildNode, _T("bold"));
    //_bstr_t eolfilled = GetAttribute(pXMLChildNode, _T("eolfilled"));

    if (!isnull(fore))
        rThemeItem.fore = ToColor(fore);
    if (!isnull(back))
        rThemeItem.back = ToColor(back);
    if (!isnull(face))
        wcscpy_s(rThemeItem.font.lfFaceName, face);
    if (!isnull(size))
    {
        HDC hDC = ::GetWindowDC(NULL);
        rThemeItem.font.lfHeight = -MulDiv(_wtoi(size), GetDeviceCaps(hDC, LOGPIXELSY), 72);
        ::ReleaseDC(NULL, hDC);
    }
    if (!isnull(bold) && bold == L"true")
        rThemeItem.font.lfWeight = FW_BOLD;
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

                ASSERT(!isnull(name));
                // TODO Check for no other attributes

                ThemeItem rThemeItem;

                ThemeItem* pThemItemOrig = const_cast<ThemeItem*>(GetThemeItem(name, pTheme));
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
                            pTheme->vecStyleClass[n].description = (wchar_t*) description;
                    }
                    *pThemItemOrig = rThemeItem;
                }
                else
                    pTheme->vecStyleClass.push_back({ name, description, rThemeItem });
            }
        }
    }
}

void ProcessStyles(MSXML2::IXMLDOMNodePtr pXMLNode, std::vector<Style>& vecStyles)
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

                ASSERT(!isnull(name));
                ASSERT(!isnull(key));
                // TODO Check for no other attributes

                ThemeItem rThemeItem;
                LoadThemeItem(pXMLChildNode, rThemeItem);

                vecStyles.push_back({ name, _wtoi(key), sclass, rThemeItem });
            }
            else if (bstrName == L"group")
            {
                _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                _bstr_t description = GetAttribute(pXMLChildNode, _T("description"));
                _bstr_t sclass = GetAttribute(pXMLChildNode, _T("class"));

                ASSERT(!isnull(name));
                //ASSERT(!isnull(description));
                // TODO Check for no other attributes

                // TODO Handle groups properly
                ProcessStyles(pXMLChildNode, vecStyles);
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

                ASSERT(!isnull(name));
                ASSERT(!isnull(keywords));
                // TODO Check for no other attributes

                pTheme->vecKeywordClass.push_back({ name, keywords });
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

                pLanguage->vecKeywords[_wtoi(key)] = { name, sclass };
            }
        }
    }
}

void ProcessLanguage(MSXML2::IXMLDOMNodePtr pXMLNode, Language* pLanguage)
{
    {
        _bstr_t title = GetAttribute(pXMLNode, _T("title"));
        if (!isnull(title))
            pLanguage->title = (wchar_t*) title;
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
                pLanguage->lexer = (wchar_t*) name;
            }
            else if (bstrName == L"use-styles")
            {
                ProcessStyles(pXMLChildNode, pLanguage->vecStyle);
            }
            else if (bstrName == L"use-keywords")
            {
                ProcessKeywords(pXMLChildNode, pLanguage);
            }
            // else if (bstrName == L"property")
            // else if (bstrName == L"comments")
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
        //MSXML2::IXMLDOMNodePtr pNode = pDoc->selectSingleNode("TestDoc");
        MSXML2::IXMLDOMNodeListPtr pXMLChildren(pXMLRoot->GetchildNodes());
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
                    ProcessStyles(pXMLChildNode, pTheme->vecBase);
                }
                else if (bstrName == L"keyword-classes")
                {
                    ProcessKeywordClasses(pXMLChildNode, pTheme);
                }
                else if (bstrName == L"base-language")
                {
                    _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                    // TODO Look for existing item - shouldn't be one
                    vecBaseLanguage.push_back(Language(name));
                    Language& rLanguage = vecBaseLanguage.back();
                    ProcessLanguage(pXMLChildNode, &rLanguage);
                }
                else if (bstrName == L"language")
                {
                    _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                    _bstr_t base = GetAttribute(pXMLChildNode, _T("base"));

                    const Language* pBaseLanguage = isnull(base) ? nullptr : GetLanguage(vecBaseLanguage, base);

                    // TODO Look for existing item - shouldn't be one
                    pTheme->vecLanguage.push_back(Language(name, pBaseLanguage));
                    Language& rLanguage = pTheme->vecLanguage.back();
                    ProcessLanguage(pXMLChildNode, &rLanguage);
                }
            }
        }
    }
    else
    {
        MSXML2::IXMLDOMParseErrorPtr pXMLErr(pDoc->GetparseError());
        _bstr_t bstrErr(pXMLErr->reason);

        CString msg;
        OutputDebugString(_T("Error:\n"));
        msg.Format(_T("Code = 0x%x\n"), pXMLErr->errorCode);  OutputDebugString(msg);
        msg.Format(_T("Source = Line : %ld; Char : %ld\n"), pXMLErr->line, pXMLErr->linepos);  OutputDebugString(msg);
        msg.Format(_T("Error Description = %s\n"), (wchar_t*) bstrErr);  OutputDebugString(msg);
        //throw;
    }
}

void LoadSchemeDirectory(LPCTSTR strDirectory, Theme* pTheme, std::vector<Language>& vecBaseLanguage)
{
    TCHAR full[_MAX_PATH];

    PathCombine(full, strDirectory, _T("schemes\\scheme.master"));
    if (PathFileExists(full))
        LoadScheme(full, pTheme, vecBaseLanguage);

    PathCombine(full, strDirectory, _T("schemes\\*.scheme"));
    WIN32_FIND_DATA fd = {};
    HANDLE hFind;
    if ((hFind = FindFirstFile(full, &fd)) != INVALID_HANDLE_VALUE)
    {
        do
        {
            PathCombine(full, strDirectory, _T("schemes"));
            PathCombine(full, full, fd.cFileName);
            LoadScheme(full, pTheme, vecBaseLanguage);
        } while (FindNextFile(hFind, &fd));
    }

    PathCombine(full, strDirectory, _T("schemes\\extmap.dat"));
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

void LoadTheme(Theme* pTheme)
{
    std::vector<Language> vecBaseLanguage;

    TCHAR path[_MAX_PATH];

    GetModuleFileName(NULL, path, MAX_PATH);
    PathFindFileName(path)[0] = _T('\0');
    LoadSchemeDirectory(path, pTheme, vecBaseLanguage);

    GetCurrentDirectory(MAX_PATH, path);
    LoadSchemeDirectory(path, pTheme, vecBaseLanguage);
}
