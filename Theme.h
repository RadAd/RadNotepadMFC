#pragma once

#include <vector>
#include <map>

#define COLOR_NONE          ((COLORREF) -1)
#define COLOR_WHITE         RGB(0xFF, 0xFF, 0xFF)
#define COLOR_BLACK         RGB(0x00, 0x00, 0x00)
#define COLOR_LT_RED        RGB(0x80, 0x00, 0x00)
#define COLOR_LT_GREEN      RGB(0x00, 0x80, 0x00)
#define COLOR_LT_BLUE       RGB(0x00, 0x00, 0x80)
#define COLOR_LT_CYAN       RGB(0x00, 0x80, 0x80)
#define COLOR_LT_MAGENTA    RGB(0x80, 0x00, 0x80)
#define COLOR_LT_YELLOW     RGB(0x80, 0x80, 0x00)

enum Bool3
{
    B3_UNDEFINED = -1,
    B3_FALSE,
    B3_TRUE,
};

struct ThemeItem
{
    ThemeItem(COLORREF fore = COLOR_NONE, COLORREF back = COLOR_NONE, LOGFONT font = {})
        : fore(fore)
        , back(back)
        , font(font)
        , eolfilled(B3_UNDEFINED)
        , hotspot(B3_UNDEFINED)
    {
    }

    COLORREF fore;
    COLORREF back;
    LOGFONT font; // TODO Maybe just capture the bits Im interested in
    Bool3 eolfilled;
    Bool3 hotspot;
    // TODO add flags to say which parts are applicable

    bool operator==(const ThemeItem& other) const
    {
        return fore == other.fore
            && back == other.back
            && memcmp(&font, &other.font, sizeof(LOGFONT)) == 0
            && eolfilled == other.eolfilled
            && hotspot == other.hotspot;
    }

    bool operator!=(const ThemeItem& other) const
    {
        return !(*this == other);
    }
};

struct Style
{
    Style(LPCTSTR name, int id, LPCTSTR sclass = nullptr, const ThemeItem& theme = ThemeItem())
        : name(name)
        , id(id)
        , sclass(sclass)
        , theme(theme)
    {
    }

    CString name;
    int id;
    CString sclass;
    ThemeItem theme;

    bool operator==(const Style& other) const
    {
        return name == other.name
            && id == other.id
            && sclass == other.sclass
            && theme == other.theme;
    }

    bool operator!=(const Style& other) const
    {
        return !(*this == other);
    }
};

struct GroupStyle
{
    CString name;
    std::vector<Style> vecStyle;

    bool operator==(const GroupStyle& other) const
    {
        return name == other.name
            && vecStyle == other.vecStyle;
    }

    bool operator!=(const GroupStyle& other) const
    {
        return !(*this == other);
    }
};

struct ThemeEditor
{
    int nCaretStyle = -1;
    int nCaretWidth = 0;
    COLORREF cCaretFG = COLOR_NONE;

    Bool3 bUseTabs = B3_UNDEFINED;
    int nTabWidth = 0;
    int nIndentGuideType = -1;
    Bool3 bHighlightMatchingBraces = B3_UNDEFINED;
    Bool3 bAutoIndent = B3_UNDEFINED;

    // Add view whitespace, eol, word wrap

    bool operator==(const ThemeEditor& other) const
    {
        return nCaretStyle == other.nCaretStyle
            && nCaretWidth == other.nCaretWidth
            && cCaretFG == other.cCaretFG
            && bUseTabs == other.bUseTabs
            && nTabWidth == other.nTabWidth
            && nIndentGuideType == other.nIndentGuideType
            && bHighlightMatchingBraces == other.bHighlightMatchingBraces
            && bAutoIndent == other.bAutoIndent;
    }

    bool operator!=(const ThemeEditor& other) const
    {
        return !(*this == other);
    }
};

struct Margin
{
    Margin(LPCTSTR name, int id)
        : name(name)
        , id(id)
    {
    }

    CString name;
    int id;
    Bool3 show = B3_UNDEFINED;
    int width = 0;
    CString width_text;
    Bool3 sensitive = B3_UNDEFINED;
    int type = -1;
    int mask = 0;

    bool operator==(const Margin& other) const
    {
        return name == other.name
            && id == other.id
            && show == other.show
            && width == other.width
            && width_text == other.width_text
            && sensitive == other.sensitive
            && type == other.type
            && mask == other.mask;
    }

    bool operator!=(const Margin& other) const
    {
        return !(*this == other);
    }
};

struct Marker
{
    Marker(LPCTSTR name, int id)
        : name(name)
        , id(id)
    {
    }

    CString name;
    int id;
    int type = -1;
    // TODO Use a class and themeitem
    COLORREF fore = COLOR_NONE;
    COLORREF back = COLOR_NONE;

    bool operator==(const Marker& other) const
    {
        return name == other.name
            && id == other.id
            && type == other.type
            && fore == other.fore
            && back == other.back;
    }

    bool operator!=(const Marker& other) const
    {
        return !(*this == other);
    }
};

struct Language
{
    Language(LPCSTR name, const Language* pBaseLanguage = nullptr)
    {
        if (pBaseLanguage != nullptr)
            *this = *pBaseLanguage;
        this->name = name;
        this->name.MakeLower();
    }
    CString name;
    CString title;
    CString lexer;
    BOOL internal = FALSE;
    CString strWordChars;
    ThemeEditor editor;
    std::map<CString, CString> mapProperties;
    std::vector<Style> vecStyle;
    std::vector<GroupStyle> vecGroupStyle;
    std::vector<Style> vecBase;
    std::vector<Margin> vecMargin;
    std::vector<Marker> vecMarker;
    struct { CString name; CString sclass; } vecKeywords[KEYWORDSET_MAX];

    bool operator==(const Language& other) const
    {
        return name == other.name
            && title == other.title
            && lexer == other.lexer
            && internal == other.internal
            && strWordChars == other.strWordChars
            && editor == other.editor
            && mapProperties == other.mapProperties
            && vecStyle == other.vecStyle
            && vecGroupStyle == other.vecGroupStyle
            && vecBase == other.vecBase
            && vecMargin == other.vecMargin
            && vecMarker == other.vecMarker;
            //&& vecKeywords == other.vecKeywords;
    }

    bool operator!=(const Language& other) const
    {
        return !(*this == other);
    }
};

static inline bool CompareLanguageTitle(const Language* pLanguageL, const Language* pLanguageR)
{
    if (pLanguageL->internal == pLanguageR->internal)
        return pLanguageL->title < pLanguageR->title;
    else
        return !pLanguageL->internal;
}

struct StyleClass
{
    CString name;
    CString description;
    ThemeItem theme;

    bool operator==(const StyleClass& other) const
    {
        return name == other.name
            && description == other.description
            && theme == other.theme;
    }

    bool operator!=(const StyleClass& other) const
    {
        return !(*this == other);
    }
};

struct KeywordClass
{
    CString name;
    CString keywords;

    bool operator==(const KeywordClass& other) const
    {
        return name == other.name
            && keywords == other.keywords;
    }

    bool operator!=(const KeywordClass& other) const
    {
        return !(*this == other);
    }
};

struct Theme
{
    ThemeItem tDefault;
    ThemeEditor editor;
    std::vector<StyleClass> vecStyleClass;
    std::vector<Style> vecBase;
    std::vector<KeywordClass> vecKeywordClass;
    std::vector<Margin> vecMargin;
    std::vector<Marker> vecMarker;
    std::map<CString, CString> mapExt;
    std::map<CString, CString> mapExtFilter;
    std::vector<Language> vecLanguage;

    bool operator==(const Theme& other) const
    {
        return tDefault == other.tDefault
            && editor == other.editor
            && vecStyleClass == other.vecStyleClass
            && vecBase == other.vecBase
            //&& vecKeywordClass == other.vecKeywordClass   // Not needed, could be slow
            && vecMargin == other.vecMargin
            && vecMarker == other.vecMarker
            && mapExt == other.mapExt
            //&& mapExtFilter == other.mapExtFilter // Not needed as its just a cache of mapExt
            && vecLanguage == other.vecLanguage;
    }

    bool operator!=(const Theme& other) const
    {
        return !(*this == other);
    }
};

void Apply(CScintillaCtrl& rCtrl, const Language* pLanguage, const Theme* pTheme);
void ApplyMargin(CScintillaCtrl& rCtrl, const Margin& margin, const Margin* pBaseMargin);
void LoadTheme(Theme* pTheme, Theme* pDefaultTheme);
void SaveTheme(const Theme* pTheme, const Theme* pDefaultTheme);
const Language* GetLanguage(const Theme* pTheme, LPCTSTR strName);
const Language* GetLanguageForExt(const Theme* pTheme, LPCTSTR strExt);
const StyleClass* GetStyleClass(const Theme* pTheme, LPCTSTR strName);
int GetMarginWidth(CScintillaCtrl& rCtrl, const Margin& margin, const Margin* pBaseMargin);

#define pn(x, y) ((x) == nullptr ? nullptr : &(x)->y)

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

template<class T>
static T Merge(const T& a, const T* b, const T& n, const T& d)
{
    if (a != n)
        return a;
    else if (b != nullptr && *b != n)
        return *b;
    else
        return d;
}
