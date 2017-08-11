
// RadNotepad.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <afxwinappex.h>
#include <afxdialogex.h>

#include "RadNotepad.h"
#include "AboutDlg.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "RadNotepadDoc.h"
#include "RadNotepadView.h"
#include "RadDocManager.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static void PathMakeAbsolute(CString& strFileName)
{
    if (!PathIsRoot(strFileName))
    {
        TCHAR strCurrentDirectory[MAX_PATH];
        TCHAR strTempFileName[MAX_PATH];
        GetCurrentDirectory(ARRAYSIZE(strCurrentDirectory), strCurrentDirectory);
        PathCombine(strTempFileName, strCurrentDirectory, strFileName);
        strFileName = strTempFileName;
    }
}

static BOOL CALLBACK FindRadNotepadProc(_In_ HWND hWnd, _In_ LPARAM lParam)
{
    DWORD_PTR lRet = 0;
    ::SendMessageTimeout(hWnd, WM_RADNOTEPAD, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT | SMTO_BLOCK, 1000, &lRet);
    if (lRet == MSG_RADNOTEPAD)
    {
        HWND* phOther = (HWND*) lParam;
        *phOther = hWnd;
        return FALSE;
    }
    else
        return TRUE;
}

// CRadNotepadApp

BEGIN_MESSAGE_MAP(CRadNotepadApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CRadNotepadApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
    ON_COMMAND(ID_FILE_CLOSEALL, &CRadNotepadApp::OnFileCloseAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEALL, &CRadNotepadApp::OnUpdateFileCloseAll)
    ON_COMMAND(ID_FILE_SAVEALL, &CRadNotepadApp::OnFileSaveAll)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVEALL, &CRadNotepadApp::OnUpdateFileSaveAll)
END_MESSAGE_MAP()


// CRadNotepadApp construction

CRadNotepadApp::CRadNotepadApp()
{
	m_bHiColorIcons = TRUE;
    m_hSciDLL = NULL;

	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("RadNotepad.AppID.NoVersion"));
}

// The one and only CRadNotepadApp object

CRadNotepadApp theApp;


// CRadNotepadApp initialization

BOOL CRadNotepadApp::InitInstance()
{
    HWND hOther = NULL;
    EnumWindows(FindRadNotepadProc, (LPARAM) &hOther);

    if (hOther != NULL)
    {
        CCommandLineInfo cli;
        ParseCommandLine(cli);

        switch (cli.m_nShellCommand)
        {
        case CCommandLineInfo::FileOpen:
            {
                PathMakeAbsolute(cli.m_strFileName);

                HGLOBAL hMem = GlobalAlloc(GHND | GMEM_SHARE, sizeof(DROPFILES) + (cli.m_strFileName.GetLength() + 2) * sizeof(TCHAR));
                DROPFILES* pDropFiles = (DROPFILES*) GlobalLock(hMem);
                pDropFiles->pFiles = (DWORD) ((LPBYTE) (pDropFiles + 1) - (LPBYTE) pDropFiles);
                pDropFiles->fWide = TRUE;
                LPTSTR pDst = (LPTSTR) ((LPBYTE) pDropFiles + pDropFiles->pFiles);
                int Size = (cli.m_strFileName.GetLength() + 1) * sizeof(TCHAR);
                memcpy(pDst, cli.m_strFileName.GetBuffer(), Size);
                pDst = (LPTSTR) ((LPBYTE) pDropFiles + pDropFiles->pFiles + Size);
                *pDst = _T('\0');
                GlobalUnlock(hMem);

                PostMessage(hOther, WM_DROPFILES, (WPARAM) hMem, 0);
            }
            break;
        }

        SetForegroundWindow(hOther);
        return FALSE;
    }

    // InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

    m_hSciDLL = LoadLibrary(_T("SciLexer.dll"));
    if (m_hSciDLL == NULL)
    {
        AfxMessageBox(_T("Scintilla DLL is not installed, Please download the SciTE editor and copy the SciLexer.dll into this application's directory"));
        return FALSE;
    }

    CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	// AfxInitRichEdit2() is required to use RichEdit control
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(10);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

    ASSERT(m_pDocManager == NULL);
    m_pDocManager = new CRadDocManager();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_RadNotepadTYPE,
		RUNTIME_CLASS(CRadNotepadDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CRadNotepadView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
    if (!m_Settings.bEmptyFileOnStartup)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CRadNotepadApp::ExitInstance()
{
	AfxOleTerm(FALSE);

    FreeLibrary(m_hSciDLL);
    m_hSciDLL = NULL;

	return CWinAppEx::ExitInstance();
}

int CRadNotepadApp::GetModifiedDocumentCount() const
{
    int nModified = 0;
    POSITION posTemplate = m_pDocManager->GetFirstDocTemplatePosition();
    while (posTemplate != NULL)
    {
        CDocTemplate* pTemplate = (CDocTemplate*) m_pDocManager->GetNextDocTemplate(posTemplate);
        ASSERT_KINDOF(CDocTemplate, pTemplate);
        {
            POSITION pos = pTemplate->GetFirstDocPosition();
            while (pos != NULL)
            {
                CDocument* pDoc = pTemplate->GetNextDoc(pos);
                if (pDoc->IsModified())
                    ++nModified;
            }
        }
    }
    return nModified;
}

// CRadNotepadApp message handlers


// CRadNotepadApp customization load/save methods

void CRadNotepadApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CRadNotepadApp::LoadCustomState()
{
}

void CRadNotepadApp::SaveCustomState()
{
}

// CRadNotepadApp message handlers

BOOL CRadNotepadApp::SaveAllModified()
{
    //return CWinAppEx::SaveAllModified();
    int nModified = GetModifiedDocumentCount();

    if (nModified == 1)
        return CWinAppEx::SaveAllModified();
    else if (nModified > 0)
    {
        // TODO Need a better dialog
        CMainFrame* pMainWnd = (CMainFrame*) m_pMainWnd;
        return pMainWnd->DoWindowsDialog() == IDOK;
    }
    else
        return TRUE;
}


void CRadNotepadApp::OnFileCloseAll()
{
    CloseAllDocuments(FALSE);
}

void CRadNotepadApp::OnUpdateFileCloseAll(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(m_pDocManager->GetOpenDocumentCount() > 0);
}

void CRadNotepadApp::OnFileSaveAll()
{
    POSITION posTemplate = m_pDocManager->GetFirstDocTemplatePosition();
    while (posTemplate != NULL)
    {
        CDocTemplate* pTemplate = (CDocTemplate*) m_pDocManager->GetNextDocTemplate(posTemplate);
        ASSERT_KINDOF(CDocTemplate, pTemplate);
        {
            POSITION pos = pTemplate->GetFirstDocPosition();
            while (pos != NULL)
            {
                CDocument* pDoc = pTemplate->GetNextDoc(pos);
                if (pDoc->IsModified())
                    pDoc->DoFileSave();
            }
        }
    }
}

void CRadNotepadApp::OnUpdateFileSaveAll(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(GetModifiedDocumentCount() > 0);
}
