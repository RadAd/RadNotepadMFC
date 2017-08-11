#pragma once

#define COLOR_NONE          ((COLORREF) -1)
#define COLOR_WHITE         RGB(0xFF, 0xFF, 0xFF)
#define COLOR_BLACK         RGB(0x00, 0x00, 0x00)
#define COLOR_LT_RED        RGB(0x80, 0x00, 0x00)
#define COLOR_LT_GREEN      RGB(0x00, 0x80, 0x00)
#define COLOR_LT_BLUE       RGB(0x00, 0x00, 0x80)
#define COLOR_LT_CYAN       RGB(0x00, 0x80, 0x80)
#define COLOR_LT_MAGENTA    RGB(0x80, 0x00, 0x80)
#define COLOR_LT_YELLOW     RGB(0x80, 0x80, 0x00)

extern LPCTSTR THEME_DEFAULT;
extern LPCTSTR THEME_COMMENT;
extern LPCTSTR THEME_NUMBER;
extern LPCTSTR THEME_WORD;
extern LPCTSTR THEME_TYPE;
extern LPCTSTR THEME_STRING;
extern LPCTSTR THEME_IDENTIFIER;
extern LPCTSTR THEME_PREPROCESSOR;
extern LPCTSTR THEME_OPERATOR;
extern LPCTSTR THEME_ERROR;

struct ThemeItem
{
    COLORREF fore;
    COLORREF back;
    LOGFONT font;
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

struct Theme
{
    ThemeItem tDefault = { COLOR_BLACK,        COLOR_WHITE, Font(-13, _T("Consolas")) };
    struct { const TCHAR* name; ThemeItem theme; } vecTheme[9] = {
        { THEME_COMMENT,      { COLOR_LT_GREEN,     COLOR_NONE } },
        { THEME_NUMBER,       { COLOR_LT_CYAN,      COLOR_NONE } },
        { THEME_WORD,         { COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) } },
        { THEME_TYPE,         { COLOR_LT_CYAN,      COLOR_NONE } },
        { THEME_STRING,       { COLOR_LT_MAGENTA,   COLOR_NONE } },
        { THEME_IDENTIFIER,   { COLOR_BLACK,        COLOR_NONE } },
        { THEME_PREPROCESSOR, { COLOR_LT_RED,       COLOR_NONE } },
        { THEME_OPERATOR,     { COLOR_LT_YELLOW,    COLOR_NONE } },
        { THEME_ERROR,        { COLOR_WHITE,        COLOR_LT_RED } },
    };
};

const ThemeItem* GetThemeItem(LPCTSTR strItem, const Theme* pSettings);
void ApplyThemeItem(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme);
