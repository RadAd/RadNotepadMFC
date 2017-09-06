#pragma once

#include "Theme.h"

enum Encoding
{
    BOM_ANSI,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF8,
};

struct Settings
{
    bool bEmptyFileOnStartup = TRUE;
    UINT nMaxMRU = 10;
    Encoding DefaultEncoding = BOM_ANSI;
    int DefaultLineEnding = SC_EOL_CRLF;

    Theme default;
    Theme user;
};
