#pragma once

#include "Theme.h"

enum FoldType
{
    MT_ARROW,
    MT_PLUSMINUS,
    MT_CIRCLE,
    MT_BOX
};

struct EditorSettings
{
    bool bUseTabs = FALSE;
    int nTabWidth = 4;

    bool bShowIndentGuides = TRUE;
    bool bHighlightMatchingBraces = TRUE;
    bool bAutoIndent = TRUE;

    bool bShowLineNumbers = FALSE;
    bool bShowBookmarks = TRUE;
    bool bShowFolds = FALSE;

    FoldType nFoldType = MT_BOX;
    COLORREF cFoldFG = COLOR_LT_CYAN;
    COLORREF cFoldBG = COLOR_BLACK;

    Theme rTheme;
};

struct Settings
{
    bool bEmptyFileOnStartup = TRUE;
    UINT nMaxMRU = 10;

    EditorSettings editor;
};
