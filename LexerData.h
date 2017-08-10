#pragma once

struct LexerData;
struct Theme;

const LexerData* GetLexerData(PCTSTR ext);
void Apply(CScintillaCtrl& rCtrl, const LexerData* pLexerData, const Theme* pTheme);
