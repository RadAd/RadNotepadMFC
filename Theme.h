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
extern LPCTSTR THEME_LINENUMBER;
extern LPCTSTR THEME_BRACELIGHT;
extern LPCTSTR THEME_BRACEBAD;
extern LPCTSTR THEME_CONTROLCHAR;
extern LPCTSTR THEME_INDENTGUIDE;
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
    ThemeItem(COLORREF fore = COLOR_NONE, COLORREF back = COLOR_NONE, LOGFONT font = {})
        : fore(fore)
        , back(back)
        , font(font)
    {

    }

    COLORREF fore;
    COLORREF back;
    LOGFONT font;
};

struct Theme
{
    ThemeItem tDefault;
    int nCount = 0;
    struct { CString name; CString description; ThemeItem theme; } vecTheme[100];
};

void InitTheme(Theme* pSettings);
const ThemeItem* GetThemeItem(LPCTSTR strItem, const Theme* pSettings);
void ApplyThemeItem(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme);
void LoadTheme(Theme* pTheme);
