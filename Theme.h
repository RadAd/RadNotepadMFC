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

struct ThemeItem
{
    ThemeItem(COLORREF fore = COLOR_NONE, COLORREF back = COLOR_NONE, LOGFONT font = {})
        : fore(fore)
        , back(back)
        , font(font)
        , eolfilled(FALSE)
        , hotspot(FALSE)
    {
    }

    bool operator==(const ThemeItem& other) const
    {
        return memcmp(this, &other, sizeof(ThemeItem)) == 0;
    }

    bool operator!=(const ThemeItem& other) const
    {
        return memcmp(this, &other, sizeof(ThemeItem)) != 0;
    }

    COLORREF fore;
    COLORREF back;
    LOGFONT font;
    BOOL eolfilled;
    BOOL hotspot;
};

struct Style
{
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
    std::map<CString, CString> mapProperties;
    std::vector<Style> vecStyle;
    std::vector<GroupStyle> vecGroupStyle;
    struct { CString name; CString sclass; } vecKeywords[KEYWORDSET_MAX];

    bool operator==(const Language& other) const
    {
        return name == other.name
            && title == other.title
            && lexer == other.lexer
            && mapProperties == other.mapProperties
            && vecStyle == other.vecStyle
            && vecGroupStyle == other.vecGroupStyle;
            //&& vecKeywords == other.vecKeywords;
    }

    bool operator!=(const Language& other) const
    {
        return !(*this == other);
    }
};

static inline bool CompareLanguageTitle(const Language* pLanguageL, const Language* pLanguageR)
{
    return pLanguageL->title < pLanguageR->title;
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
    std::vector<StyleClass> vecStyleClass;
    std::vector<Style> vecBase;
    std::vector<KeywordClass> vecKeywordClass;
    std::map<CString, CString> mapExt;
    std::map<CString, CString> mapExtFilter;
    std::vector<Language> vecLanguage;

    bool operator==(const Theme& other) const
    {
        return tDefault == other.tDefault
            && vecStyleClass == other.vecStyleClass
            && vecBase == other.vecBase
            //&& vecKeywordClass == other.vecKeywordClass   // Not needed, could be slow
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
void LoadTheme(Theme* pTheme, Theme* pDefaultTheme);
void SaveTheme(const Theme* pTheme, const Theme* pDefaultTheme);
const Language* GetLanguage(const Theme* pTheme, LPCTSTR strName);
const Language* GetLanguageForExt(const Theme* pTheme, LPCTSTR strExt);
const StyleClass* GetStyleClass(const Theme* pTheme, LPCTSTR strName);
