#include "stdafx.h"
#include "Theme.h"

extern LPCTSTR THEME_DEFAULT = _T("default");
extern LPCTSTR THEME_COMMENT = _T("comment");
extern LPCTSTR THEME_NUMBER = _T("number");
extern LPCTSTR THEME_WORD = _T("keyword");
extern LPCTSTR THEME_TYPE = _T("keyword2");
extern LPCTSTR THEME_STRING = _T("string");
extern LPCTSTR THEME_IDENTIFIER = _T("Identifier");
extern LPCTSTR THEME_PREPROCESSOR = _T("preprocessor");
extern LPCTSTR THEME_OPERATOR = _T("operator");
extern LPCTSTR THEME_ERROR = _T("error");

struct LanguageSet
{
    int nCount = 0;
    Language vec[100];
};

inline LOGFONT Font(int size, LPCWSTR face, bool bold = false)
{
    LOGFONT lf = {};
    lf.lfHeight = size;
    if (face != nullptr)
        wcscpy_s(lf.lfFaceName, face);
    lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
    return lf;
}

const Language* GetLanguage(const LanguageSet& rLanguageSet, LPCSTR name)
{
    for (int i = 0; i < rLanguageSet.nCount; ++i)
    {
        if (rLanguageSet.vec[i].name == name)
            return &rLanguageSet.vec[i];
    }
    return nullptr;
}

int GetThemeItemIndex(LPCTSTR strItem, const Theme* pTheme)
{
    if (_wcsicmp(strItem, THEME_DEFAULT) == 0)
        return -2;
    for (int i = 0; i < pTheme->nThemeCount; ++i)
    {
        if (_wcsicmp(strItem, pTheme->vecTheme[i].name) == 0)
            return i;
    }
    return -1;
};

const ThemeItem* GetThemeItem(LPCTSTR strItem, const Theme* pTheme)
{
    if (_wcsicmp(strItem, THEME_DEFAULT) == 0)
        return &pTheme->tDefault;
    for (int i = 0; i < pTheme->nThemeCount; ++i)
    {
        if (_wcsicmp(strItem, pTheme->vecTheme[i].name) == 0)
            return &pTheme->vecTheme[i].theme;
    }
    return nullptr;
};

void ApplyThemeItem(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme)
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

void InitTheme(Theme* pTheme)
{
    pTheme->tDefault = { COLOR_BLACK, COLOR_WHITE, Font(-13, _T("Consolas")) };
    {
        int& i = pTheme->nThemeCount;
        pTheme->vecTheme[i++] = { THEME_COMMENT,      _T("#Comment"),            { COLOR_LT_GREEN,     COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_NUMBER,       _T("#Number"),             { COLOR_LT_CYAN,      COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_WORD,         _T("#Word"),               { COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) } };
        pTheme->vecTheme[i++] = { THEME_TYPE,         _T("#Type"),               { COLOR_LT_CYAN,      COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_STRING,       _T("#String"),             { COLOR_LT_MAGENTA,   COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_IDENTIFIER,   _T("#Identifier"),         { COLOR_BLACK,        COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_PREPROCESSOR, _T("#Preprocessor"),       { COLOR_LT_RED,       COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_OPERATOR,     _T("#Operator"),           { COLOR_LT_YELLOW,    COLOR_NONE } };
        pTheme->vecTheme[i++] = { THEME_ERROR,        _T("#Error"),              { COLOR_WHITE,        COLOR_LT_RED } };
    }
    {
        int& i = pTheme->nBaseCount;
        pTheme->vecBase[i++] = { _T("Indent Guide"), STYLE_INDENTGUIDE, _T("indentguide"), { COLOR_NONE,     COLOR_NONE } };  // TODO Should I add to scheme.master
        // TODO THEME_CONTROLCHAR Looks like only the font is used ?
        // TODO STYLE_INDENTGUIDE Looks like the font isn't used ?
        // TODO
        // STYLE_CALLTIP
        // STYLE_FOLDDISPLAYTEXT
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
                            pTheme->vecTheme[n].description = (wchar_t*) description;
                    }
                    *pThemItemOrig = rThemeItem;
                }
                else
                    pTheme->vecTheme[pTheme->nThemeCount++] = { name, description, rThemeItem };
            }
        }
    }
}

void ProcessStyles(MSXML2::IXMLDOMNodePtr pXMLNode, int& nCount, StyleNew* pVec)
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

                pVec[nCount++];
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

                pTheme->vecKeywords[pTheme->nKeywordCount++] = { name, keywords };
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

void ProcessLanguage(MSXML2::IXMLDOMNodePtr pXMLNode, Language* pLanguage, const LanguageSet& rBaseLanguage)
{
    {
        _bstr_t base = GetAttribute(pXMLNode, _T("base"));
        if (!isnull(base))
        {
            const Language* pBaseLanguage = GetLanguage(rBaseLanguage, base);
            *pLanguage = *pBaseLanguage;
        }

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
                ProcessStyles(pXMLChildNode, pLanguage->nStyleCount, pLanguage->vecStyle);
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

void LoadScheme(LPCTSTR pFilename, Theme* pTheme, LanguageSet& rBaseLanguage)
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
                    ProcessStyles(pXMLChildNode, pTheme->nBaseCount, pTheme->vecBase);
                }
                else if (bstrName == L"keyword-classes")
                {
                    ProcessKeywordClasses(pXMLChildNode, pTheme);
                }
                else if (bstrName == L"base-language")
                {
                    _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                    // TODO Look for existing item - shouldn't be one
                    Language* pLanguage = &rBaseLanguage.vec[rBaseLanguage.nCount++];
                    pLanguage->name = (wchar_t*) name;
                    ProcessLanguage(pXMLChildNode, pLanguage, rBaseLanguage);
                }
                else if (bstrName == L"language")
                {
                    _bstr_t name = GetAttribute(pXMLChildNode, _T("name"));
                    // TODO Look for existing item - shouldn't be one
                    Language* pLanguage = &pTheme->vecLanguages[pTheme->nLanguageCount++];
                    pLanguage->name = (wchar_t*) name;
                    ProcessLanguage(pXMLChildNode, pLanguage, rBaseLanguage);
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


void LoadSchemeDirectory(LPCTSTR strDirectory, Theme* pTheme, LanguageSet& rBaseLanguage)
{
    TCHAR full[_MAX_PATH];

    PathCombine(full, strDirectory, _T("schemes\\scheme.master"));
    if (PathFileExists(full))
        LoadScheme(full, pTheme, rBaseLanguage);

    PathCombine(full, strDirectory, _T("schemes\\cpp.scheme"));
    if (PathFileExists(full))
        LoadScheme(full, pTheme, rBaseLanguage);
}

void LoadTheme(Theme* pTheme)
{
    LanguageSet* pBaseLanguage = new LanguageSet;

    TCHAR path[_MAX_PATH];
    TCHAR full[_MAX_PATH];

    GetModuleFileName(NULL, path, MAX_PATH);
    PathFindFileName(path)[0] = _T('\0');
    LoadSchemeDirectory(path, pTheme, *pBaseLanguage);

    GetCurrentDirectory(MAX_PATH, path);
    LoadSchemeDirectory(path, pTheme, *pBaseLanguage);

    delete pBaseLanguage;
}
