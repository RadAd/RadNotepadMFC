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

struct Theme
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

enum FoldType
{
    MT_ARROW,
    MT_PLUSMINUS,
    MT_CIRCLE,
    MT_BOX
};

struct Settings
{
    bool bEmptyFileOnStartup = TRUE;

    bool bUseTabs = FALSE;
    int nTabWidth = 4;

    bool PropShowLineNumbers = FALSE;
    bool PropShowBookmarks = TRUE;
    bool PropShowFolds = FALSE;

    FoldType nFoldType = MT_BOX;
    COLORREF cFoldFG = COLOR_LT_CYAN;
    COLORREF cFoldBG = COLOR_BLACK;

    Theme tDefault = { COLOR_BLACK,        COLOR_WHITE, Font(-13, _T("Consolas")) };
    struct { const TCHAR* name; Theme theme; } vecTheme[7] = {
        { _T("Comment"), { COLOR_LT_GREEN,     COLOR_NONE } },
        { _T("Number"), { COLOR_LT_CYAN,      COLOR_NONE } },
        { _T("Word"), { COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) } },
        { _T("String"), { COLOR_LT_MAGENTA,   COLOR_NONE } },
        { _T("Identifier"), { COLOR_BLACK,        COLOR_NONE } },
        { _T("Preprocessor"), { COLOR_LT_RED,       COLOR_NONE } },
        { _T("Operator"), { COLOR_LT_YELLOW,    COLOR_NONE } },
    };
};
