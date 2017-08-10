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

enum ThemeType
{
    THEME_BACKGROUND,
    THEME_DEFAULT,
    THEME_COMMENT,
    THEME_NUMBER,
    THEME_WORD,
    THEME_STRING,
    THEME_IDENTIFIER,
    THEME_PREPROCESSOR,
    THEME_OPERATOR,
};

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

struct Settings;
const ThemeItem* GetTheme(ThemeType nItem, const Settings* pSettings);
void ApplyTheme(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme);
