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

struct Settings
{
    bool bEmptyFileOnStartup = TRUE;
    UINT nMaxMRU = 10;
    Encoding DefaultEncoding = BOM_ANSI;
    int DefaultLineEnding = SC_EOL_CRLF;

    Theme default;
    Theme user;
};
