#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "RadNotepad.h"
#endif

#include "RadNotepadDoc.h"
#include "RadNotepadView.h"
#include "RadVisualManager.h"
#include "MainFrm.h"
#include "GoToLineDlg.h"
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// TODO
// Look into replacing scintilla scrollbars with splitter scrollbars (maybe just the vertical one)
// Replace tabs with spaces or spaces with tabs
// Support to comment out selection

#define WM_CHECKUPDATE (WM_USER + 1)

#define RAD_MARKER_BOOKMARK 2

#define RAD_INDIC_TRAILINGWS 4

#define ID_VIEW_FIRSTSCHEME             33100
#define ID_VIEW_LASTSCHEME              33199

namespace
{
    void AddSearchTextToHistory(CMFCToolBarComboBoxButton* pButton)
    {
        int sel1 = pButton->GetCurSel();
        if (sel1 != CB_ERR && _tcscmp(pButton->GetItem(sel1), pButton->GetText()) != 0)
            sel1 = CB_ERR;
        if (sel1 != 0)
        {
            CString search = pButton->GetText();
            pButton->DeleteItem(sel1);
            int sel = (int) pButton->AddSortedItem(search); // CToolBarHistoryButton::AddSortedItem always inserts in front
            if (sel != 0)
            {   // Item exists, delete and reinsert in correct position
                pButton->DeleteItem(sel);
                sel = (int) pButton->AddSortedItem(search);
            }
        }
        // --- TODO Trim history to 100 items
    }
};

class CRadScintillaCtrl : public Scintilla::CScintillaCtrl
{
protected:
    DECLARE_MESSAGE_MAP()

    void OnMButtonUp(UINT /* nFlags */, CPoint /*point*/)
    {
        if (GetSelectionEmpty())
            Paste();
        else
            Copy();
    }
};

BEGIN_MESSAGE_MAP(CRadScintillaCtrl, CScintillaCtrl)
    ON_WM_MBUTTONUP()
END_MESSAGE_MAP()

// CRadNotepadView

IMPLEMENT_DYNCREATE(CRadNotepadView, CScintillaView)

BEGIN_MESSAGE_MAP(CRadNotepadView, CScintillaView)
    // Standard printing commands
    ON_WM_CREATE()
    ON_WM_CONTEXTMENU()
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, &CRadNotepadView::OnUpdateLine)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_SCHEME, &CRadNotepadView::OnUpdateSchemeIndicator)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE_ENDING, &CRadNotepadView::OnUpdateLineEndingIndicator)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_OVR, &CRadNotepadView::OnUpdateInsert)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CRadNotepadView::OnFilePrintPreview)
    ON_COMMAND_RANGE(ID_MARGINS_1, ID_MARGINS_5, &CRadNotepadView::OnViewMargin)
    ON_UPDATE_COMMAND_UI_RANGE(ID_MARGINS_1, ID_MARGINS_5, &CRadNotepadView::OnUpdateViewMargin)
    ON_COMMAND(ID_VIEW_TRAILINGSPACES, &CRadNotepadView::OnViewTrailingSpaces)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TRAILINGSPACES, &CRadNotepadView::OnUpdateViewTrailingSpaces)
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
    ON_COMMAND(ID_EDIT_DELETETRAILINGSPACES, &CRadNotepadView::OnEditDeleteTrailingSpaces)
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
    ON_COMMAND(ID_SEARCH_NEXT, &CRadNotepadView::OnSearchNext)
    ON_UPDATE_COMMAND_UI(ID_SEARCH_NEXT, &CRadNotepadView::OnUpdateSearchNext)
    ON_COMMAND(ID_SEARCH_PREV, &CRadNotepadView::OnSearchPrev)
    ON_UPDATE_COMMAND_UI(ID_SEARCH_PREV, &CRadNotepadView::OnUpdateSearchPrev)
    ON_COMMAND(ID_SEARCH_TEXT, &CRadNotepadView::OnSearchNext)
    ON_CONTROL(CBN_EDITCHANGE, ID_SEARCH_TEXT, &CRadNotepadView::OnSearchTextEditChange)
    ON_COMMAND(ID_SEARCH_INCREMENTAL, &CRadNotepadView::OnSearchIncremental) // TODO Move to view
    ON_COMMAND(ID_VIEW_RETURN, &CRadNotepadView::OnViewReturn)
END_MESSAGE_MAP()

// CRadNotepadView construction/destruction

CRadNotepadView::CRadNotepadView()
    : m_pLanguage(nullptr)
    , m_bHighlightMatchingBraces(FALSE)
    , m_bAutoIndent(FALSE)
    , m_bShowTrailingSpaces(FALSE)
{
}

CRadNotepadView::~CRadNotepadView()
{
}

void CRadNotepadView::OnCharAdded(_Inout_ Scintilla::NotificationData* pSCNotification)
{
    CScintillaView::OnCharAdded(pSCNotification);
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    if (m_bAutoIndent)
    {
        if ((rCtrl.GetEOLMode() == Scintilla::EndOfLine::Cr && pSCNotification->ch == '\r')
            || (rCtrl.GetEOLMode() == Scintilla::EndOfLine::Lf && pSCNotification->ch == '\n')
            || (rCtrl.GetEOLMode() == Scintilla::EndOfLine::CrLf && pSCNotification->ch == '\n'))
        {
            Scintilla::Position nPos = rCtrl.GetCurrentPos();
            Scintilla::Line nLine = rCtrl.LineFromPosition(nPos);
            if (nLine > 0)
            {
                CString strLine = rCtrl.GetLine(nLine - 1);
                for (int pos = 0; pos < strLine.GetLength(); pos++)
                {
                    if (strLine[pos] != ' ' && strLine[pos] != '\t')
                    {
                        strLine = strLine.Left(pos);
                        break;
                    }
                }
                if (!strLine.IsEmpty())
                    rCtrl.ReplaceSel(strLine);
            }
        }
    }
}

void CRadNotepadView::OnModified(_Inout_ Scintilla::NotificationData* pSCNotification)
{
    CScintillaView::OnModified(pSCNotification);
    if ((pSCNotification->modificationType & (Scintilla::ModificationFlags::InsertText | Scintilla::ModificationFlags::DeleteText)) != Scintilla::ModificationFlags::None)
    {
        CRadNotepadDoc* pDoc = GetDocument();
        pDoc->SyncModified();
    }
    if ((pSCNotification->modificationType & Scintilla::ModificationFlags::InsertText) == Scintilla::ModificationFlags::InsertText)
    {
        if (m_bShowTrailingSpaces)
        {
            Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
            const Scintilla::Line bl = rCtrl.LineFromPosition(pSCNotification->position);
            ShowTrailingSpaces(bl, bl + pSCNotification->linesAdded);
        }
    }
}

static bool IsBrace(int c)
{
    return strchr("()[]{}<>", c) != nullptr;
}

void CRadNotepadView::OnUpdateUI(_Inout_ Scintilla::NotificationData* pSCNotification)
{
    CScintillaView::OnUpdateUI(pSCNotification);

    if (m_bHighlightMatchingBraces)
    {
        Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
        Scintilla::Position nPos = rCtrl.GetCurrentPos();

        int c = rCtrl.GetCharAt(nPos);
        if (IsBrace(c))
        {
            Scintilla::Position nMatch = rCtrl.BraceMatch(nPos, 0);
            if (nMatch >= 0)
                rCtrl.BraceHighlight(nPos, nMatch);
            else
                rCtrl.BraceBadLight(nPos);
        }
        else
            rCtrl.BraceBadLight(INVALID_POSITION);
    }
}

BOOL CRadNotepadView::FindText(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
    CFrameWnd* pMainWnd = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd());
    pMainWnd->SetMessageText(AFX_IDS_IDLEMESSAGE);

    return CScintillaView::FindText(lpszFind, bNext, bCase, bWord, bRegularExpression);
}

void CRadNotepadView::TextNotFound(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_ BOOL bReplaced)
{
    CScintillaView::TextNotFound(lpszFind, bNext, bCase, bWord, bRegularExpression, bReplaced);

    CFrameWnd* pMainWnd = DYNAMIC_DOWNCAST(CFrameWnd, AfxGetMainWnd());
    // TODO Want this message to stick for a little while
    pMainWnd->SetMessageText(IDS_SEARCH_NOT_FOUND);
}

void CRadNotepadView::ShowTrailingSpaces(const Scintilla::Line bl, const Scintilla::Line el)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    rCtrl.SetIndicatorCurrent(RAD_INDIC_TRAILINGWS);
    for (Scintilla::Line l = bl; l <= el; ++l)
    {
        const CString strLine = rCtrl.GetLine(l).TrimRight(L"\r\n");
        CString strLineTrim = strLine;
        strLineTrim.TrimRight();
        const int diff = strLine.GetLength() - strLineTrim.GetLength();
        if (diff > 0)
        {
            Scintilla::Position end = rCtrl.GetLineEndPosition(l);
            rCtrl.IndicatorFillRange(end - diff, diff);
        }
    }
}

// CRadNotepadView printing

void CRadNotepadView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
    AFXPrintPreview(this);
#endif
}

void CRadNotepadView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
    theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

void CRadNotepadView::OnUpdateLine(CCmdUI* pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    const Scintilla::Position nPos = rCtrl.GetCurrentPos();
    const Scintilla::Line nLine = rCtrl.LineFromPosition(nPos);
    const Scintilla::Position nLineStart = rCtrl.PositionFromLine(nLine);
    const Scintilla::Position nColumn = rCtrl.GetColumn(nPos);
    const Scintilla::Position nAnchor = rCtrl.GetAnchor();

    CString sLine;
    sLine.Format(ID_INDICATOR_LINE, nLine + 1, nColumn + 1, nPos - nLineStart + 1, abs(nAnchor - nPos));
    pCmdUI->SetText(sLine);
    pCmdUI->Enable();
}

void CRadNotepadView::OnUpdateSchemeIndicator(CCmdUI* pCmdUI)
{
    if (m_pLanguage != nullptr)
        pCmdUI->SetText(m_pLanguage->title);
    else
    {
        CString strText;
        VERIFY(strText.LoadString(IDS_SCHEME_NONE));
        pCmdUI->SetText(strText);
    }
}

void CRadNotepadView::OnUpdateLineEndingIndicator(CCmdUI* pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    LPCTSTR eol[] = {
        _T("CRLF"), // Scintilla::EndOfLine::CrLf
        _T("CR"), // Scintilla::EndOfLine::Cr
        _T("LF"), // Scintilla::EndOfLine::Lf
    };
    pCmdUI->SetText(eol[static_cast<std::underlying_type_t<Scintilla::EndOfLine>>(rCtrl.GetEOLMode())]);
}

void CRadNotepadView::OnUpdateInsert(CCmdUI* pCmdUI)
{
    CString sText;
    VERIFY(sText.LoadString(ID_INDICATOR_OVR));
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

void CRadNotepadView::SetLineEndingsMode(Scintilla::EndOfLine mode)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    if (rCtrl.GetEOLMode() != mode)
    {
        int r = AfxMessageBox(IDS_CONVERTLINEENDINGS, MB_YESNOCANCEL);
        if (r == IDYES)
            rCtrl.ConvertEOLs(mode);
        if (r != IDCANCEL)
            rCtrl.SetEOLMode(mode);
    }
}

CStringW CRadNotepadView::GetTextRange(Scintilla::CharacterRange cr)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    CStringA ret;

    Scintilla::TextRange tr = {};
    tr.chrg = cr;
    //Scintilla::Position nLen = rCtrl.GetTextRange(&tr);
    int nLen = tr.chrg.cpMax - tr.chrg.cpMin + 1;
    tr.lpstrText = ret.GetBufferSetLength(nLen);
    /*Scintilla::Position nRange =*/ rCtrl.GetTextRange(&tr);
    ret.ReleaseBuffer();

    return Scintilla::CScintillaCtrl::UTF82W(ret, -1);
}

CStringW CRadNotepadView::GetCurrentWord(BOOL bSelect)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    if (rCtrl.GetSelectionEmpty())
    {
        if (bSelect)
        {
            Scintilla::Position nPos = rCtrl.GetCurrentPos();
            Scintilla::Position start = rCtrl.WordStartPosition(nPos, TRUE);
            Scintilla::Position end = rCtrl.WordEndPosition(nPos, TRUE);
            rCtrl.SetSel(start, end);
            return rCtrl.GetSelText();
        }
        else
        {
            Scintilla::Position nPos = rCtrl.GetCurrentPos();
            Scintilla::CharacterRange cr;
            cr.cpMin = static_cast<Scintilla::PositionCR>(rCtrl.WordStartPosition(nPos, TRUE));
            cr.cpMax = static_cast<Scintilla::PositionCR>(rCtrl.WordEndPosition(nPos, TRUE));
            return GetTextRange(cr);
        }
    }
    else
        return rCtrl.GetSelText();
}

// CRadNotepadView message handlers

void CRadNotepadView::OnInitialUpdate()
{
    CScintillaView::OnInitialUpdate();

    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();

    CRadNotepadDoc* pDoc = GetDocument();
    CString strFileName = pDoc->GetPathName();
    PCTSTR strExt = PathFindExtension(strFileName);
    if (strExt[0] == _T('\0'))
        strExt = PathFindFileName(strFileName);

    m_pLanguage = GetLanguageForExt(&theApp.m_Settings.user, strExt);
    if (m_pLanguage == nullptr)
    {
        CString strLine = rCtrl.GetLine(0);
        // TODO Define this in a file somewhere
        if (strLine.Left(5) == L"<?xml")
            m_pLanguage = GetLanguage(&theApp.m_Settings.user, L"xml");
    }

    // TODO Copy some settings from other ctrl (ie split view, new window)

    const Theme* pTheme = &theApp.m_Settings.user;

    Apply(rCtrl, m_pLanguage, pTheme);

    rCtrl.SetEOLMode(pDoc->GetLineEndingMode());

    const ThemeEditor& pThemeEditor = m_pLanguage != nullptr ? m_pLanguage->editor : pTheme->editor;

    m_bShowTrailingSpaces = Merge(pThemeEditor.bShowTrailingSpaces, &pTheme->editor.bShowTrailingSpaces, Bool3::B3_UNDEFINED, Bool3::B3_FALSE) == Bool3::B3_TRUE;
    m_bHighlightMatchingBraces = Merge(pThemeEditor.bHighlightMatchingBraces, &pTheme->editor.bHighlightMatchingBraces, Bool3::B3_UNDEFINED, Bool3::B3_FALSE) == Bool3::B3_TRUE;
    m_bAutoIndent = Merge(pThemeEditor.bAutoIndent, &pTheme->editor.bAutoIndent, Bool3::B3_UNDEFINED, Bool3::B3_FALSE) == Bool3::B3_TRUE;

    rCtrl.ClearCmdKey('[' | (SCMOD_CTRL << 16));
    rCtrl.ClearCmdKey('[' | ((SCMOD_CTRL | SCMOD_SHIFT) << 16));
    rCtrl.ClearCmdKey(']' | (SCMOD_CTRL << 16));
    rCtrl.ClearCmdKey(']' | ((SCMOD_CTRL | SCMOD_SHIFT) << 16));

    rCtrl.UsePopUp(Scintilla::PopUp::Never);
    rCtrl.SetWrapVisualFlags(Scintilla::WrapVisualFlag::End);
    rCtrl.SetTechnology(Scintilla::Technology::DirectWrite);

    rCtrl.IndicSetStyle(RAD_INDIC_TRAILINGWS, Scintilla::IndicatorStyle::FullBox);
    rCtrl.IndicSetFore(RAD_INDIC_TRAILINGWS, 0x0000FF);
    rCtrl.IndicSetAlpha(RAD_INDIC_TRAILINGWS, (Scintilla::Alpha) 128);
    if (m_bShowTrailingSpaces)
        ShowTrailingSpaces(0, rCtrl.GetLineCount() - 1);

#if 0
    //Setup auto completion
    rCtrl.AutoCSetSeparator(10); //Use a separator of line feed

                                 //Setup call tips
    rCtrl.SetMouseDwellTime(1000);

    //Enable Multiple selection
    rCtrl.SetMultipleSelection(TRUE);
#endif
}

std::unique_ptr<Scintilla::CScintillaCtrl> CRadNotepadView::CreateScintillaControl()
{
    return std::make_unique<CRadScintillaCtrl>();
}

void CRadNotepadView::OnViewMargin(UINT nID)
{
    Theme* pTheme = &theApp.m_Settings.user;
    size_t i = nID - ID_MARGINS_1;
    const std::vector<Margin>& vecMargin = m_pLanguage == nullptr ? pTheme->vecMargin : m_pLanguage->vecMargin;
    if (i >= 0 && i < vecMargin.size())
    {
        Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
        const Margin& margin = vecMargin[i];
        bool show = rCtrl.GetMarginWidthN(margin.id) > 0;
        show = !show;
        int width = 0;
        if (show)
        {
            const Margin* pBaseMargin = m_pLanguage == nullptr ? GetKey(pTheme->vecMargin, margin.id) : nullptr;
            width = GetMarginWidth(rCtrl, margin, pBaseMargin);
        }
        rCtrl.SetMarginWidthN(margin.id, width);
    }
}

void CRadNotepadView::OnUpdateViewMargin(CCmdUI *pCmdUI)
{
    const Theme* pTheme = &theApp.m_Settings.user;
    size_t i = pCmdUI->m_nID - ID_MARGINS_1;
    const std::vector<Margin>& vecMargin = m_pLanguage == nullptr ? pTheme->vecMargin : m_pLanguage->vecMargin;
    if (i >= 0 && i < vecMargin.size())
    {
        Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
        const Margin& margin = vecMargin[i];
        pCmdUI->SetText(margin.name);
        bool show = rCtrl.GetMarginWidthN(margin.id) > 0;
        pCmdUI->SetCheck(show);
    }
    else if (pCmdUI->m_pMenu != nullptr)
        pCmdUI->m_pMenu->RemoveMenu(pCmdUI->m_nID, MF_BYCOMMAND);
}

void CRadNotepadView::OnViewTrailingSpaces()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    if (m_bShowTrailingSpaces)
    {
        rCtrl.SetIndicatorCurrent(RAD_INDIC_TRAILINGWS);
        rCtrl.IndicatorClearRange(0, rCtrl.GetLength());
    }
    else
        ShowTrailingSpaces(0, rCtrl.GetLineCount() - 1);
    m_bShowTrailingSpaces = !m_bShowTrailingSpaces;
}

void CRadNotepadView::OnUpdateViewTrailingSpaces(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_bShowTrailingSpaces);
}

void CRadNotepadView::OnViewWhitespace()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::WhiteSpace ws = rCtrl.GetViewWS();
    const Theme* pTheme = &theApp.m_Settings.user;
    const ThemeEditor& pThemeEditor = m_pLanguage != nullptr ? m_pLanguage->editor : pTheme->editor;
    ws = ws == Scintilla::WhiteSpace::Invisible ? Merge(pThemeEditor.nWhitespaceMode, &pTheme->editor.nWhitespaceMode, 0, Scintilla::WhiteSpace::VisibleAlways) : Scintilla::WhiteSpace::Invisible;
    rCtrl.SetViewWS(ws);
}

void CRadNotepadView::OnUpdateViewWhitespace(CCmdUI *pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetViewWS() != Scintilla::WhiteSpace::Invisible);
}

void CRadNotepadView::OnViewEndOfLine()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    BOOL bEol = rCtrl.GetViewEOL();
    bEol = !bEol;
    rCtrl.SetViewEOL(bEol);
}

void CRadNotepadView::OnUpdateViewEndOfLine(CCmdUI *pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetViewEOL());
}

void CRadNotepadView::OnViewWordWrap()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::Wrap wm = rCtrl.GetWrapMode();
    wm = wm == Scintilla::Wrap::Word ? Scintilla::Wrap::None : Scintilla::Wrap::Word;
    rCtrl.SetWrapMode(wm);
}

void CRadNotepadView::OnUpdateViewWordWrap(CCmdUI *pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetWrapMode() != Scintilla::Wrap::None);
}

int CRadNotepadView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CScintillaView::OnCreate(lpCreateStruct) == -1)
        return -1;

    CRadVisualManagerDark* pDark = DYNAMIC_DOWNCAST(CRadVisualManagerDark, CMFCVisualManager::GetInstance());
    CRadVisualManagerDark::Init(&GetCtrl(), pDark != nullptr);

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
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    rCtrl.SetUseTabs(!rCtrl.GetUseTabs());
}

void CRadNotepadView::OnUpdateViewUseTabs(CCmdUI *pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    pCmdUI->SetCheck(rCtrl.GetUseTabs());
}

void CRadNotepadView::OnEditToggleBookmark()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::Position nPos = rCtrl.GetCurrentPos();
    Scintilla::Line nLine = rCtrl.LineFromPosition(nPos);
    if (rCtrl.MarkerGet(nLine) & (1 << RAD_MARKER_BOOKMARK))
        rCtrl.MarkerDelete(nLine, RAD_MARKER_BOOKMARK);
    else
        rCtrl.MarkerAdd(nLine, RAD_MARKER_BOOKMARK);
}

void CRadNotepadView::OnEditPreviousBookmark()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::Position nPos = rCtrl.GetCurrentPos();
    Scintilla::Line nLine = rCtrl.LineFromPosition(nPos);
    nLine = rCtrl.MarkerPrevious(nLine - 1, 1 << RAD_MARKER_BOOKMARK);
    if (nLine < 0)
        nLine = rCtrl.MarkerPrevious(rCtrl.GetLineCount() - 1, 1 << RAD_MARKER_BOOKMARK);
    if (nLine >= 0)
        rCtrl.GotoLine(nLine);
}

void CRadNotepadView::OnEditNextBookmark()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::Position nPos = rCtrl.GetCurrentPos();
    Scintilla::Line nLine = rCtrl.LineFromPosition(nPos);
    nLine = rCtrl.MarkerNext(nLine + 1, 1 << RAD_MARKER_BOOKMARK);
    if (nLine < 0)
        nLine = rCtrl.MarkerNext(0, 1 << RAD_MARKER_BOOKMARK);
    if (nLine >= 0)
        rCtrl.GotoLine(nLine);
}

void CRadNotepadView::OnLineEndings(UINT nID)
{
    const Scintilla::EndOfLine mode = static_cast<Scintilla::EndOfLine>(nID - ID_LINEENDINGS_WINDOWS);
    SetLineEndingsMode(mode);
}

void CRadNotepadView::OnUpdateLineEndings(CCmdUI *pCmdUI)
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    const Scintilla::EndOfLine mode = static_cast<Scintilla::EndOfLine>(pCmdUI->m_nID - ID_LINEENDINGS_WINDOWS);
    pCmdUI->SetRadio(rCtrl.GetEOLMode() == mode);
}

void CRadNotepadView::OnEditMakeUppercase()
{
    CString sel = GetCurrentWord(TRUE);
    if (!sel.IsEmpty())
    {
        Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
        rCtrl.UpperCase();
    }
}

void CRadNotepadView::OnEditMakeLowercase()
{
    CString sel = GetCurrentWord(TRUE);
    if (!sel.IsEmpty())
    {
        Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
        rCtrl.LowerCase();
    }
}

void CRadNotepadView::OnEditDeleteTrailingSpaces()
{
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    rCtrl.BeginUndoAction();
    for (Scintilla::Line l = 0; l < rCtrl.GetLineCount(); ++l)
    {
        const CString strLine = rCtrl.GetLine(l).TrimRight(L"\r\n");
        CString strLineTrim = strLine;
        strLineTrim.TrimRight();
        const int diff = strLine.GetLength() - strLineTrim.GetLength();
        if (diff > 0)
        {
            Scintilla::Position end = rCtrl.GetLineEndPosition(l);
            rCtrl.DeleteRange(end - diff, diff);
        }
    }
    rCtrl.EndUndoAction();
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
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::Position nPos = rCtrl.GetCurrentPos();
    Scintilla::Line nLine = rCtrl.LineFromPosition(nPos);

    CGoToLineDlg dlg;
    dlg.m_nLine = nLine + 1;
    dlg.m_nMaxLine = rCtrl.GetLineCount();
    if (dlg.DoModal() == IDOK)
        rCtrl.GotoLine(dlg.m_nLine - 1);
}

extern Scintilla::CScintillaEditState g_scintillaEditState;

void CRadNotepadView::OnEditFindPrevious()
{
    BOOL bNext = g_scintillaEditState.bNext;
    OnFindNext(g_scintillaEditState.strFind, !bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression);
    g_scintillaEditState.bNext = bNext;
}

void CRadNotepadView::OnEditFindNextCurrentWord()
{
    CString strFind = GetCurrentWord();

    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    pButton->SetText(strFind);
    AddSearchTextToHistory(pButton);

    m_bFirstSearch = TRUE;
    OnFindNext(strFind, TRUE, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression);
}

void CRadNotepadView::OnEditFindPreviousCurrentWord()
{
    CString strFind = GetCurrentWord();

    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    pButton->SetText(strFind);
    AddSearchTextToHistory(pButton);

    m_bFirstSearch = TRUE;
    OnFindNext(strFind, FALSE, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression);
}

void CRadNotepadView::OnEditFindMatchingBrace()
{
    // TODO Handle shift to also select
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    Scintilla::Position nPos = rCtrl.GetCurrentPos();
    int c = rCtrl.GetCharAt(nPos);
    if (IsBrace(c))
    {
        Scintilla::Position nMatch = rCtrl.BraceMatch(nPos, 0);
        if (nMatch >= 0)
            rCtrl.GotoPos(nMatch);
    }
}

void CRadNotepadView::OnSchemeNone()
{
    const Language* pLanguage = nullptr;

    m_pLanguage = pLanguage;
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    const Theme* pTheme = &theApp.m_Settings.user;
    Apply(rCtrl, m_pLanguage, pTheme);
}

void CRadNotepadView::OnUpdateSchemeNone(CCmdUI *pCmdUI)
{
    CMenu* pMenu = pCmdUI->m_pSubMenu ? pCmdUI->m_pSubMenu : pCmdUI->m_pMenu;
    if (pMenu != nullptr)
    {
        for (int i = ID_VIEW_FIRSTSCHEME; i < ID_VIEW_LASTSCHEME; ++i)
            pMenu->DeleteMenu(i, MF_BYCOMMAND);

        const std::vector<Language>& vecLanguage = theApp.m_Settings.user.vecLanguage;

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
            if (!rLanguage.internal)
            {
                vecSortLanguage.push_back(LanguageMenuItem());
                LanguageMenuItem& lmi = vecSortLanguage.back();
                lmi.pLanguage = &rLanguage;
                lmi.nID = nID++;
            }
            else
                nID++;
        }

        std::sort(vecSortLanguage.begin(), vecSortLanguage.end());

        int nIndex = pMenu->GetMenuItemCount();
        for (const LanguageMenuItem& rLanguage : vecSortLanguage)
            pMenu->InsertMenu(nIndex++, MF_STRING | MF_BYPOSITION, rLanguage.nID, rLanguage.pLanguage->title);

        pCmdUI->m_bEnableChanged = TRUE;    // all the added items are enabled
        pCmdUI->m_nIndexMax = pMenu->GetMenuItemCount();
    }

    pCmdUI->SetRadio(m_pLanguage == nullptr);
}

void CRadNotepadView::OnScheme(UINT nID)
{
    const std::vector<Language>& vecLanguage = theApp.m_Settings.user.vecLanguage;
    const Language* pLanguage = &vecLanguage[nID - ID_VIEW_FIRSTSCHEME];

    m_pLanguage = pLanguage;
    Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
    const Theme* pTheme = &theApp.m_Settings.user;
    Apply(rCtrl, m_pLanguage, pTheme);
}

void CRadNotepadView::OnUpdateScheme(CCmdUI *pCmdUI)
{
    const std::vector<Language>& vecLanguage = theApp.m_Settings.user.vecLanguage;
    pCmdUI->SetRadio(m_pLanguage == &vecLanguage[pCmdUI->m_nID - ID_VIEW_FIRSTSCHEME]);
}

void CRadNotepadView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* pHint)
{
    CRadNotepadDoc* pDoc = GetDocument();
    switch (lHint)
    {
    case HINT_UPDATE_SETTINGS:
        {
            Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
            const Theme* pTheme = &theApp.m_Settings.user;
            Apply(rCtrl, m_pLanguage, pTheme);
        }
        break;

    case HINT_PATH_UPDATED:
        if (pDoc->GetView() == this && m_pLanguage == nullptr)
        {
            CString strFileName = pDoc->GetPathName();
            PCTSTR strExt = PathFindExtension(strFileName);
            if (strExt[0] == _T('\0'))
                strExt = PathFindFileName(strFileName);

            m_pLanguage = GetLanguageForExt(&theApp.m_Settings.user, strExt);

            Scintilla::CScintillaCtrl& rCtrl = GetCtrl();
            const Theme* pTheme = &theApp.m_Settings.user;
            Apply(rCtrl, m_pLanguage, pTheme);

            CFrameWnd* pFrame = GetParentFrame();
            if (pFrame->GetIcon(FALSE) != NULL)
            {
                DestroyIcon(pFrame->SetIcon(NULL, FALSE));
                pFrame->OnUpdateFrameTitle(TRUE);
            }
            // TODO SetIcon is calling UpdateTabs but it doesn't seem to redraw it.
        }
        break;

    case HINT_SHELL_CHANGED:
        if (pDoc->GetView() == this && !pDoc->GetPathName().IsEmpty())
        {
            const CShellChanged* psc = DYNAMIC_DOWNCAST(CShellChanged, pHint);
            CString strFileName = pDoc->GetPathName();
            if (strFileName == psc->strName)
            {
                if (psc->wEventId & SHCNE_RENAMEITEM || psc->wEventId & SHCNE_RENAMEFOLDER)
                    pDoc->SetPathName(psc->strNewName);
            }
        }
        break;

    case HINT_REVERT_PRE:
        {
            //RevertDataMapT* pRevertData = DYNAMIC_DOWNCAST(CMap<CView*, CView*, int, int>, pHint);
            // TODO Support multiple selections
            RevertDataMapT* pRevertData = dynamic_cast<RevertDataMapT*>(pHint);
            RevertData& data = (*pRevertData)[this];
            data.selStart = GetCtrl().GetSelectionStart();
            data.selEnd = GetCtrl().GetSelectionEnd();
            data.firstVisibleLine = GetCtrl().GetFirstVisibleLine();
        }
        break;

    case HINT_REVERT:
        {
            RevertDataMapT* pRevertData = dynamic_cast<RevertDataMapT*>(pHint);
            const RevertData& data = (*pRevertData)[this];
            GetCtrl().SetSel(data.selStart, data.selEnd);
            GetCtrl().SetFirstVisibleLine(data.firstVisibleLine);
            Invalidate();
        }
        break;

    case HINT_THEME:
        {
            CRadVisualManagerDark* pDark = DYNAMIC_DOWNCAST(CRadVisualManagerDark, CMFCVisualManager::GetInstance());
            CRadVisualManagerDark::Init(&GetCtrl(), pDark != nullptr);
        }
        break;
    }
}

void CRadNotepadView::OnSearchNext()
{
    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    AddSearchTextToHistory(pButton);
    SetFocus();
    OnFindNext(pButton->GetText(), TRUE, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression);
}

void CRadNotepadView::OnSearchTextEditChange()
{
    Scintilla::Position sel = GetCtrl().GetSelectionStart();
    GetCtrl().SetSel(sel, sel);
    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    //AddSearchTextToHistory(pButton);
    OnFindNext(pButton->GetText(), TRUE, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression);
}

void CRadNotepadView::OnUpdateSearchNext(CCmdUI *pCmdUI)
{
    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    pCmdUI->Enable(pButton->GetText()[0] != _T('\0'));
}

void CRadNotepadView::OnSearchPrev()
{
    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    AddSearchTextToHistory(pButton);
    SetFocus();
    OnFindNext(pButton->GetText(), FALSE, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression);
}

void CRadNotepadView::OnUpdateSearchPrev(CCmdUI *pCmdUI)
{
    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    pCmdUI->Enable(pButton->GetText()[0] != _T('\0'));
}

void CRadNotepadView::OnSearchIncremental()
{
    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CMFCToolBarComboBoxButton* pButton = pMainFrame->GetHistoryButton();
    if (pButton != nullptr)
        pButton->GetComboBox()->SetFocus();
}

void CRadNotepadView::OnViewReturn()
{
    SetFocus();
}
