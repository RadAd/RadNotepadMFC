
// RadNotepadView.cpp : implementation of the CRadNotepadView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "RadNotepad.h"
#endif

#include "RadNotepadDoc.h"
#include "RadNotepadView.h"
#include <SciLexer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define RAD_MARKER_BOOKMARK 2

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

enum ThemeItem
{
    THEME_BACKGROUND,
    THEME_DEFAULT,
    THEME_COMMENT,
    THEME_NUMBER,
    THEME_WORD,
    THEME_STRING,
    THEME_IDENTIFIER,
    THEME_PREPROCESSOR,
    THEME_OPERATOR,
};

struct
{
    ThemeItem nItem;
    Theme theme;
} vThemeDefault[] = {
    { THEME_COMMENT,        COLOR_LT_GREEN,     COLOR_NONE },
    { THEME_NUMBER,         COLOR_LT_CYAN,      COLOR_NONE },
    { THEME_WORD,           COLOR_LT_BLUE,      COLOR_NONE, Font(0, nullptr, true) },
    { THEME_STRING,         COLOR_LT_MAGENTA,   COLOR_NONE },
    { THEME_IDENTIFIER,     COLOR_BLACK,        COLOR_NONE },
    { THEME_PREPROCESSOR,   COLOR_LT_RED,       COLOR_NONE },
    { THEME_OPERATOR,       COLOR_LT_YELLOW,    COLOR_NONE },
};

const Theme* GetTheme(ThemeItem nItem, const Settings* pSettings)
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

void ApplyTheme(CScintillaCtrl& rCtrl, int nStyle, const Theme& rTheme)
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

struct Style
{
    int nID;
    int nStyle;
    ThemeItem nTheme;
};

Style vStyleDefault = { SCLEX_NULL, STYLE_DEFAULT, THEME_DEFAULT };

Style vStyle[] = {
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

void ApplyStyle(CScintillaCtrl& rCtrl, const Style& rStyle, const Settings* pSettings)
{
    const Theme* pTheme = GetTheme(rStyle.nTheme, pSettings);
    if (pTheme != nullptr)
        ApplyTheme(rCtrl, rStyle.nStyle, *pTheme);
}

enum Margin
{
    MARGIN_LINENUMBERS,
    MARGIN_SYMBOLS,
    MARGIN_FOLDS,
};

enum Margin GetMarginFromMenu(UINT nId)
{
    switch (nId)
    {
    default: ASSERT(FALSE);
    case ID_VIEW_LINENUMBERS:   return MARGIN_LINENUMBERS;
    case ID_VIEW_BOOKMARKS:     return MARGIN_SYMBOLS;
    case ID_VIEW_FOLDS:         return MARGIN_FOLDS;
    }
}

int GetWidth(CScintillaCtrl& rCtrl, Margin m)
{
    switch (m)
    {
    case MARGIN_LINENUMBERS: return rCtrl.TextWidth(STYLE_LINENUMBER, "99999");
    default: return 16;
    }
}

// CRadNotepadView

IMPLEMENT_DYNCREATE(CRadNotepadView, CScintillaView)

BEGIN_MESSAGE_MAP(CRadNotepadView, CScintillaView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CRadNotepadView::OnFilePrintPreview)
    ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, &CRadNotepadView::OnUpdateLine)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, &CRadNotepadView::OnUpdateInsert)
    ON_COMMAND_RANGE(ID_VIEW_LINENUMBERS, ID_VIEW_FOLDS, &CRadNotepadView::OnViewMarker)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_LINENUMBERS, ID_VIEW_FOLDS, &CRadNotepadView::OnUpdateViewMarker)
    ON_COMMAND(ID_VIEW_WHITESPACE, &CRadNotepadView::OnViewWhitespace)
    ON_UPDATE_COMMAND_UI(ID_VIEW_WHITESPACE, &CRadNotepadView::OnUpdateViewWhitespace)
    ON_COMMAND(ID_VIEW_ENDOFLINE, &CRadNotepadView::OnViewEndOfLine)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ENDOFLINE, &CRadNotepadView::OnUpdateViewEndOfLine)
    ON_COMMAND(ID_VIEW_WORDWRAP, &CRadNotepadView::OnViewWordWrap)
    ON_UPDATE_COMMAND_UI(ID_VIEW_WORDWRAP, &CRadNotepadView::OnUpdateViewWordWrap)
    ON_COMMAND(ID_VIEW_USETABS, &CRadNotepadView::OnViewUseTabs)
    ON_UPDATE_COMMAND_UI(ID_VIEW_USETABS, &CRadNotepadView::OnUpdateViewUseTabs)
    ON_COMMAND(ID_EDIT_TOGGLEBOOKMARK, &CRadNotepadView::OnEditToggleBookmark)
    ON_COMMAND(ID_EDIT_PREVIOUSBOOKMARK, &CRadNotepadView::OnEditPreviousBookmark)
    ON_COMMAND(ID_EDIT_NEXTBOOKMARK, &CRadNotepadView::OnEditNextBookmark)
    ON_COMMAND(ID_LINEENDINGS_WINDOWS, &CRadNotepadView::OnLineEndingsWindows)
    ON_COMMAND(ID_LINEENDINGS_UNIX, &CRadNotepadView::OnLineEndingsUnix)
    ON_COMMAND(ID_LINEENDINGS_MAC, &CRadNotepadView::OnLineEndingsMac)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LINEENDINGS_WINDOWS, ID_LINEENDINGS_UNIX, &CRadNotepadView::OnUpdateLineEndings)
END_MESSAGE_MAP()

// CRadNotepadView construction/destruction

CRadNotepadView::CRadNotepadView()
{
	// TODO: add construction code here

}

CRadNotepadView::~CRadNotepadView()
{
}

BOOL CRadNotepadView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CScintillaView::PreCreateWindow(cs);
}

void CRadNotepadView::OnModified(_Inout_ SCNotification* pSCNotification)
{
    CScintillaView::OnModified(pSCNotification);
    if (pSCNotification->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT))
    {
        CRadNotepadDoc* pDoc = GetDocument();
        pDoc->SyncModified();
    }
}

// CRadNotepadView drawing

void CRadNotepadView::OnDraw(CDC* /*pDC*/)
{
	CRadNotepadDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: add draw code for native data here
}


// CRadNotepadView printing


void CRadNotepadView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

void CRadNotepadView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CRadNotepadView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

void CRadNotepadView::OnUpdateLine(CCmdUI* pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    Sci_Position nPos = rCtrl.GetCurrentPos();
    int nLine = rCtrl.LineFromPosition(nPos);
    int nColumn = rCtrl.GetColumn(nPos);

    CString sLine;
    sLine.Format(ID_INDICATOR_LINE, nLine, nColumn, nPos);
    pCmdUI->SetText(sLine);
    pCmdUI->Enable();
}

void CRadNotepadView::OnUpdateInsert(CCmdUI* pCmdUI)
{
    CString sText;
    sText.LoadString(ID_INDICATOR_OVR);
    pCmdUI->SetText(sText);
    pCmdUI->Enable(GetCtrl().GetOvertype());
}


// CRadNotepadView diagnostics

#ifdef _DEBUG
void CRadNotepadView::AssertValid() const
{
    CScintillaView::AssertValid();
}

void CRadNotepadView::Dump(CDumpContext& dc) const
{
    CScintillaView::Dump(dc);
}

CRadNotepadDoc* CRadNotepadView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CRadNotepadDoc)));
	return (CRadNotepadDoc*)m_pDocument;
}
#endif //_DEBUG

void CRadNotepadView::SetLineEndingsMode(int mode)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    if (rCtrl.GetEOLMode() != mode)
    {
        int r = AfxMessageBox(IDS_CONVERTLINEENDINGS, MB_YESNOCANCEL);
        if (r == IDYES)
            rCtrl.ConvertEOLs(mode);
        if (r != IDCANCEL)
            rCtrl.SetEOLMode(mode);
    }
}

// CRadNotepadView message handlers


void CRadNotepadView::OnInitialUpdate()
{
    CScintillaView::OnInitialUpdate();

    CRadNotepadDoc* pDoc = GetDocument();
    CString strFileName = pDoc->GetPathName();
    PCTSTR strExt = PathFindExtension(strFileName);
    const LexerData* pLexerData = GetLexerData(strExt);

    // TODO Copy some settings from other ctrl (ie split view, new window)

    CScintillaCtrl& rCtrl = GetCtrl();

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

    const Settings& settings = theApp.m_Settings;

    //Setup styles
    ApplyStyle(rCtrl, vStyleDefault, &settings);
    rCtrl.StyleClearAll();
    if (pLexerData != nullptr)
    {
        // TODO Can I do this just once? Is this state shared across ctrls?
        int i = 0;
        while (vStyle[i].nID != SCLEX_NULL)
        {
            if (vStyle[i].nID == pLexerData->nID)
                ApplyStyle(rCtrl, vStyle[i], &settings);
            ++i;
        }
    }

    //Setup folding
    rCtrl.SetMarginWidthN(MARGIN_FOLDS, settings.PropShowFolds ? GetWidth(rCtrl, MARGIN_FOLDS) : 0);
    rCtrl.SetMarginSensitiveN(MARGIN_FOLDS, TRUE);
    rCtrl.SetMarginTypeN(MARGIN_FOLDS, SC_MARGIN_SYMBOL);
    rCtrl.SetMarginMaskN(MARGIN_FOLDS, SC_MASK_FOLDERS);
    rCtrl.SetProperty(_T("fold"), _T("1"));

    rCtrl.SetMarginWidthN(MARGIN_LINENUMBERS, settings.PropShowLineNumbers ? GetWidth(rCtrl, MARGIN_LINENUMBERS) : 0);
    rCtrl.SetMarginWidthN(MARGIN_SYMBOLS, settings.PropShowBookmarks ? GetWidth(rCtrl, MARGIN_SYMBOLS) : 0);

    //Setup markers
    int MTMarker[] = {
        SC_MARKNUM_FOLDEROPEN,
        SC_MARKNUM_FOLDER,
        SC_MARKNUM_FOLDERSUB,
        SC_MARKNUM_FOLDERTAIL,
        SC_MARKNUM_FOLDEREND,
        SC_MARKNUM_FOLDEROPENMID,
        SC_MARKNUM_FOLDERMIDTAIL,
    };
    int MT[][7] = {
        { SC_MARK_ARROWDOWN,   SC_MARK_ARROW,      SC_MARK_EMPTY, SC_MARK_EMPTY,        SC_MARK_EMPTY,               SC_MARK_EMPTY,                SC_MARK_EMPTY },
        { SC_MARK_MINUS,       SC_MARK_PLUS,       SC_MARK_EMPTY, SC_MARK_EMPTY,        SC_MARK_EMPTY,               SC_MARK_EMPTY,                SC_MARK_EMPTY },
        { SC_MARK_CIRCLEMINUS, SC_MARK_CIRCLEPLUS, SC_MARK_VLINE, SC_MARK_LCORNERCURVE, SC_MARK_CIRCLEPLUSCONNECTED, SC_MARK_CIRCLEMINUSCONNECTED, SC_MARK_TCORNERCURVE },
        { SC_MARK_BOXMINUS,    SC_MARK_BOXPLUS,    SC_MARK_VLINE, SC_MARK_LCORNER,      SC_MARK_BOXPLUSCONNECTED,    SC_MARK_BOXMINUSCONNECTED,    SC_MARK_TCORNER },
    };
    for (int i = 0; i < ARRAYSIZE(MTMarker); ++i)
        DefineMarker(MTMarker[i], MT[settings.nFoldType][i], settings.cFoldFG, settings.cFoldBG);

    DefineMarker(RAD_MARKER_BOOKMARK, SC_MARK_BOOKMARK, settings.cFoldFG, settings.cFoldBG);

    rCtrl.SetUseTabs(settings.bUseTabs);
    rCtrl.SetTabWidth(settings.nTabWidth);

    // TODO Detect line endings and set current mode

#if 0
    //Setup auto completion
    rCtrl.AutoCSetSeparator(10); //Use a separator of line feed

                                 //Setup call tips
    rCtrl.SetMouseDwellTime(1000);

    //Enable Multiple selection
    rCtrl.SetMultipleSelection(TRUE);
#endif
}

void CRadNotepadView::DefineMarker(int marker, int markerType, COLORREF fore, COLORREF back)
{
    CScintillaCtrl& rCtrl = GetCtrl();

    rCtrl.MarkerDefine(marker, markerType);
    rCtrl.MarkerSetFore(marker, fore);
    rCtrl.MarkerSetBack(marker, back);
}


void CRadNotepadView::OnViewMarker(UINT nId)
{
    Margin nMarker = GetMarginFromMenu(nId);
    CScintillaCtrl& rCtrl = GetCtrl();
    int nMarginWidth = rCtrl.GetMarginWidthN(nMarker);
    if (nMarginWidth)
        rCtrl.SetMarginWidthN(nMarker, 0);
    else
        rCtrl.SetMarginWidthN(nMarker, GetWidth(rCtrl, nMarker));
}


void CRadNotepadView::OnUpdateViewMarker(CCmdUI *pCmdUI)
{
    Margin nMarker = GetMarginFromMenu(pCmdUI->m_nID);
    CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetMarginWidthN(nMarker) != 0);
}


void CRadNotepadView::OnViewWhitespace()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    int ws = rCtrl.GetViewWS();
    ws = ws == SCWS_VISIBLEALWAYS ? SCWS_INVISIBLE : SCWS_VISIBLEALWAYS;
    rCtrl.SetViewWS(ws);
}


void CRadNotepadView::OnUpdateViewWhitespace(CCmdUI *pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetViewWS() != SCWS_INVISIBLE);
}


void CRadNotepadView::OnViewEndOfLine()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    BOOL bEol = rCtrl.GetViewEOL();
    bEol = !bEol;
    rCtrl.SetViewEOL(bEol);
}


void CRadNotepadView::OnUpdateViewEndOfLine(CCmdUI *pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetViewEOL());
}


void CRadNotepadView::OnViewWordWrap()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    int wm = rCtrl.GetWrapMode();
    wm = wm == SC_WRAP_WORD ? SC_WRAP_NONE : SC_WRAP_WORD;
    rCtrl.SetWrapMode(wm);
}


void CRadNotepadView::OnUpdateViewWordWrap(CCmdUI *pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetWrapMode() != SC_WRAP_NONE);
}


int CRadNotepadView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScintillaView::OnCreate(lpCreateStruct) == -1)
        return -1;

    CCreateContext* pContext = (CCreateContext*) lpCreateStruct->lpCreateParams;
#if 0
    CRadNotepadView* pLastView = (CRadNotepadView*) pContext->m_pLastView;
    if (pLastView != nullptr)
    {
        void* p = pLastView->GetCtrl().GetDocPointer();
        GetCtrl().SetDocPointer(p);
    }
#else
    CRadNotepadDoc* pDoc = (CRadNotepadDoc*) pContext->m_pCurrentDoc;
    if (pDoc->GetView() != this)
    {
        void* p = pDoc->GetView()->GetCtrl().GetDocPointer();
        GetCtrl().SetDocPointer(p);
    }
#endif

    return 0;
}

void CRadNotepadView::OnViewUseTabs()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    rCtrl.SetUseTabs(!rCtrl.GetUseTabs());
}

void CRadNotepadView::OnUpdateViewUseTabs(CCmdUI *pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetUseTabs());
}


void CRadNotepadView::OnEditToggleBookmark()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    Sci_Position nPos = rCtrl.GetCurrentPos();
    int nLine = rCtrl.LineFromPosition(nPos);
    if (rCtrl.MarkerGet(nLine) & (1 << RAD_MARKER_BOOKMARK))
        rCtrl.MarkerDelete(nLine, RAD_MARKER_BOOKMARK);
    else
        rCtrl.MarkerAdd(nLine, RAD_MARKER_BOOKMARK);
}

void CRadNotepadView::OnEditPreviousBookmark()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    Sci_Position nPos = rCtrl.GetCurrentPos();
    int nLine = rCtrl.LineFromPosition(nPos);
    nLine = rCtrl.MarkerPrevious(nLine - 1, 1 << RAD_MARKER_BOOKMARK);
    if (nLine < 0)
        nLine = rCtrl.MarkerPrevious(rCtrl.GetLineCount() - 1, 1 << RAD_MARKER_BOOKMARK);
    if (nLine >= 0)
        rCtrl.GotoLine(nLine);
}

void CRadNotepadView::OnEditNextBookmark()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    Sci_Position nPos = rCtrl.GetCurrentPos();
    int nLine = rCtrl.LineFromPosition(nPos);
    nLine = rCtrl.MarkerNext(nLine + 1, 1 << RAD_MARKER_BOOKMARK);
    if (nLine < 0)
        nLine = rCtrl.MarkerNext(0, 1 << RAD_MARKER_BOOKMARK);
    if (nLine >= 0)
        rCtrl.GotoLine(nLine);
}


void CRadNotepadView::OnLineEndingsWindows()
{
    SetLineEndingsMode(SC_EOL_CRLF);
}


void CRadNotepadView::OnLineEndingsUnix()
{
    SetLineEndingsMode(SC_EOL_LF);
}


void CRadNotepadView::OnLineEndingsMac()
{
    SetLineEndingsMode(SC_EOL_CR);
}


void CRadNotepadView::OnUpdateLineEndings(CCmdUI *pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    const int mode = pCmdUI->m_nID - ID_LINEENDINGS_WINDOWS;
    pCmdUI->SetRadio(rCtrl.GetEOLMode() == mode);
}
