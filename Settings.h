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
    // TODO Also make these settings per Language?
    bool bUseTabs = FALSE;
    int nTabWidth = 4;

    COLORREF cCaretFG = COLOR_NONE;
    int nCaretStyle = CARETSTYLE_LINE;
    int nCaretWidth = 1;

    bool bShowIndentGuides = TRUE;
    // TODO Move indentguide colours here (font not used)
    bool bHighlightMatchingBraces = TRUE;
    // TODO Move bracematch, bracemismatch colours here (font is used)

    bool bAutoIndent = TRUE;

    bool bShowLineNumbers = FALSE;
    // TODO Move linenumbers colours here (font is used)
    // TODO Move controlchars colours here (*only* font is used)
    bool bShowBookmarks = TRUE;
    // TODO bookmark style

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
    int DefaultLineEnding = SC_EOL_CRLF;

    EditorSettings default;
    EditorSettings editor;
};
