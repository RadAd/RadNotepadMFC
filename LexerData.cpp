#include "stdafx.h"
#include "LexerData.h"

const LexerData* GetLexerData(PCTSTR ext)
{
    int i = 0;
    while (vLexerData[i].strExtensions != nullptr)
    {
        // TODO Better search for extension
        if (ext[0] != _T('\0') && _tcsstr(vLexerData[i].strExtensions, ext) != nullptr)
            break;
        ++i;
    }
    return &vLexerData[i];
}
