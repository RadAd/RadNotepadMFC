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

    Theme rTheme;
};
