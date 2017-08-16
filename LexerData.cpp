#include "stdafx.h"
#include "LexerData.h"
#include "Theme.h"

#include <SciLexer.h>

struct Style
{
    int nStyle;
    LPCTSTR strTheme;
};

struct LexerData
{
    int nID;
    PCTSTR strName;
    PCTSTR strExtensions;
    PCTSTR strKeywords[KEYWORDSET_MAX];
    Style* vStyle;
};

Style vStyleDefault = { STYLE_DEFAULT, THEME_DEFAULT };

#if 0
const TCHAR cppKeyWords[] =
    _T("and and_eq asm auto bitand bitor bool break ")
    _T("case catch char class compl const const_cast continue ")
    _T("default delete do double dynamic_cast else enum explicit export extern false float for ")
    _T("friend goto if inline int long mutable namespace new not not_eq ")
    _T("operator or or_eq private protected public ")
    _T("register reinterpret_cast return short signed sizeof static static_cast struct switch ")
    _T("template this throw true try typedef typeid typename union unsigned using ")
    _T("virtual void volatile wchar_t while xor xor_eq ");
#else
const TCHAR cppKeyWords[] =
    _T("alignas alignof and and_eq asm atomic_cancel atomic_commit atomic_noexcept bitand bitor break ")
    _T("case catch class compl concept const constexpr ")
    _T("const_cast continue decltype ")
    _T("default delete do dynamic_cast ")
    _T("else enum explicit export extern for friend goto if import inline ")
    _T("module mutable namespace new not not_eq ")
    _T("operator or or_eq private protected public register reinterpret_cast requries return ")
    _T("sizeof static static_assert static_cast struct switch synchronized template ")
    _T("thread_local throw try typedef typeid typename union ")
    _T("using virtual void volatile whcar_t while xor xor_eq ")
    _T("__finally __exception __try __declspec deprecated dllexport dllimport naked uuid noinline noreturn nothrow selectany thread"); //Microsoft
#endif

const TCHAR cppDatatypes[] =
    _T("ATOM BOOL BOOLEAN BYTE CHAR COLORREF DWORD DWORDLONG DWORD_PTR ")
    _T("DWORD32 DWORD64 FLOAT HACCEL HALF_PTR HANDLE HBITMAP HBRUSH ")
    _T("HCOLORSPACE HCONV HCONVLIST HCURSOR HDC HDDEDATA HDESK HDROP HDWP ")
    _T("HENHMETAFILE HFILE HFONT HGDIOBJ HGLOBAL HHOOK HICON HINSTANCE HKEY ")
    _T("HKL HLOCAL HMENU HMETAFILE HMODULE HMONITOR HPALETTE HPEN HRESULT ")
    _T("HRGN HRSRC HSZ HWINSTA HWND INT INT_PTR INT32 INT64 LANGID LCID LCTYPE ")
    _T("LGRPID LONG LONGLONG LONG_PTR LONG32 LONG64 LPARAM LPBOOL LPBYTE LPCOLORREF ")
    _T("LPCSTR LPCTSTR LPCVOID LPCWSTR LPDWORD LPHANDLE LPINT LPLONG LPSTR LPTSTR ")
    _T("LPVOID LPWORD LPWSTR LRESULT PBOOL PBOOLEAN PBYTE PCHAR PCSTR PCTSTR PCWSTR ")
    _T("PDWORDLONG PDWORD_PTR PDWORD32 PDWORD64 PFLOAT PHALF_PTR PHANDLE PHKEY PINT ")
    _T("PINT_PTR PINT32 PINT64 PLCID PLONG PLONGLONG PLONG_PTR PLONG32 PLONG64 POINTER_32 ")
    _T("POINTER_64 PSHORT PSIZE_T PSSIZE_T PSTR PTBYTE PTCHAR PTSTR PUCHAR PUHALF_PTR ")
    _T("PUINT PUINT_PTR PUINT32 PUINT64 PULONG PULONGLONG PULONG_PTR PULONG32 PULONG64 ")
    _T("PUSHORT PVOID PWCHAR PWORD PWSTR SC_HANDLE SC_LOCK SERVICE_STATUS_HANDLE SHORT ")
    _T("SIZE_T SSIZE_T TBYTE TCHAR UCHAR UHALF_PTR UINT UINT_PTR UINT32 UINT64 ULONG ")
    _T("ULONGLONG ULONG_PTR ULONG32 ULONG64 USHORT USN VOID WCHAR WORD WPARAM WPARAM WPARAM ")
    _T("auto bool char char16_t char32_t short int __int32 __int64 __int8 __int16 long float double __wchar_t ")
    _T("clock_t _complex _dev_t _diskfree_t div_t ldiv_t _exception exception _EXCEPTION_POINTERS ")
    _T("FILE _finddata_t _finddatai64_t _wfinddata_t _wfinddatai64_t __finddata64_t ")
    _T("__wfinddata64_t _FPIEEE_RECORD fpos_t _HEAPINFO _HFILE lconv intptr_t _locale_t ")
    _T("jmp_buf mbstate_t _off_t _onexit_t _PNH ptrdiff_t _purecall_handler ")
    _T("sig_atomic_t size_t _stat __stat64 _stati64 terminate_function ")
    _T("time_t __time64_t _timeb __timeb64 tm uintptr_t _utimbuf ")
    _T("va_list wchar_t wctrans_t wctype_t wint_t signed unsigned ")
    _T("std const_iterator deque iterator list multimap map pair set string stringstream vector wstring wstringstream");

const TCHAR cppValues[] = _T("true false nullptr this TRUE FALSE NULL");

const TCHAR javaDatatypes[] = _T("boolean byte char double float int long short void ")
    _T("Boolean Byte Character Class Double Enum Float Integer Long Number String");
const TCHAR javaKeyWords[] = _T("abstract assert break case catch class const ")
    _T("continue default do else enum extends ")
    _T("final finally for goto if implements import ")
    _T("instanceof interface native new ")
    _T("package private protected public return ")
    _T("static strictfp super switch synchronized this throw throws ")
    _T("transient try volatile while");
const TCHAR javaValues[] = _T("true false null");
//const TCHAR javaPreprocessor[] = _T("import package");

const TCHAR htmlTags[] = _T("a abbr acronym address applet area aside audio b base basefont ")
    _T("bdo big blockquote body br button canvas caption center ")
    _T("cite code col colgroup command datalist dd del details dfn dir div dl dt em ")
    _T("embed fieldset figcaption figure font footer form frame frameset h1 h2 h3 h4 h5 h6 ")
    _T("head header hgroup hr html i iframe img input ins kbd keygen label ")
    _T("legend li link map mark menu meta meter nav noframes noscript ")
    _T("object ol optgroup option output p param pre progress q rp rt ruby s samp ")
    _T("script section select small source span strike strong style sub summary sup ")
    _T("table tbody td textarea tfoot th thead time title tr tt u ul ")
    _T("var video wbr xml xmp xmlns ")
    // Attributes
    _T("abbr accept-charset accept accesskey action align alink ")
    _T("alt archive autocomplete autofocus axis background bgcolor border ")
    _T("cellpadding cellspacing char charoff charset checkbox checked cite ")
    _T("class classid clear codebase codetype color cols colspan ")
    _T("compact content contenteditable contextmenu coords ")
    _T("data datafld dataformatas datapagesize datasrc datetime ")
    _T("declare defer dir disabled draggable dropzone encoding enctype event ")
    _T("face file for form formaction formenctype formmethod formnovalidate formtarget frame frameborder ")
    _T("headers height hidden href hreflang hspace http-equiv ")
    _T("id image ismap label lang language leftmargin link list longdesc ")
    _T("marginwidth marginheight max maxlength media method min multiple ")
    _T("name nohref noresize noshade novalidate nowrap ")
    _T("object onabort onafterprint onbeforeprint onbeforeonload onblur oncanplay oncanplaythrough onchange onclick oncontextmenu ondblclick ondrag ondragend ondragenter ondragleave ondragover ondragstart ondrop ondurationchange onemptied onended onerror onfocus onformchange onforminput onhaschange ")
    _T("oninput oninvalid onkeydown onkeypress onkeyup onload onloadeddata onloadedmetadata onloadstart onmessage onmousedown ")
    _T("onmousemove onmouseover onmouseout onmouseup onmousewheel ")
    _T("onoffline ononline onpagehide onpageshow onpause onplay onplaying onprogress onpopstate onratechange onreadystatechange onredo ")
    _T("onreset onresize onscroll onseeked onseeking onselect onstalled onstorage onsubmit onsuspend ontimeupdate onundo onunload onvolumechange onwaiting ")
    _T("password pattern placeholder profile prompt radio readonly rel required reset rev rows rowspan rules ")
    _T("scheme scope selected shape size span spellcheck src standby start step style submit ")
    _T("summary tabindex target text title topmargin type usemap ")
    _T("valign value valuetype version vlink vspace width ");
const TCHAR sgmlTags[] = _T("DOCTYPE ");

Style vStyleCpp[] = {
    { SCE_C_DEFAULT,                 THEME_DEFAULT },
    { SCE_C_COMMENT,                 THEME_COMMENT },
    { SCE_C_COMMENTLINE,             THEME_COMMENT },
    { SCE_C_COMMENTDOC,              THEME_COMMENT },
    { SCE_C_COMMENTLINEDOC,          THEME_COMMENT },
    { SCE_C_COMMENTDOCKEYWORD,       THEME_COMMENT },   // Keywords 3
    { SCE_C_COMMENTDOCKEYWORDERROR,  THEME_ERROR },
    { SCE_C_PREPROCESSORCOMMENT,     THEME_COMMENT },
    { SCE_C_PREPROCESSORCOMMENTDOC,  THEME_COMMENT },
    { SCE_C_NUMBER,                  THEME_NUMBER },
    { SCE_C_WORD,                    THEME_WORD },      // Keywords 1
    { SCE_C_STRING,                  THEME_STRING },
    { SCE_C_CHARACTER,               THEME_STRING },
    //{ SCE_C_UUID,                  THEME_STRING },
    { SCE_C_PREPROCESSOR,            THEME_PREPROCESSOR },
    { SCE_C_OPERATOR,                THEME_OPERATOR },
    { SCE_C_IDENTIFIER,              THEME_IDENTIFIER },
    //{ SCE_C_STRINGEOL,             THEME_ERROR },
    //{ SCE_C_VERBATIM,              THEME_STRING },
    //{ SCE_C_REGEX,                 THEME_STRING },
    { SCE_C_WORD2,                   THEME_TYPE },      // Keywords 2
    { SCE_C_GLOBALCLASS,             THEME_NUMBER },    // Keywords 4
    //{ SCE_C_STRINGRAW,             THEME_STRING },
    //{ SCE_C_TRIPLEVERBATIM,        THEME_STRING },
    //{ SCE_C_HASHQUOTEDSTRING,      THEME_STRING },
    //{ SCE_C_USERLITERAL,           THEME_STRING },
    //{ SCE_C_TASKMARKER,            THEME_STRING },    // Keywords 5
    //{ SCE_C_ESCAPESEQUENCE,        THEME_STRING },
    { -1 },
};

Style vStyleHtml[] = {
    { SCE_H_DEFAULT,                THEME_DEFAULT },
    { SCE_H_TAG,                    THEME_WORD },
    { SCE_H_TAGUNKNOWN,             THEME_ERROR },
    { SCE_H_ATTRIBUTE,              THEME_IDENTIFIER },
    { SCE_H_ATTRIBUTEUNKNOWN,       THEME_ERROR },
    { SCE_H_NUMBER,                 THEME_NUMBER },
    { SCE_H_DOUBLESTRING,           THEME_STRING },
    { SCE_H_SINGLESTRING,           THEME_STRING },
    //{ SCE_H_OTHER,                  THEME_ERROR },
    { SCE_H_COMMENT,                THEME_COMMENT },
    { SCE_H_ENTITY,                 THEME_NUMBER },      // ie &amp;
    //{ SCE_H_TAGEND,                 THEME_ERROR },    // />
    { SCE_H_XMLSTART,               THEME_PREPROCESSOR },    // <?
    { SCE_H_XMLEND,                 THEME_PREPROCESSOR },    // ?>
    { SCE_H_SCRIPT,                 THEME_ERROR },
    { SCE_H_ASP,                    THEME_ERROR },
    { SCE_H_ASPAT,                  THEME_ERROR },
    { SCE_H_CDATA,                  THEME_ERROR },
    { SCE_H_QUESTION,               THEME_ERROR },
    { SCE_H_VALUE,                  THEME_ERROR },
    { SCE_H_XCCOMMENT,              THEME_COMMENT },
    { SCE_H_SGML_DEFAULT,           THEME_DEFAULT },
    { SCE_H_SGML_COMMAND,           THEME_PREPROCESSOR },
    //{ SCE_H_SGML_1ST_PARAM,         THEME_ERROR },
    { SCE_H_SGML_DOUBLESTRING,      THEME_ERROR },
    { SCE_H_SGML_SIMPLESTRING,      THEME_ERROR },
    { SCE_H_SGML_ERROR,             THEME_ERROR },
    { SCE_H_SGML_SPECIAL,           THEME_ERROR },
    { SCE_H_SGML_ENTITY,            THEME_ERROR },
    { SCE_H_SGML_COMMENT,           THEME_ERROR },
    { SCE_H_SGML_1ST_PARAM_COMMENT, THEME_ERROR },
    { SCE_H_SGML_BLOCK_DEFAULT,     THEME_ERROR },
    { -1 },
};

const LexerData vLexerData[] = {
    { SCLEX_CPP, _T("C/C++"), _T(".cpp;.cc;.c;.h"), { cppKeyWords, cppDatatypes, _T(""), cppValues }, vStyleCpp },
    { SCLEX_CPP, _T("Java"),  _T(".java"),          { javaKeyWords, javaDatatypes, _T(""), javaValues }, vStyleCpp },
    { SCLEX_XML, _T("HTML"),  _T(".html;.xhtml"),   { htmlTags, nullptr, nullptr, nullptr, nullptr, sgmlTags }, vStyleHtml },
    { SCLEX_XML, _T("XML"),   _T(".xml;.rss"),      { }, vStyleHtml },
    { SCLEX_NULL, _T("None") },
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

const PCTSTR GetLexerName(const LexerData* pLexerData)
{
    return pLexerData->strName;
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
    for (int i = 0; i < pTheme->nBaseCount; ++i)
    {
        const auto& item = pTheme->vecBase[i];
        const ThemeItem* pThemeItem = GetThemeItem(item.sclass, pTheme);
        if (pThemeItem != nullptr)
            ApplyThemeItem(rCtrl, item.id2, *pThemeItem);
        ApplyThemeItem(rCtrl, item.id2, item.theme);
    }
    if (pLexerData != nullptr && pLexerData->vStyle != nullptr)
    {
        // TODO Can I do this just once? Is this state shared across ctrls?
        int i = 0;
        while (pLexerData->vStyle[i].nStyle != -1)
        {
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

const LexerData* GetNextData(const LexerData* pLexerData)
{
    if (pLexerData == nullptr)
        return vLexerData;
    else if (pLexerData[1].nID == SCLEX_NULL)
        return nullptr;
    else
        return pLexerData + 1;
}

const LexerData* GetLexerNone()
{
    int i = 0;
    while (vLexerData[i].nID != SCLEX_NULL)
        ++i;
    return &vLexerData[i];
}
