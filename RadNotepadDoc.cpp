
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
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CRadNotepadDoc::OnUpdateFileSave)
END_MESSAGE_MAP()


// CRadNotepadDoc construction/destruction

CRadNotepadDoc::CRadNotepadDoc()
{
    m_ftWrite.dwHighDateTime = 0;
    m_ftWrite.dwLowDateTime = 0;
}

CRadNotepadDoc::~CRadNotepadDoc()
{
}

void CRadNotepadDoc::CheckUpdated()
{
    if (!GetPathName().IsEmpty())
    {
        FILETIME ftWrite = {};
        {
            CFile rFile;
            if (rFile.Open(GetPathName(), CFile::modeRead))
            {
                GetFileTime(rFile, NULL, NULL, &ftWrite);
            }
        }

        if (ftWrite.dwHighDateTime == 0 && ftWrite.dwLowDateTime == 0)
        {
            m_bModified = TRUE;
            SetTitle(nullptr);
        }
        else if (CompareFileTime(&ftWrite, &m_ftWrite) != 0)
        {
            if (/*!IsModified() ||*/ AfxMessageBox(IDS_FILE_REVERT, MB_ICONQUESTION | MB_YESNO) == IDYES)
                OnFileRevert();
            else
            {
                m_ftWrite = ftWrite;
                m_bModified = TRUE;
                SetTitle(nullptr);
            }
        }
    }
}

void CRadNotepadDoc::CheckReadOnly()
{
    if (!GetPathName().IsEmpty())
    {
        CScintillaView* pView = GetView();
        CScintillaCtrl& rCtrl = pView->GetCtrl();

        DWORD dwAttr = GetFileAttributes(GetPathName());
        BOOL bReadOnly = pView->GetUseROFileAttributeDuringLoading() && dwAttr != INVALID_FILE_ATTRIBUTES && dwAttr & FILE_ATTRIBUTE_READONLY;
        if (bReadOnly != rCtrl.GetReadOnly())
        {
            rCtrl.SetReadOnly(bReadOnly);
            SetTitle(nullptr);
        }
    }
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

void CRadNotepadDoc::Serialize(CArchive& ar)
{
    CScintillaView* pView = GetView();
    CScintillaCtrl& rCtrl = pView->GetCtrl();

    if (ar.IsLoading())
    {
        //Tell the control not to maintain any undo info while we stream the data
        rCtrl.Cancel();
        rCtrl.SetUndoCollection(FALSE);

        //Read the data in from the file in blocks
        CFile* pFile = ar.GetFile();
        char Buffer[4096];
        int nBytesRead = 0;
        do
        {
            nBytesRead = pFile->Read(Buffer, 4096);
            if (nBytesRead)
                rCtrl.AddText(nBytesRead, Buffer);
        } while (nBytesRead);

        CheckReadOnly();
        GetFileTime(*pFile, NULL, NULL, &m_ftWrite);

        //Reinitialize the control settings
        rCtrl.SetUndoCollection(TRUE);
        rCtrl.EmptyUndoBuffer();
        rCtrl.SetSavePoint();
        rCtrl.GotoPos(0);
    }
    else
    {
        //Get the length of the document
        int nDocLength = rCtrl.GetLength();

        //Write the data in blocks to disk
        CFile* pFile = ar.GetFile();
        for (int i = 0; i<nDocLength; i += 4095) //4095 because data will be returned nullptr terminated
        {
            int nGrabSize = nDocLength - i;
            if (nGrabSize > 4095)
                nGrabSize = 4095;

            //Get the data from the control
            Sci_TextRange tr;
            tr.chrg.cpMin = i;
            tr.chrg.cpMax = i + nGrabSize;
            char Buffer[4096];
            tr.lpstrText = Buffer;
            rCtrl.GetTextRange(&tr);

            //Write it to disk
            pFile->Write(Buffer, nGrabSize);
        }

        GetFileTime(*pFile, NULL, NULL, &m_ftWrite);
    }
}

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
    CScintillaView* pView = GetView();
    CString strTitle = lpszTitle ? lpszTitle : GetTitle();
    strTitle.Remove(_T('^'));
    strTitle.Remove(_T('*'));
    if (pView->GetCtrl().GetReadOnly())
        strTitle += _T('^');
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


void CRadNotepadDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(IsModified());
}
