#pragma once

#include "Theme.h"

enum class Encoding
{
    BOM_ANSI,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF8,
};

struct Settings
{
    // TODO Move into Theme
    bool bEmptyFileOnStartup = TRUE;
    UINT nMaxMRU = 10;
    Encoding DefaultEncoding = Encoding::BOM_ANSI;
    int DefaultLineEnding = SC_EOL_CRLF;

    Theme default;
    Theme user;
};
