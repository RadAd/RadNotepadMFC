
// RadNotepadDoc.cpp : implementation of the CRadNotepadDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "RadNotepad.h"
#endif

#include "RadNotepadDoc.h"
#include "RadWaitCursor.h"

#include <propkey.h>
#include <afxinet.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static std::unique_ptr<CInternetSession> g_pInternetSession;

static CString GetSourceUrlName(LPCTSTR lpszFileName)
{
    CString strSourceUrlName;

    std::vector<BYTE> data;
    LPINTERNET_CACHE_ENTRY_INFO pCacheInfo = nullptr;
    DWORD size = 1024;
    HANDLE hFind = NULL;

    do
    {
        data.resize(size / sizeof(BYTE));
        pCacheInfo = (LPINTERNET_CACHE_ENTRY_INFO) data.data();
        pCacheInfo->dwStructSize = sizeof(INTERNET_CACHE_ENTRY_INFO);
        size = (DWORD) (data.size() * sizeof(BYTE));
        hFind = FindFirstUrlCacheEntry(nullptr, pCacheInfo, &size);
    } while (hFind == NULL && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    if (hFind != NULL)
    {
        BOOL b = FALSE;
        do
        {
            if (pCacheInfo->lpszLocalFileName != nullptr && wcscmp(pCacheInfo->lpszLocalFileName, lpszFileName) == 0)
            {
                strSourceUrlName = pCacheInfo->lpszSourceUrlName;
                break;
            }

            size = (DWORD) (data.size() * sizeof(BYTE));

            do
            {
                data.resize(size / sizeof(BYTE));
                pCacheInfo = (LPINTERNET_CACHE_ENTRY_INFO) data.data();
                size = (DWORD) (data.size() * sizeof(BYTE));
                b = FindNextUrlCacheEntry(hFind, pCacheInfo, &size);
            } while (!b && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
        }
        while (b);

        FindCloseUrlCache(hFind);
    }
    ASSERT(GetLastError() == ERROR_SUCCESS);

    return strSourceUrlName;
}

static const Encoding g_EncodingList[] = { Encoding::BOM_ANSI, Encoding::BOM_UTF16_LE, Encoding::BOM_UTF16_BE, Encoding::BOM_UTF8 };

static BYTE g_BomBytes[][4] = {
    { 0 },  // BOM_ANSI
    { u'\xFF', u'\xFE', 0 }, // BOM_UTF16_LE
    { u'\xFE', u'\xFF', 0 }, // BOM_UTF16_BE
    { u'\xEF', u'\xBB', u'\xBF', 0 }, // BOM_UTF8
};

static void byteswap16(unsigned short* data, size_t count)
{
    for (size_t i = 0; i < count; ++i)
        data[i] = _byteswap_ushort(data[i]);
}

static UINT GetBomLength(Encoding eEncoding)
{
    PBYTE pBomData = g_BomBytes[to_underlying(eEncoding)];
    int i = 0;
    for (i = 0; pBomData[i] != 0; ++i)
        ;
    return i;
}

static BOOL IsBom(PBYTE pData, PBYTE pBomData)
{
    int i = 0;
    for (i = 0; pBomData[i] != 0; ++i)
        if (pBomData[i] != pData[i])
            break;
    return (pBomData[i] == 0);
}

static Encoding CheckBom(PBYTE pData)
{
    for (Encoding eEncoding : g_EncodingList)
    {
        if (g_BomBytes[to_underlying(eEncoding)][0] != 0 && IsBom(pData, g_BomBytes[to_underlying(eEncoding)]))
            return eEncoding;
    }
    return Encoding::BOM_ANSI;
}

static Scintilla::EndOfLine GetLineEndingMode(Scintilla::CScintillaCtrl& rCtrl, int nLine, Scintilla::EndOfLine def)
{
    CString line = rCtrl.GetLine(nLine);
    if (line.Right(2) == _T("\r\n"))
        return Scintilla::EndOfLine::CrLf;
    else if (line.Right(1) == _T("\n"))
        return Scintilla::EndOfLine::Lf;
    else if (line.Right(1) == _T("\r"))
        return Scintilla::EndOfLine::Cr;
    else
        return def;
}

// CRadNotepadDoc

IMPLEMENT_DYNCREATE(CRadNotepadDoc, CScintillaDoc)

BEGIN_MESSAGE_MAP(CRadNotepadDoc, CScintillaDoc)
    ON_COMMAND(ID_FILE_REVERT, &CRadNotepadDoc::OnFileRevert)
    ON_UPDATE_COMMAND_UI(ID_FILE_REVERT, &CRadNotepadDoc::OnUpdateFileRevert)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CRadNotepadDoc::OnUpdateFileSave)
    ON_COMMAND(ID_FILE_READONLY, &CRadNotepadDoc::OnFileReadOnly)
    ON_UPDATE_COMMAND_UI(ID_FILE_READONLY, &CRadNotepadDoc::OnUpdateFileReadOnly)
    ON_COMMAND_RANGE(ID_ENCODING_ANSI, ID_ENCODING_UTF8, &CRadNotepadDoc::OnEncoding)
    ON_UPDATE_COMMAND_UI_RANGE(ID_ENCODING_ANSI, ID_ENCODING_UTF8, &CRadNotepadDoc::OnUpdateEncoding)
END_MESSAGE_MAP()


// CRadNotepadDoc construction/destruction

CRadNotepadDoc::CRadNotepadDoc()
    : m_eEncoding(Encoding::BOM_ANSI)
    , m_nLineEndingMode(Scintilla::EndOfLine::CrLf)
    , m_ftWrite {}

{
}

CRadNotepadDoc::~CRadNotepadDoc()
{
}

void CRadNotepadDoc::CheckUpdated()
{
    if (!GetPathName().IsEmpty() && !CRadNotepadApp::IsInternetUrl(GetPathName()))
    {
        FILETIME ftWrite = {};
        {
            CFile rFile;
            if (rFile.Open(GetPathName(), CFile::modeRead | CFile::shareDenyNone))
                GetFileTime(rFile, NULL, NULL, &ftWrite);
        }

        if (ftWrite.dwHighDateTime == 0 && ftWrite.dwLowDateTime == 0)
        {
            m_bModified = TRUE;
            SetTitle(nullptr);
        }
        else if (CompareFileTime(&ftWrite, &m_ftWrite) != 0)
        {
            m_ftWrite = ftWrite;    // The message box causes another activation, causing an infinite loop
            if (/*!IsModified() ||*/ AfxMessageBox(IDS_FILE_REVERT, MB_ICONQUESTION | MB_YESNO) == IDYES)
                OnFileRevert();
            else
            {
                m_bModified = TRUE;
                SetTitle(nullptr);
            }
        }
    }
}

void CRadNotepadDoc::CheckReadOnly()
{
    Scintilla::CScintillaView* pView = GetView();
    Scintilla::CScintillaCtrl& rCtrl = pView->GetCtrl();

    BOOL bReadOnly = FALSE;
    if (GetTitle().Left(5) == _T("temp:"))
        bReadOnly = TRUE;
    else if (CRadNotepadApp::IsInternetUrl(GetPathName()))
        bReadOnly = TRUE;
    else if (!GetPathName().IsEmpty())
    {
        DWORD dwAttr = GetFileAttributes(GetPathName());
        bReadOnly = pView->GetUseROFileAttributeDuringLoading() && dwAttr != INVALID_FILE_ATTRIBUTES && dwAttr & FILE_ATTRIBUTE_READONLY;
    }

    if (bReadOnly != rCtrl.GetReadOnly())
    {
        rCtrl.SetReadOnly(bReadOnly);
        SetTitle(nullptr);
    }
}

BOOL CRadNotepadDoc::OnNewDocument()
{
	if (!CScintillaDoc::OnNewDocument())
		return FALSE;

    m_eEncoding = theApp.m_Settings.DefaultEncoding;
    m_nLineEndingMode = static_cast<Scintilla::EndOfLine>(theApp.m_Settings.DefaultLineEnding);
    m_ftWrite.dwHighDateTime = 0;
    m_ftWrite.dwLowDateTime = 0;

	return TRUE;
}

BOOL CRadNotepadDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    m_eEncoding = theApp.m_Settings.DefaultEncoding;

    if (CRadNotepadApp::IsInternetUrl(lpszPathName) || PathFileExists(lpszPathName))
        return CScintillaDoc::OnOpenDocument(lpszPathName);
    else
        return TRUE; // Allow opening non-existent files
}

// CRadNotepadDoc serialization

static void AddText(Scintilla::CScintillaCtrl& rCtrl, LPBYTE Buffer, int nBytesRead, Encoding eEncoding)
{
    // TODO Check/Handle when nBytesRead is not an exact multiple
    if (nBytesRead > 0)
    {
        switch (eEncoding)
        {
        case Encoding::BOM_ANSI:
        case Encoding::BOM_UTF8:
            rCtrl.AddText(nBytesRead / sizeof(char), reinterpret_cast<char*>(Buffer));
            ASSERT(nBytesRead % sizeof(char) == 0);
            break;

        case Encoding::BOM_UTF16_BE:
            byteswap16(reinterpret_cast<unsigned short*>(Buffer), nBytesRead / sizeof(unsigned short));
            // fallthrough
        case Encoding::BOM_UTF16_LE:
            rCtrl.AddText(nBytesRead / sizeof(wchar_t), reinterpret_cast<wchar_t*>(Buffer));
            ASSERT(nBytesRead % sizeof(wchar_t) == 0);
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }
}

static void WriteBom(CFile* pFile, Encoding eEncoding)
{
    UINT nLen = GetBomLength(eEncoding);
    if (nLen > 0)
        pFile->Write(g_BomBytes[to_underlying(eEncoding)], GetBomLength(eEncoding));
}

static void WriteText(CFile* pFile, const char* Buffer, int nBytes, Encoding eEncoding)
{
    if (nBytes > 0)
    {
        switch (eEncoding)
        {
        case Encoding::BOM_ANSI:
        case Encoding::BOM_UTF8:
            pFile->Write(Buffer, nBytes);
            break;

        case Encoding::BOM_UTF16_BE:
        case Encoding::BOM_UTF16_LE:
            {
                CStringW s = Scintilla::CScintillaCtrl::UTF82W(Buffer, nBytes);
                if (eEncoding == Encoding::BOM_UTF16_BE)
                {
                    int nLen = s.GetLength();
                    byteswap16(reinterpret_cast<unsigned short*>(s.GetBuffer()), nLen);
                    s.ReleaseBuffer(nLen);
                }
                pFile->Write(s, s.GetLength() * sizeof(WCHAR));
            }
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }
}

void CRadNotepadDoc::Serialize(CArchive& ar)
{
    Scintilla::CScintillaCtrl& rCtrl = GetView()->GetCtrl();
    CRadWaitCursor wc;
    CMFCStatusBar* pMsgWnd = wc.GetStatusBar();
    const int nIndex = 0;
    pMsgWnd->EnablePaneProgressBar(nIndex, 100, TRUE);

    const int BUFSIZE = 4 * 1024;

    if (ar.IsLoading())
    {
        rCtrl.SetRedraw(FALSE);

        //Tell the control not to maintain any undo info while we stream the data
        rCtrl.Cancel();
        rCtrl.SetUndoCollection(FALSE);

        //Read the data in from the file in blocks
        CFile* pFile = ar.GetFile();
        ULONGLONG nLength = pFile->GetLength();
        CHttpFile* pHttpFile = DYNAMIC_DOWNCAST(CHttpFile, pFile);
        if (pHttpFile != nullptr)
        {   // GetLength for CHttpFile returns how much can be read, not length  of file
            DWORD dwLength = 0;
            pHttpFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, dwLength);
            nLength = dwLength;
        }
        ULONGLONG nBytesReadTotal = 0;

        BYTE Buffer[BUFSIZE];
        int nBytesRead = 0;
        bool bFirst = true;
        do
        {
            nBytesRead = pFile->Read(Buffer, BUFSIZE);
            if (bFirst)
            {
                ASSERT(nBytesRead >= 3);
                bFirst = false;
                m_eEncoding = CheckBom(Buffer);
                UINT nLen = GetBomLength(m_eEncoding);
                AddText(rCtrl, Buffer + nLen, nBytesRead - nLen, m_eEncoding);
            }
            else
                AddText(rCtrl, Buffer, nBytesRead, m_eEncoding);
            nBytesReadTotal += nBytesRead;
            if (nLength > 0)
                pMsgWnd->SetPaneProgress(nIndex, static_cast<long>(nBytesReadTotal * 100 / nLength));

            MSG msg;
            while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
            {
                if (!::AfxPumpMessage())
                    break;
            }
        } while (nBytesRead);

        CheckReadOnly();
        m_nLineEndingMode = ::GetLineEndingMode(rCtrl, 0, m_nLineEndingMode);

        //Reinitialize the control settings
        rCtrl.SetUndoCollection(TRUE);
        rCtrl.EmptyUndoBuffer();
        rCtrl.SetSavePoint();
        rCtrl.GotoPos(0);

        rCtrl.SetRedraw(TRUE);
    }
    else
    {
        //Get the length of the document
        Scintilla::Position nDocLength = rCtrl.GetLength();
        ULONGLONG nBytesWriteTotal = 0;

        //Write the data in blocks to disk
        CFile* pFile = ar.GetFile();
        WriteBom(pFile, m_eEncoding);
        for (int i = 0; i < nDocLength; i += (BUFSIZE - 1)) // (BUFSIZE - 1) because data will be returned nullptr terminated
        {
            Scintilla::Position nGrabSize = nDocLength - i;
            if (nGrabSize > (BUFSIZE - 1))
                nGrabSize = (BUFSIZE - 1);

            //Get the data from the control
            Scintilla::TextRange tr;
            tr.chrg.cpMin = i;
            tr.chrg.cpMax = static_cast<Scintilla::PositionCR>(i + nGrabSize);
            char Buffer[BUFSIZE];
            tr.lpstrText = Buffer;
            rCtrl.GetTextRange(&tr);

            //Write it to disk
            WriteText(pFile, Buffer, (int) nGrabSize, m_eEncoding);
            nBytesWriteTotal += nGrabSize;
            pMsgWnd->SetPaneProgress(nIndex, static_cast<long>(nBytesWriteTotal * 100 / nDocLength));

            MSG msg;
            while (::PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
            {
                if (!::AfxPumpMessage())
                    break;
            }
        }
    }
    pMsgWnd->EnablePaneProgressBar(nIndex, -1);
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
    Scintilla::CScintillaView* pView = GetView();
    CString strTitle = lpszTitle ? lpszTitle : GetTitle();
    strTitle.Remove(_T('^'));
    strTitle.Remove(_T('*'));
    if (pView->GetCtrl().GetReadOnly())
        strTitle += _T('^');
    if (IsModified())
        strTitle += _T('*');

    CScintillaDoc::SetTitle(strTitle);
}

void CRadNotepadDoc::SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU)
{
    const bool bPathNameFirstSet = GetPathName().IsEmpty();

    if (!CRadNotepadApp::IsInternetUrl(lpszPathName))
    {
        CString strSourceUrlName = GetSourceUrlName(lpszPathName);

        if (strSourceUrlName.IsEmpty())
        {
            CScintillaDoc::SetPathName(lpszPathName, bAddToMRU && PathFileExists(lpszPathName));

            CFile rFile;
            if (rFile.Open(GetPathName(), CFile::modeRead | CFile::shareDenyNone))
                GetFileTime(rFile, NULL, NULL, &m_ftWrite);
        }
        else
        {
            //m_strPathName = strSourceUrlName;

            //ASSERT(!m_strPathName.IsEmpty());       // must be set to something
            m_bEmbedded = FALSE;
            ASSERT_VALID(this);

            SetTitle(strSourceUrlName);

            bAddToMRU = FALSE;

            ASSERT_VALID(this);
        }
    }
    else
    {
        m_strPathName = lpszPathName;

        ASSERT(!m_strPathName.IsEmpty());       // must be set to something
        m_bEmbedded = FALSE;
        ASSERT_VALID(this);

        DWORD dwServiceType;
        CString strServer;
        CString strObject;
        INTERNET_PORT nPort;
        AfxParseURL(m_strPathName, dwServiceType, strServer, strObject, nPort);

        int i = strObject.ReverseFind('/');
        CString strTitle;
        if (i >= 0)
            strTitle = strObject.Mid(i + 1);
        if (strTitle.IsEmpty())
            strTitle = _T("index");
        SetTitle(strTitle);

        // add it to the file MRU list
        if (bAddToMRU)
            AfxGetApp()->AddToRecentFileList(m_strPathName);

        ASSERT_VALID(this);
    }

    if (bPathNameFirstSet)
        UpdateAllViews(nullptr, HINT_PATH_UPDATED);
}

CFile* CRadNotepadDoc::GetFile(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError)
{
    if (!CRadNotepadApp::IsInternetUrl(lpszFileName))
    {
        if (nOpenFlags & CFile::shareDenyWrite)
        {   // Allow reading locked files
            nOpenFlags &= ~CFile::shareDenyWrite;
            nOpenFlags |= CFile::shareDenyNone;
        }
        return CScintillaDoc::GetFile(lpszFileName, nOpenFlags, pError);
    }
    else
    {
        if (g_pInternetSession == nullptr)
            g_pInternetSession = std::make_unique<CInternetSession>();

        CStdioFile* pFile = g_pInternetSession->OpenURL(lpszFileName, 1, INTERNET_FLAG_TRANSFER_BINARY);

#if 0   // TODO May need to use these
        CHttpFile* pHttpFile = DYNAMIC_DOWNCAST(CHttpFile, pFile);
        if (pHttpFile != nullptr)
        {
            CString encoding;
            pHttpFile->QueryInfo(HTTP_QUERY_CONTENT_ENCODING, encoding);

            CString disposition;
            pHttpFile->QueryInfo(HTTP_QUERY_CONTENT_DISPOSITION, disposition);
        }
#endif

        return pFile;
    }
}

void CRadNotepadDoc::SyncModified()
{
    Scintilla::CScintillaView* pView = GetView();
    if (m_bModified != pView->GetCtrl().GetModify())
    {
        m_bModified = pView->GetCtrl().GetModify();
        SetTitle(nullptr);
    }
}


void CRadNotepadDoc::OnFileRevert()
{
    RevertDataMapT revertData;
    UpdateAllViews(nullptr, HINT_REVERT_PRE, &revertData);
    OnOpenDocument(GetPathName());
    UpdateAllViews(nullptr, HINT_REVERT, &revertData);
}


void CRadNotepadDoc::OnUpdateFileRevert(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(!GetPathName().IsEmpty() && (CRadNotepadApp::IsInternetUrl(GetPathName()) || PathFileExists(GetPathName())));
}


void CRadNotepadDoc::OnUpdateFileSave(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(IsModified());
}


void CRadNotepadDoc::OnFileReadOnly()
{
    if (!GetPathName().IsEmpty())
    {
        DWORD dwAttr = GetFileAttributes(GetPathName());
        if (dwAttr != INVALID_FILE_ATTRIBUTES)
        {
            if (dwAttr & FILE_ATTRIBUTE_READONLY)
                dwAttr &= ~FILE_ATTRIBUTE_READONLY;
            else
                dwAttr |= FILE_ATTRIBUTE_READONLY;
            SetFileAttributes(GetPathName(), dwAttr);
            CheckReadOnly();
        }
    }
}


void CRadNotepadDoc::OnUpdateFileReadOnly(CCmdUI *pCmdUI)
{
    Scintilla::CScintillaView* pView = GetView();
    Scintilla::CScintillaCtrl& rCtrl = pView->GetCtrl();
    pCmdUI->Enable(!GetPathName().IsEmpty());
    pCmdUI->SetCheck(rCtrl.GetReadOnly());
}


void CRadNotepadDoc::OnEncoding(UINT nID)
{
    Encoding e = static_cast<Encoding>(nID - ID_ENCODING_ANSI);
    if (m_eEncoding != e)
    {
        m_eEncoding = e;
        m_bModified = TRUE;
        SetTitle(nullptr);
    }
}

void CRadNotepadDoc::OnUpdateEncoding(CCmdUI *pCmdUI)
{
    Encoding e = static_cast<Encoding>(pCmdUI->m_nID - ID_ENCODING_ANSI);
    pCmdUI->SetCheck(e == m_eEncoding);
}
