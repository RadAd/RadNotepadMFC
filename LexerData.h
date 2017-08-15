#pragma once

struct LexerData;
struct Theme;

const LexerData* GetLexerData(PCTSTR ext);
const PCTSTR GetLexerName(const LexerData* pLexerData);
void Apply(CScintillaCtrl& rCtrl, const LexerData* pLexerData, const Theme* pTheme);
const LexerData* GetNextData(const LexerData* pLexerData = nullptr);
const LexerData* GetLexerNone();
