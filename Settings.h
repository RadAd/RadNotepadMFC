#pragma once

#include "Theme.h"

enum Encoding
{
    BOM_ANSI,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF8,
};

static LPCTSTR strEncoding[] = { _T("ANSI"), _T("UTF-16"), _T("UTF-16 BE"), _T("UTF-8") };

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
    Encoding DefaultEncoding = BOM_ANSI;

    EditorSettings default;
    EditorSettings user;
    EditorSettings editor;
};
