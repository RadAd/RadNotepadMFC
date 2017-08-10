#pragma once

#include <SciLexer.h>

struct LexerData
{
    int nID;
    PCTSTR strExtensions;
    PCTSTR strKeywords[KEYWORDSET_MAX];
};

const TCHAR cppKeyWords[] =
_T("and and_eq asm auto bitand bitor bool break ")
_T("case catch char class compl const const_cast continue ")
_T("default delete do double dynamic_cast else enum explicit export extern false float for ")
_T("friend goto if inline int long mutable namespace new not not_eq ")
_T("operator or or_eq private protected public ")
_T("register reinterpret_cast return short signed sizeof static static_cast struct switch ")
_T("template this throw true try typedef typeid typename union unsigned using ")
_T("virtual void volatile wchar_t while xor xor_eq ");

const LexerData vLexerData[] = {
    { SCLEX_CPP, _T(".cpp;.cc;.c;.h"), { cppKeyWords } },
    { SCLEX_NULL },
};

const LexerData* GetLexerData(PCTSTR ext);
