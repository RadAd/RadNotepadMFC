#include "stdafx.h"
#include "LexerData.h"
#include "Theme.h"

#include <SciLexer.h>

struct Style
{
    int nID;
    int nStyle;
    LPCTSTR strTheme;
};

struct LexerData
{
    int nID;
    PCTSTR strExtensions;
    PCTSTR strKeywords[KEYWORDSET_MAX];
    Style* vStyle;
};

Style vStyleDefault = { SCLEX_NULL, STYLE_DEFAULT, THEME_DEFAULT };

const TCHAR cppKeyWords[] =
    _T("and and_eq asm auto bitand bitor bool break ")
    _T("case catch char class compl const const_cast continue ")
    _T("default delete do double dynamic_cast else enum explicit export extern false float for ")
    _T("friend goto if inline int long mutable namespace new not not_eq ")
    _T("operator or or_eq private protected public ")
    _T("register reinterpret_cast return short signed sizeof static static_cast struct switch ")
    _T("template this throw true try typedef typeid typename union unsigned using ")
    _T("virtual void volatile wchar_t while xor xor_eq ");

Style vStyleCpp[] = {
    { SCLEX_CPP, SCE_C_DEFAULT,                 THEME_DEFAULT },
    { SCLEX_CPP, SCE_C_COMMENT,                 THEME_COMMENT },
    { SCLEX_CPP, SCE_C_COMMENTLINE,             THEME_COMMENT },
    { SCLEX_CPP, SCE_C_COMMENTDOC,              THEME_COMMENT },
    { SCLEX_CPP, SCE_C_COMMENTLINEDOC,          THEME_COMMENT },
    { SCLEX_CPP, SCE_C_COMMENTDOCKEYWORD,       THEME_COMMENT },
    { SCLEX_CPP, SCE_C_COMMENTDOCKEYWORDERROR,  THEME_COMMENT },
    { SCLEX_CPP, SCE_C_NUMBER,                  THEME_NUMBER },
    { SCLEX_CPP, SCE_C_WORD,                    THEME_WORD },
    { SCLEX_CPP, SCE_C_STRING,                  THEME_STRING },
    { SCLEX_CPP, SCE_C_IDENTIFIER,              THEME_IDENTIFIER },
    { SCLEX_CPP, SCE_C_PREPROCESSOR,            THEME_PREPROCESSOR },
    { SCLEX_CPP, SCE_C_OPERATOR,                THEME_OPERATOR },
    { SCLEX_NULL },
};

const LexerData vLexerData[] = {
    { SCLEX_CPP, _T(".cpp;.cc;.c;.h"), { cppKeyWords }, vStyleCpp },
    { SCLEX_NULL },
};

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

void ApplyStyle(CScintillaCtrl& rCtrl, const Style& rStyle, const Theme* pTheme)
{
    const ThemeItem* pThemeItem = GetThemeItem(rStyle.strTheme, pTheme);
    if (pThemeItem != nullptr)
        ApplyThemeItem(rCtrl, rStyle.nStyle, *pThemeItem);
}

void ApplyStyle(CScintillaCtrl& rCtrl, const LexerData* pLexerData, const Theme* pTheme)
{
    ApplyStyle(rCtrl, vStyleDefault, pTheme);
    rCtrl.StyleClearAll();
    if (pLexerData != nullptr && pLexerData->vStyle != nullptr)
    {
        // TODO Can I do this just once? Is this state shared across ctrls?
        int i = 0;
        while (pLexerData->vStyle[i].nID != SCLEX_NULL)
        {
            if (pLexerData->vStyle[i].nID == pLexerData->nID)
                ApplyStyle(rCtrl, pLexerData->vStyle[i], pTheme);
            ++i;
        }
    }
}

void Apply(CScintillaCtrl& rCtrl, const LexerData* pLexerData, const Theme* pTheme)
{
    if (pLexerData != nullptr)
    {
        //Setup the Lexer
        rCtrl.SetLexer(pLexerData->nID);
        for (int i = 0; i < KEYWORDSET_MAX; ++i)
        {
            // TODO Can I do this just once? Is this state shared across ctrls?
            rCtrl.SetKeyWords(i, pLexerData->strKeywords[i]);
        }
    }
    else
    {
        rCtrl.SetLexer(SCLEX_NULL);
    }

    //Setup styles
    ApplyStyle(rCtrl, pLexerData, pTheme);
}
