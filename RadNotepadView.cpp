
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

#define COLOR_NONE          RGB(0xFE, 0x00, 0xFE)
#define COLOR_WHITE         RGB(0xFF, 0xFF, 0xFF)
#define COLOR_BLACK         RGB(0x00, 0x00, 0x00)
#define COLOR_LT_RED        RGB(0x80, 0x00, 0x00)
#define COLOR_LT_GREEN      RGB(0x00, 0x80, 0x00)
#define COLOR_LT_BLUE       RGB(0x00, 0x00, 0x80)
#define COLOR_LT_CYAN       RGB(0x00, 0x80, 0x80)
#define COLOR_LT_MAGENTA    RGB(0x80, 0x00, 0x80)
#define COLOR_LT_YELLOW     RGB(0x80, 0x80, 0x00)

struct Style
{
    int nID;
    int nStyle;
    COLORREF fore;
    COLORREF back;
    int size;
    const char* face;
    bool bold;
    bool italic;
    bool underline;
};

Style vStyleDefault = { SCLEX_NULL, STYLE_DEFAULT, COLOR_BLACK, COLOR_WHITE, 10, "Consolas" };

Style vStyle[] = {
    { SCLEX_CPP, SCE_C_DEFAULT,                 COLOR_BLACK,        COLOR_NONE },
    { SCLEX_CPP, SCE_C_COMMENT,                 COLOR_LT_GREEN,     COLOR_NONE },
    { SCLEX_CPP, SCE_C_COMMENTLINE,             COLOR_LT_GREEN,     COLOR_NONE },
    { SCLEX_CPP, SCE_C_COMMENTDOC,              COLOR_LT_GREEN,     COLOR_NONE },
    { SCLEX_CPP, SCE_C_COMMENTLINEDOC,          COLOR_LT_GREEN,     COLOR_NONE },
    { SCLEX_CPP, SCE_C_COMMENTDOCKEYWORD,       COLOR_LT_GREEN,     COLOR_NONE },
    { SCLEX_CPP, SCE_C_COMMENTDOCKEYWORDERROR,  COLOR_LT_GREEN,     COLOR_NONE },
    { SCLEX_CPP, SCE_C_NUMBER,                  COLOR_LT_CYAN,      COLOR_NONE },
    { SCLEX_CPP, SCE_C_WORD,                    COLOR_LT_BLUE,      COLOR_NONE, 0, nullptr, true },
    { SCLEX_CPP, SCE_C_STRING,                  COLOR_LT_MAGENTA,   COLOR_NONE },
    { SCLEX_CPP, SCE_C_IDENTIFIER,              COLOR_BLACK,        COLOR_NONE },
    { SCLEX_CPP, SCE_C_PREPROCESSOR,            COLOR_LT_RED,       COLOR_NONE },
    { SCLEX_CPP, SCE_C_OPERATOR,                COLOR_LT_YELLOW,    COLOR_NONE },
    { SCLEX_NULL },
};

void ApplyStyle(CScintillaCtrl& rCtrl, const Style& vStyle)
{
    if (vStyle.fore != COLOR_NONE)
        rCtrl.StyleSetFore(vStyle.nStyle, vStyle.fore);
    if (vStyle.back != COLOR_NONE)
        rCtrl.StyleSetBack(vStyle.nStyle, vStyle.back);
    if (vStyle.size >= 1)
        rCtrl.StyleSetSize(vStyle.nStyle, vStyle.size);
    if (vStyle.face != nullptr)
        rCtrl.StyleSetFont(vStyle.nStyle, vStyle.face);
    if (vStyle.bold)
        rCtrl.StyleSetBold(vStyle.nStyle, TRUE);
    if (vStyle.italic)
        rCtrl.StyleSetItalic(vStyle.nStyle, TRUE);
    if (vStyle.underline)
        rCtrl.StyleSetUnderline(vStyle.nStyle, TRUE);
}

enum Margin
{
    MARGIN_LINENUMBERS,
    MARGIN_SYMBOLS,
    MARGIN_FOLDS,
};

// CRadNotepadView

IMPLEMENT_DYNCREATE(CRadNotepadView, CScintillaView)

BEGIN_MESSAGE_MAP(CRadNotepadView, CScintillaView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CRadNotepadView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, &CRadNotepadView::OnUpdateLine)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, &CRadNotepadView::OnUpdateInsert)
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


// CRadNotepadView message handlers


void CRadNotepadView::OnInitialUpdate()
{
    CScintillaView::OnInitialUpdate();

    CRadNotepadDoc* pDoc = GetDocument();
    CString strFileName = pDoc->GetPathName();
    PCTSTR strExt = PathFindExtension(strFileName);
    const LexerData* pLexerData = GetLexerData(strExt);

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

    //Setup styles
    ApplyStyle(rCtrl, vStyleDefault);
    rCtrl.StyleClearAll();
    if (pLexerData != nullptr)
    {
        // TODO Can I do this just once? Is this state shared across ctrls?
        int i = 0;
        while (vStyle[i].nID != SCLEX_NULL)
        {
            if (vStyle[i].nID == pLexerData->nID)
                ApplyStyle(rCtrl, vStyle[i]);
            ++i;
        }
    }

    //Setup folding
    rCtrl.SetMarginWidthN(MARGIN_FOLDS, 16);
    rCtrl.SetMarginSensitiveN(MARGIN_FOLDS, TRUE);
    rCtrl.SetMarginTypeN(MARGIN_FOLDS, SC_MARGIN_SYMBOL);
    rCtrl.SetMarginMaskN(MARGIN_FOLDS, SC_MASK_FOLDERS);
    rCtrl.SetProperty(_T("fold"), _T("1"));

    rCtrl.SetMarginWidthN(MARGIN_LINENUMBERS, rCtrl.TextWidth(STYLE_LINENUMBER, "99999"));

    //Setup markers
    DefineMarker(SC_MARKNUM_FOLDEROPEN,     SC_MARK_BOXMINUS,           COLOR_WHITE, COLOR_BLACK);
    DefineMarker(SC_MARKNUM_FOLDER,         SC_MARK_BOXPLUS,            COLOR_WHITE, COLOR_BLACK);
    DefineMarker(SC_MARKNUM_FOLDERSUB,      SC_MARK_VLINE,              COLOR_WHITE, COLOR_BLACK);
    DefineMarker(SC_MARKNUM_FOLDERTAIL,     SC_MARK_LCORNER,            COLOR_WHITE, COLOR_BLACK);
    DefineMarker(SC_MARKNUM_FOLDEREND,      SC_MARK_BOXPLUSCONNECTED,   COLOR_WHITE, COLOR_BLACK);
    DefineMarker(SC_MARKNUM_FOLDEROPENMID,  SC_MARK_BOXMINUSCONNECTED,  COLOR_WHITE, COLOR_BLACK);
    DefineMarker(SC_MARKNUM_FOLDERMIDTAIL,  SC_MARK_TCORNER,            COLOR_WHITE, COLOR_BLACK);

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
