#pragma once

#include "Theme.h"

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

    ThemeItem tDefault = { COLOR_BLACK,        COLOR_WHITE, Font(-13, _T("Consolas")) };
    struct { const TCHAR* name; ThemeItem theme; } vecTheme[7] = {
        { _T("Comment"),      { COLOR_LT_GREEN,     COLOR_NONE } },
        { _T("Number"),       { COLOR_LT_CYAN,      COLOR_NONE } },
        { _T("Word"),         { COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) } },
        { _T("String"),       { COLOR_LT_MAGENTA,   COLOR_NONE } },
        { _T("Identifier"),   { COLOR_BLACK,        COLOR_NONE } },
        { _T("Preprocessor"), { COLOR_LT_RED,       COLOR_NONE } },
        { _T("Operator"),     { COLOR_LT_YELLOW,    COLOR_NONE } },
    };
};
