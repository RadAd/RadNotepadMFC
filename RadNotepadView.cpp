
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
#include "GoToLineDlg.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// TODO
// Look into replacing scintilla scrollbars with splitter scrollbars (maybe just the vertical one)

#define WM_CHECKUPDATE (WM_USER + 1)

#define RAD_MARKER_BOOKMARK 2

#define ID_VIEW_FIRSTSCHEME             33100
#define ID_VIEW_LASTSCHEME              33199

enum Margin
{
    MARGIN_LINENUMBERS,
    MARGIN_SYMBOLS,
    MARGIN_FOLDS,
};

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
    ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, &CRadNotepadView::OnUpdateLine)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, &CRadNotepadView::OnUpdateInsert)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CRadNotepadView::OnFilePrintPreview)
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
    ON_COMMAND_RANGE(ID_LINEENDINGS_WINDOWS, ID_LINEENDINGS_UNIX, &CRadNotepadView::OnLineEndings)
    ON_UPDATE_COMMAND_UI_RANGE(ID_LINEENDINGS_WINDOWS, ID_LINEENDINGS_UNIX, &CRadNotepadView::OnUpdateLineEndings)
    ON_COMMAND(ID_EDIT_MAKEUPPERCASE, &CRadNotepadView::OnEditMakeUppercase)
    ON_COMMAND(ID_EDIT_MAKELOWERCASE, &CRadNotepadView::OnEditMakeLowercase)
    ON_COMMAND(ID_EDIT_GOTOLINE, &CRadNotepadView::OnEditGotoLine)
    ON_COMMAND(ID_EDIT_FINDPREVIOUS, &CRadNotepadView::OnEditFindPrevious)
    ON_UPDATE_COMMAND_UI(ID_EDIT_FINDPREVIOUS, &CScintillaView::OnUpdateNeedFind)
    ON_COMMAND(ID_EDIT_FINDNEXTCURRENTWORD, &CRadNotepadView::OnEditFindNextCurrentWord)
    ON_COMMAND(ID_EDIT_FINDPREVIOUSCURRENTWORD, &CRadNotepadView::OnEditFindPreviousCurrentWord)
    ON_MESSAGE(WM_CHECKUPDATE, &CRadNotepadView::OnCheckUpdate)
    ON_COMMAND(ID_EDIT_FINDMATCHINGBRACE, &CRadNotepadView::OnEditFindMatchingBrace)
    ON_COMMAND(ID_VIEW_SCHEMENONE, &CRadNotepadView::OnSchemeNone)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SCHEMENONE, &CRadNotepadView::OnUpdateSchemeNone)
    ON_COMMAND_RANGE(ID_VIEW_FIRSTSCHEME, ID_VIEW_LASTSCHEME, &CRadNotepadView::OnScheme)
    ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_FIRSTSCHEME, ID_VIEW_LASTSCHEME, &CRadNotepadView::OnUpdateScheme)
END_MESSAGE_MAP()

// CRadNotepadView construction/destruction

CRadNotepadView::CRadNotepadView()
    : m_pLanguage(nullptr)
    , m_bHighlightMatchingBraces(FALSE)
    , m_bAutoIndent(FALSE)
{
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

void CRadNotepadView::OnCharAdded(_Inout_ SCNotification* pSCNotification)
{
    CScintillaView::OnCharAdded(pSCNotification);
    CScintillaCtrl& rCtrl = GetCtrl();
    if (m_bAutoIndent)
    {
        if ((rCtrl.GetEOLMode() == SC_EOL_CR &&  pSCNotification->ch == '\r')
            || (rCtrl.GetEOLMode() == SC_EOL_LF &&  pSCNotification->ch == '\n')
            || (rCtrl.GetEOLMode() == SC_EOL_CRLF &&  pSCNotification->ch == '\n'))
        {
            Sci_Position nPos = rCtrl.GetCurrentPos();
            int nLine = rCtrl.LineFromPosition(nPos);
            if (nLine > 0)
            {
                CString srtLine = rCtrl.GetLine(nLine - 1);
                for (int pos = 0; pos < srtLine.GetLength(); pos++)
                {
                    if (srtLine[pos] != ' ' && srtLine[pos] != '\t')
                    {
                        srtLine = srtLine.Left(pos);
                        break;
                    }
                }
                if (!srtLine.IsEmpty())
                    rCtrl.ReplaceSel(srtLine);
            }
        }
    }
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

static bool IsBrace(int c)
{
    return strchr("()[]{}<>", c) != nullptr;
}

void CRadNotepadView::OnUpdateUI(_Inout_ SCNotification* pSCNotification)
{
    CScintillaView::OnUpdateUI(pSCNotification);

    if (m_bHighlightMatchingBraces)
    {
        CScintillaCtrl& rCtrl = GetCtrl();
        Sci_Position nPos = rCtrl.GetCurrentPos();

        int c = rCtrl.GetCharAt(nPos);
        if (IsBrace(c))
        {
            Sci_Position nMatch = rCtrl.BraceMatch(nPos, 0);
            if (nMatch >= 0)
                rCtrl.BraceHighlight(nPos, nMatch);
            else
                rCtrl.BraceBadLight(nPos);
        }
        else
            rCtrl.BraceBadLight(INVALID_POSITION);
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
    sLine.Format(ID_INDICATOR_LINE, nLine + 1, nColumn + 1, nPos + 1);
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

    m_pLanguage = GetLanguageForExt(&theApp.m_Settings.editor.rTheme, strExt);

    // TODO Copy some settings from other ctrl (ie split view, new window)

    CScintillaCtrl& rCtrl = GetCtrl();
    const EditorSettings& settings = theApp.m_Settings.editor;

    Apply(rCtrl, m_pLanguage, &settings.rTheme);

    //Setup folding
    rCtrl.SetMarginWidthN(MARGIN_FOLDS, settings.bShowFolds ? GetWidth(rCtrl, MARGIN_FOLDS) : 0);
    rCtrl.SetMarginSensitiveN(MARGIN_FOLDS, TRUE);
    rCtrl.SetMarginTypeN(MARGIN_FOLDS, SC_MARGIN_SYMBOL);
    rCtrl.SetMarginMaskN(MARGIN_FOLDS, SC_MASK_FOLDERS);
    rCtrl.SetProperty(_T("fold"), _T("1"));

    rCtrl.SetMarginWidthN(MARGIN_LINENUMBERS, settings.bShowLineNumbers ? GetWidth(rCtrl, MARGIN_LINENUMBERS) : 0);
    rCtrl.SetMarginWidthN(MARGIN_SYMBOLS, settings.bShowBookmarks ? GetWidth(rCtrl, MARGIN_SYMBOLS) : 0);

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

    rCtrl.SetCaretFore(settings.cCaretFG);
    rCtrl.SetCaretStyle(settings.nCaretStyle);
    rCtrl.SetCaretWidth(settings.nCaretWidth);

    rCtrl.SetEOLMode(pDoc->GetLineEndingMode());

    if (settings.bShowIndentGuides)
        rCtrl.SetIndentationGuides(SC_IV_LOOKBOTH);
    //rCtrl.SetHighlightGuide(6); // TODO Not sure what this does

    m_bHighlightMatchingBraces = settings.bHighlightMatchingBraces;
    m_bAutoIndent = settings.bAutoIndent;

    rCtrl.ClearCmdKey('[' | (SCMOD_CTRL << 16));
    rCtrl.ClearCmdKey('[' | ((SCMOD_CTRL | SCMOD_SHIFT) << 16));
    rCtrl.ClearCmdKey(']' | (SCMOD_CTRL << 16));
    rCtrl.ClearCmdKey(']' | ((SCMOD_CTRL | SCMOD_SHIFT) << 16));

    rCtrl.UsePopUp(SC_POPUP_NEVER);

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


void CRadNotepadView::OnViewMarker(UINT nID)
{
    Margin nMarker = static_cast<Margin>(nID - ID_VIEW_LINENUMBERS);
    CScintillaCtrl& rCtrl = GetCtrl();
    int nMarginWidth = rCtrl.GetMarginWidthN(nMarker);
    if (nMarginWidth)
        rCtrl.SetMarginWidthN(nMarker, 0);
    else
        rCtrl.SetMarginWidthN(nMarker, GetWidth(rCtrl, nMarker));
}


void CRadNotepadView::OnUpdateViewMarker(CCmdUI *pCmdUI)
{
    Margin nMarker = static_cast<Margin>(pCmdUI->m_nID - ID_VIEW_LINENUMBERS);
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


void CRadNotepadView::OnLineEndings(UINT nID)
{
    const int mode = nID - ID_LINEENDINGS_WINDOWS;
    SetLineEndingsMode(mode);
}

void CRadNotepadView::OnUpdateLineEndings(CCmdUI *pCmdUI)
{
    CScintillaCtrl& rCtrl = GetCtrl();
    const int mode = pCmdUI->m_nID - ID_LINEENDINGS_WINDOWS;
    pCmdUI->SetRadio(rCtrl.GetEOLMode() == mode);
}


void CRadNotepadView::OnEditMakeUppercase()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    CString sel = rCtrl.GetSelText();
    if (!sel.IsEmpty())
    {
        sel.MakeUpper();
        rCtrl.ReplaceSel(sel);

        Sci_Position start = rCtrl.GetSelectionStart();
        Sci_Position end = rCtrl.GetSelectionStart();
        rCtrl.SetSel(start - sel.GetLength(), end);
    }
}


void CRadNotepadView::OnEditMakeLowercase()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    CString sel = rCtrl.GetSelText();
    if (!sel.IsEmpty())
    {
        sel.MakeLower();
        rCtrl.ReplaceSel(sel);

        Sci_Position start = rCtrl.GetSelectionStart();
        Sci_Position end = rCtrl.GetSelectionStart();
        rCtrl.SetSel(start - sel.GetLength(), end);
    }
}

void CRadNotepadView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    if (bActivate)
        PostMessage(WM_CHECKUPDATE);

    CScintillaView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


afx_msg LRESULT CRadNotepadView::OnCheckUpdate(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    CRadNotepadDoc* pDoc = GetDocument();
    pDoc->CheckUpdated();
    pDoc->CheckReadOnly();
    return 0;
}

void CRadNotepadView::OnEditGotoLine()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    Sci_Position nPos = rCtrl.GetCurrentPos();
    int nLine = rCtrl.LineFromPosition(nPos);

    CGoToLineDlg dlg;
    dlg.m_nLine = nLine + 1;
    dlg.m_nMaxLine = rCtrl.GetLineCount();
    if (dlg.DoModal() == IDOK)
        rCtrl.GotoLine(dlg.m_nLine - 1);
}

extern CScintillaEditState g_scintillaEditState;

void CRadNotepadView::OnEditFindPrevious()
{
    if (!FindText(g_scintillaEditState.strFind, !g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression))
        TextNotFound(g_scintillaEditState.strFind, !g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE);
}

static CStringW GetTextRange(CScintillaCtrl& rCtrl, Sci_CharacterRange cr)
{
    CStringA ret;

    Sci_TextRange tr = {};
    tr.chrg = cr;
    //int nLen = rCtrl.GetTextRange(&tr);
    int nLen = tr.chrg.cpMax - tr.chrg.cpMin + 1;
    tr.lpstrText = ret.GetBufferSetLength(nLen);
    nLen = rCtrl.GetTextRange(&tr);
    ret.ReleaseBuffer();

    return CScintillaCtrl::UTF82W(ret, -1);
}

static CStringW GetCurrentWord(CScintillaCtrl& rCtrl)
{
    if (rCtrl.GetSelectionEmpty())
    {
        Sci_Position nPos = rCtrl.GetCurrentPos();
        Sci_CharacterRange cr;
        cr.cpMin = rCtrl.WordStartPosition(nPos, TRUE);
        cr.cpMax = rCtrl.WordEndPosition(nPos, TRUE);
        return GetTextRange(rCtrl, cr);
    }
    else
        return rCtrl.GetSelText();
}

void CRadNotepadView::OnEditFindNextCurrentWord()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    g_scintillaEditState.strFind = GetCurrentWord(rCtrl);
    g_scintillaEditState.bNext = TRUE;
    if (!FindText(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression))
        TextNotFound(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE);
}

void CRadNotepadView::OnEditFindPreviousCurrentWord()
{
    CScintillaCtrl& rCtrl = GetCtrl();
    g_scintillaEditState.strFind = GetCurrentWord(rCtrl);
    g_scintillaEditState.bNext = TRUE;
    if (!FindText(g_scintillaEditState.strFind, !g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression))
        TextNotFound(g_scintillaEditState.strFind, !g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE);
}


void CRadNotepadView::OnEditFindMatchingBrace()
{
    // TODO Handle shift to also select
    CScintillaCtrl& rCtrl = GetCtrl();
    Sci_Position nPos = rCtrl.GetCurrentPos();
    int c = rCtrl.GetCharAt(nPos);
    if (IsBrace(c))
    {
        Sci_Position nMatch = rCtrl.BraceMatch(nPos, 0);
        if (nMatch >= 0)
            rCtrl.GotoPos(nMatch);
    }
}

void CRadNotepadView::OnSchemeNone()
{
    const Language* pLanguage = nullptr;

    m_pLanguage = pLanguage;
    CScintillaCtrl& rCtrl = GetCtrl();
    const EditorSettings& settings = theApp.m_Settings.editor;
    Apply(rCtrl, m_pLanguage, &settings.rTheme);
}

void CRadNotepadView::OnUpdateSchemeNone(CCmdUI *pCmdUI)
{
    // TODO Sort
    if (pCmdUI->m_pSubMenu != nullptr)
    {
        for (int i = ID_VIEW_FIRSTSCHEME; i < ID_VIEW_LASTSCHEME; ++i)
            pCmdUI->m_pSubMenu->DeleteMenu(i, MF_BYCOMMAND);

        const std::vector<Language>& vecLanguage = theApp.m_Settings.editor.rTheme.vecLanguage;

        int nID = ID_VIEW_FIRSTSCHEME;
        struct LanguageMenuItem
        {
            const Language* pLanguage;
            int nID;
            bool operator<(const LanguageMenuItem& other) const
            {
                return pLanguage->title < other.pLanguage->title;
            }
        };
        std::vector<LanguageMenuItem> vecSortLanguage;
        for (const Language& rLanguage : vecLanguage)
        {
            vecSortLanguage.push_back(LanguageMenuItem());
            LanguageMenuItem& lmi = vecSortLanguage.back();
            lmi.pLanguage = &rLanguage;
            lmi.nID = nID++;
        }

        std::sort(vecSortLanguage.begin(), vecSortLanguage.end());

        int nIndex = pCmdUI->m_pSubMenu->GetMenuItemCount();
        for (const LanguageMenuItem& rLanguage : vecSortLanguage)
            pCmdUI->m_pSubMenu->InsertMenu(nIndex++, MF_STRING | MF_BYPOSITION, rLanguage.nID, rLanguage.pLanguage->title);

        pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
        pCmdUI->m_nIndexMax = pCmdUI->m_pSubMenu->GetMenuItemCount();
    }

    pCmdUI->SetRadio(m_pLanguage == nullptr);
}

void CRadNotepadView::OnScheme(UINT nID)
{
    const std::vector<Language>& vecLanguage = theApp.m_Settings.editor.rTheme.vecLanguage;
    const Language* pLanguage = &vecLanguage[nID - ID_VIEW_FIRSTSCHEME];

    m_pLanguage = pLanguage;
    CScintillaCtrl& rCtrl = GetCtrl();
    const EditorSettings& settings = theApp.m_Settings.editor;
    Apply(rCtrl, m_pLanguage, &settings.rTheme);
}

void CRadNotepadView::OnUpdateScheme(CCmdUI *pCmdUI)
{
    const std::vector<Language>& vecLanguage = theApp.m_Settings.editor.rTheme.vecLanguage;
    pCmdUI->SetRadio(m_pLanguage == &vecLanguage[pCmdUI->m_nID - ID_VIEW_FIRSTSCHEME]);
}


void CRadNotepadView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
    if (lHint == HINT_UPDATE_SETTINGS)
    {
        CScintillaCtrl& rCtrl = GetCtrl();
        const EditorSettings& settings = theApp.m_Settings.editor;
        Apply(rCtrl, m_pLanguage, &settings.rTheme);
    }
}
