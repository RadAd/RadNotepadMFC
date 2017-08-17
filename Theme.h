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
};

struct GroupStyle
{
    CString name;
    std::vector<Style> vecStyle;
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
    std::vector<Style> vecStyle;
    std::vector<GroupStyle> vecGroupStyle;
    struct { CString name; CString sclass; } vecKeywords[KEYWORDSET_MAX];
};

struct StyleClass
{
    CString name;
    CString description;
    ThemeItem theme;
};

struct KeywordClass
{
    CString name;
    CString keywords;
};

struct Theme
{
    ThemeItem tDefault;
    std::vector<StyleClass> vecStyleClass;
    std::vector<Style> vecBase;
    std::vector<KeywordClass> vecKeywordClass;
    std::map<CString, CString> mapExt;
    std::vector<Language> vecLanguage;
};

void InitTheme(Theme* pSettings);
void Apply(CScintillaCtrl& rCtrl, const Language* pLanguage, const Theme* pTheme);
void LoadTheme(Theme* pTheme);
const Language* GetLanguage(const Theme* pTheme, LPCTSTR strName);
const Language* GetLanguageForExt(const Theme* pTheme, LPCTSTR strExt);
