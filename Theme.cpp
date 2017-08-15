#include "stdafx.h"
#include "Theme.h"

extern LPCTSTR THEME_DEFAULT = _T("Default");
extern LPCTSTR THEME_LINENUMBER = _T("Line Number");
extern LPCTSTR THEME_BRACELIGHT = _T("Brace Matching");
extern LPCTSTR THEME_BRACEBAD = _T("Brace Matching (Error)");
extern LPCTSTR THEME_CONTROLCHAR = _T("Control Character");
extern LPCTSTR THEME_INDENTGUIDE = _T("Indent Guide");
extern LPCTSTR THEME_COMMENT = _T("Comment");
extern LPCTSTR THEME_NUMBER = _T("Number");
extern LPCTSTR THEME_WORD = _T("Word");
extern LPCTSTR THEME_TYPE = _T("Type");
extern LPCTSTR THEME_STRING = _T("String");
extern LPCTSTR THEME_IDENTIFIER = _T("Identifier");
extern LPCTSTR THEME_PREPROCESSOR = _T("Preprocessor");
extern LPCTSTR THEME_OPERATOR = _T("Operator");
extern LPCTSTR THEME_ERROR = _T("Error");

const ThemeItem* GetThemeItem(LPCTSTR strItem, const Theme* pTheme)
{
    if (_wcsicmp(strItem, THEME_DEFAULT) == 0)
        return &pTheme->tDefault;
    for (int i = 0; i < ARRAYSIZE(Theme::vecTheme); ++i)
    {
        if (_wcsicmp(strItem, pTheme->vecTheme[i].name) == 0)
            return &pTheme->vecTheme[i].theme;
    }
    return nullptr;
};

void ApplyThemeItem(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme)
{
    if (rTheme.fore != COLOR_NONE)
        rCtrl.StyleSetFore(nStyle, rTheme.fore);
    if (rTheme.back != COLOR_NONE)
        rCtrl.StyleSetBack(nStyle, rTheme.back);
    if (rTheme.font.lfHeight != 0)
    {
        CWindowDC dc(&rCtrl);
        int nLogY = dc.GetDeviceCaps(LOGPIXELSY);
        if (nLogY != 0)
        {
            int pt = MulDiv(72, -rTheme.font.lfHeight, nLogY);
            rCtrl.StyleSetSize(nStyle, pt);
        }
    }
    if (rTheme.font.lfFaceName[0] != _T('\0'))
        rCtrl.StyleSetFont(nStyle, rTheme.font.lfFaceName);
    rCtrl.StyleSetCharacterSet(nStyle, rTheme.font.lfCharSet);
    if (rTheme.font.lfWeight >= FW_BOLD)
        rCtrl.StyleSetBold(nStyle, TRUE);
    if (rTheme.font.lfItalic)
        rCtrl.StyleSetItalic(nStyle, TRUE);
    if (rTheme.font.lfUnderline)
        rCtrl.StyleSetUnderline(nStyle, TRUE);
}
