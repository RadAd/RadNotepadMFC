
// RadNotepadDoc.cpp : implementation of the CRadNotepadDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "RadNotepad.h"
#endif

#include "RadNotepadDoc.h"

#include <propkey.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CRadNotepadDoc

IMPLEMENT_DYNCREATE(CRadNotepadDoc, CScintillaDoc)

BEGIN_MESSAGE_MAP(CRadNotepadDoc, CScintillaDoc)
    ON_COMMAND(ID_FILE_REVERT, &CRadNotepadDoc::OnFileRevert)
    ON_UPDATE_COMMAND_UI(ID_FILE_REVERT, &CRadNotepadDoc::OnUpdateFileRevert)
END_MESSAGE_MAP()


// CRadNotepadDoc construction/destruction

CRadNotepadDoc::CRadNotepadDoc()
{
	// TODO: add one-time construction code here

}

CRadNotepadDoc::~CRadNotepadDoc()
{
}

BOOL CRadNotepadDoc::OnNewDocument()
{
	if (!CScintillaDoc::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CRadNotepadDoc serialization

#if 0
void CRadNotepadDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}
#endif

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CRadNotepadDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CRadNotepadDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data.
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CRadNotepadDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CRadNotepadDoc diagnostics

#ifdef _DEBUG
void CRadNotepadDoc::AssertValid() const
{
    CScintillaDoc::AssertValid();
}

void CRadNotepadDoc::Dump(CDumpContext& dc) const
{
    CScintillaDoc::Dump(dc);
}
#endif //_DEBUG


// CRadNotepadDoc commands


void CRadNotepadDoc::SetTitle(LPCTSTR lpszTitle)
{
    // TODO: Add your specialized code here and/or call the base class
    CString strTitle = lpszTitle ? lpszTitle : GetTitle();
    strTitle.Remove(_T('^'));
    strTitle.Remove(_T('*'));
#if 0
    // TODO
    if (IsReadOnly())
        strTitle += _T('^');
#endif
    if (IsModified())
        strTitle += _T('*');

    CScintillaDoc::SetTitle(strTitle);
}

void CRadNotepadDoc::SyncModified()
{
    CScintillaView* pView = GetView();
    if (m_bModified != pView->GetCtrl().GetModify())
    {
        m_bModified = pView->GetCtrl().GetModify();
        SetTitle(nullptr);
    }
}


void CRadNotepadDoc::OnFileRevert()
{
    OnOpenDocument(GetPathName());
}


void CRadNotepadDoc::OnUpdateFileRevert(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty() && PathFileExists(GetPathName()));
}
