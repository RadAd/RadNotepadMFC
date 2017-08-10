#include "stdafx.h"
#include "Theme.h"
#include "Settings.h"

struct
{
    ThemeType nItem;
    ThemeItem theme;
} vThemeDefault[] = {
    { THEME_COMMENT,        COLOR_LT_GREEN,     COLOR_NONE },
    { THEME_NUMBER,         COLOR_LT_CYAN,      COLOR_NONE },
    { THEME_WORD,           COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) },
    { THEME_STRING,         COLOR_LT_MAGENTA,   COLOR_NONE },
    { THEME_IDENTIFIER,     COLOR_BLACK,        COLOR_NONE },
    { THEME_PREPROCESSOR,   COLOR_LT_RED,       COLOR_NONE },
    { THEME_OPERATOR,       COLOR_LT_YELLOW,    COLOR_NONE },
};

const ThemeItem* GetTheme(ThemeType nItem, const Settings* pSettings)
{
    if (nItem == THEME_DEFAULT)
        return &pSettings->tDefault;
    for (int i = 0; i < ARRAYSIZE(vThemeDefault); ++i)
    {
        if (vThemeDefault[i].nItem == nItem)
            return &vThemeDefault[i].theme;
    }
    return nullptr;
};

void ApplyTheme(CScintillaCtrl& rCtrl, int nStyle, const ThemeItem& rTheme)
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
