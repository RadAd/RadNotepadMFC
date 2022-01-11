
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
#include "RadUserTool.h"
#include "RadRecentFileList.h"

#include <afxinet.h>

#include <algorithm>

static std::vector<ACCEL> CopyAcceleratorTable(HACCEL hAccel)
{
    std::vector<ACCEL> vecAccel(::CopyAcceleratorTable(hAccel, NULL, 0));
    ::CopyAcceleratorTable(hAccel, vecAccel.data(), (int) vecAccel.size());
    return vecAccel;
}

static std::vector<ACCEL> CopyAcceleratorTable(UINT nIdResources)
{
    HINSTANCE hInst = AfxFindResourceHandle(ATL_MAKEINTRESOURCE(nIdResources), ATL_RT_ACCELERATOR);
    HACCEL hAccel = ::LoadAccelerators(hInst, ATL_MAKEINTRESOURCE(nIdResources));
    return CopyAcceleratorTable(hAccel);
}

std::vector<ACCEL> LoadAccelFromRegistry()
{
    std::vector<ACCEL> vecAccel;

    CString strProfileName = theApp.GetRegSectionPath();
    LPCTSTR AFX_REG_SECTION_FMT = _T("%TsKeyboard-%d");
    LPCTSTR AFX_REG_ENTRY_DATA = _T("DefaultAccelerators");
    UINT uiResId = 0;

    CString strSection;
    strSection.Format(AFX_REG_SECTION_FMT, strProfileName.GetString(), uiResId);

    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, FALSE);

    if (reg.Open(strSection))
    {
        UINT uiSize;
        LPACCEL lpAccel;

        if (reg.Read(AFX_REG_ENTRY_DATA, (LPBYTE*) &lpAccel, &uiSize))
        {
            int nAccelSize = uiSize / sizeof(ACCEL);
            ENSURE(lpAccel != NULL);

            vecAccel.resize(nAccelSize);
            std::copy(lpAccel, lpAccel + nAccelSize, vecAccel.data());
        }

        delete[] lpAccel;
    }

    return vecAccel;
}

void SaveAccelToRegistry(const std::vector<ACCEL>& vecAccel)
{
    CString strProfileName = theApp.GetRegSectionPath();
    LPCTSTR AFX_REG_SECTION_FMT = _T("%TsKeyboard-%d");
    LPCTSTR AFX_REG_ENTRY_DATA = _T("DefaultAccelerators");
    UINT uiResId = 0;

    CString strSection;
    strSection.Format(AFX_REG_SECTION_FMT, strProfileName.GetString(), uiResId);

    CSettingsStoreSP regSP;
    CSettingsStore& reg = regSP.Create(FALSE, FALSE);

    if (reg.CreateKey(strSection))
    {
        reg.Write(AFX_REG_ENTRY_DATA, (LPBYTE) vecAccel.data(), (UINT) (vecAccel.size() * sizeof(ACCEL)));
    }
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static void PathMakeAbsolute(CString& strFileName)
{
    // TODO Should I use AfxFullPath?
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
    INT nCloaked = 0;
    DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &nCloaked, sizeof(INT));
    if (nCloaked)
        return TRUE;

    TCHAR Class[1024];
    DWORD_PTR lRet = 0;
    ::GetClassName(hWnd, Class, ARRAYSIZE(Class));
    if (wcsncmp(Class, _T("Afx:"), 4) == 0)
        ::SendMessageTimeout(hWnd, WM_RADNOTEPAD, 0, 0, SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT | SMTO_BLOCK, 100, &lRet);
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
    ON_COMMAND(ID_FILE_CLOSEOTHERS, &CRadNotepadApp::OnFileCloseOthers)
    ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEOTHERS, &CRadNotepadApp::OnUpdateFileCloseOthers)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SAVESETTINGSONEXIT, &CRadNotepadApp::OnUpdateViewSaveSettingsOnExit)
    ON_COMMAND(ID_VIEW_SAVESETTINGSONEXIT, &CRadNotepadApp::OnViewSaveSettingsOnExit)
END_MESSAGE_MAP()


// CRadNotepadApp construction

CRadNotepadApp::CRadNotepadApp()
{
    m_hSciDLL = NULL;
    m_SaveSettings = FALSE;

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

BOOL CRadNotepadApp::IsInternetUrl(LPCTSTR lpszFileName)
{
    DWORD dwServiceType;
    CString strServer;
    CString strObject;
    INTERNET_PORT nPort;
    AfxParseURL(lpszFileName, dwServiceType, strServer, strObject, nPort);

    return dwServiceType != AFX_INET_SERVICE_UNK && dwServiceType != AFX_INET_SERVICE_FILE;
}

class CRadCommandLineInfo : public CCommandLineInfo
{
public:
    // Sets default values
    CRadCommandLineInfo()
        : m_bNewWindow(FALSE)
    {
    }

    // plain char* version on UNICODE for source-code backwards compatibility
    virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override
    {
        if (bFlag)
        {
            const CStringA strParam(pszParam);
            ParseParamFlag(strParam.GetString());
        }
        else
            ParseParamNotFlag(pszParam);

        ParseLast(bLast);
    }
#ifdef _UNICODE
    virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast) override
    {
        if (bFlag)
            ParseParamFlag(pszParam);
        else
            ParseParamNotFlag(pszParam);

        ParseLast(bLast);
    }

    BOOL m_bNewWindow;

#endif
protected:
    void ParseParamFlag(const char* pszParam)
    {
        CCommandLineInfo::ParseParamFlag(pszParam);

        if (lstrcmpiA(pszParam, "NewWindow") == 0)
            m_bNewWindow = TRUE;
    }
};

// CRadNotepadApp initialization

BOOL CRadNotepadApp::InitInstance()
{
    m_bSaveState = FALSE;
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // Parse command line for standard shell commands, DDE, file open
    CRadCommandLineInfo cmdInfo;
    if (!m_Settings.bEmptyFileOnStartup)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
    ParseCommandLine(cmdInfo);

    if (!cmdInfo.m_bNewWindow && cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen && !(GetKeyState(VK_SHIFT) & KF_UP))
    {
        HWND hOther = NULL;
        EnumWindows(FindRadNotepadProc, (LPARAM) &hOther);

        if (hOther != NULL)
        {
#if 0
            CCommandLineInfo cmdInfo;
            ParseCommandLine(cmdInfo);
#endif
            if (!IsInternetUrl(cmdInfo.m_strFileName))
                PathMakeAbsolute(cmdInfo.m_strFileName);

            HGLOBAL hMem = GlobalAlloc(GHND | GMEM_SHARE, sizeof(DROPFILES) + (cmdInfo.m_strFileName.GetLength() + 2) * sizeof(TCHAR));
            DROPFILES* pDropFiles = (DROPFILES*) GlobalLock(hMem);
            pDropFiles->pFiles = (DWORD) ((LPBYTE) (pDropFiles + 1) - (LPBYTE) pDropFiles);
            pDropFiles->fWide = TRUE;
            LPTSTR pDst = (LPTSTR) ((LPBYTE) pDropFiles + pDropFiles->pFiles);
            int Size = (cmdInfo.m_strFileName.GetLength() + 1) * sizeof(TCHAR);
            memcpy(pDst, cmdInfo.m_strFileName.GetBuffer(), Size);
            pDst = (LPTSTR) ((LPBYTE) pDropFiles + pDropFiles->pFiles + Size);
            *pDst = _T('\0');
            GlobalUnlock(hMem);

            PostMessage(hOther, WM_DROPFILES, (WPARAM) hMem, 0);

            SetForegroundWindow(hOther);
            return FALSE;
        }
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

    CoInitialize(0);

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

    AfxEnableControlContainer();

	// AfxInitRichEdit2() is required to use RichEdit control
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
#ifdef _DEBUG
	SetRegistryKey(_T("RadSoftDebug"));
#else
    SetRegistryKey(_T("RadSoft"));
#endif
    LoadStdProfileSettings(m_Settings.nMaxMRU);  // Load standard INI file options (including MRU)

    ASSERT(m_pRecentFileList != NULL);
    CRecentFileList* pRecentFileList = new CRadRecentFileList(*m_pRecentFileList);
    m_pRecentFileList->m_arrNames = nullptr;
    delete m_pRecentFileList;
    m_pRecentFileList = pRecentFileList;

    InitContextMenuManager();
	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

    EnableUserTools(ID_TOOLS, ID_TOOLS_FIRSTTOOL, ID_TOOLS_LASTTOOL, RUNTIME_CLASS(CRadUserTool), IDR_TOOL_FIELDS, IDR_TOOL_FIELDS);
    EnableTaskbarInteraction(GetProfileInt(_T("Settings"), _T("EnableTaskbarInteraction"), 0));

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

    std::vector<ACCEL> vecOldAccel = LoadAccelFromRegistry();

    if (!vecOldAccel.empty())
    {
        const auto fAccelCmp = [](const ACCEL& rAccelL, const ACCEL& rAccelR) {
            if (rAccelL.key != rAccelR.key)
                return rAccelL.key < rAccelR.key;
            else if (rAccelL.fVirt != rAccelR.fVirt)
                return rAccelL.fVirt < rAccelR.fVirt;
            else //if (rAccelL.cmd != rAccelR.cmd)
                return rAccelL.cmd < rAccelR.cmd;
        };
        const auto fAccelKeyCmp = [](const ACCEL& rAccelL, const ACCEL& rAccelR) {
            if (rAccelL.key != rAccelR.key)
                return rAccelL.key < rAccelR.key;
            else //if (rAccelL.fVirt != rAccelR.fVirt)
                return rAccelL.fVirt < rAccelR.fVirt;
        };
        const auto fAccelKeyEq = [](const ACCEL& rAccelL, const ACCEL& rAccelR) {
            return (rAccelL.key == rAccelR.key)
                && (rAccelL.fVirt == rAccelR.fVirt);
        };


        std::sort(vecOldAccel.begin(), vecOldAccel.end(), fAccelCmp);

        std::vector<ACCEL> vecOrigAccel = ::CopyAcceleratorTable(IDR_MAINFRAME);
        std::sort(vecOrigAccel.begin(), vecOrigAccel.end(), fAccelCmp);
        ASSERT(std::adjacent_find(vecOrigAccel.begin(), vecOrigAccel.end(), fAccelKeyEq) == vecOrigAccel.end());

        std::vector<ACCEL> vecRemoved;
        std::vector<ACCEL> vecAdded;

        {
            auto itOld = vecOldAccel.begin();
            auto itOrig = vecOrigAccel.begin();
            while (itOld != vecOldAccel.end() && itOrig != vecOrigAccel.end())
            {
                if (fAccelCmp(*itOld, *itOrig))
                {
                    vecRemoved.push_back(*itOld);
                    ++itOld;
                }
                else if (fAccelCmp(*itOrig, *itOld))
                {
                    vecAdded.push_back(*itOrig);
                    ++itOrig;
                }
                else
                {
                    ++itOld;
                    ++itOrig;
                }
            }
            while (itOld != vecOldAccel.end())
            {
                vecRemoved.push_back(*itOld);
                ++itOld;
            }
            while (itOrig != vecOrigAccel.end())
            {
                vecAdded.push_back(*itOrig);
                ++itOrig;
            }
        }

        if (!vecRemoved.empty() || !vecAdded.empty())
        {
            std::vector<ACCEL> vecCurrentAccel = ::CopyAcceleratorTable(pMainFrame->m_hAccelTable);
            std::sort(vecCurrentAccel.begin(), vecCurrentAccel.end(), fAccelCmp);
            bool fModified = false;

            for (const ACCEL& rAccel : vecRemoved)
            {
                auto it = std::lower_bound(vecCurrentAccel.begin(), vecCurrentAccel.end(), rAccel, fAccelCmp);
                if (it != vecCurrentAccel.end())
                {
                    fModified = true;
                    vecCurrentAccel.erase(it);
                }
            }
            for (const ACCEL& rAccel : vecAdded)
            {
                auto it = std::lower_bound(vecCurrentAccel.begin(), vecCurrentAccel.end(), rAccel, fAccelKeyCmp);
                if (it == vecCurrentAccel.end() || !fAccelKeyEq(*it, rAccel))
                {
                    fModified = true;
                    vecCurrentAccel.push_back(rAccel);
                }
            }

            if (fModified)
            {
                ::DestroyAcceleratorTable(pMainFrame->m_hAccelTable);
                pMainFrame->m_hAccelTable = ::CreateAcceleratorTable(vecCurrentAccel.data(), (int) vecCurrentAccel.size());
            }
        }
    }

#if 0
    // Parse command line for standard shell commands, DDE, file open
    CCommandLineInfo cmdInfo;
    if (!m_Settings.bEmptyFileOnStartup)
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
    ParseCommandLine(cmdInfo);
#endif

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

void CRadNotepadApp::NotifySettingsChanged()
{
    m_SaveSettings = TRUE;
    CRadDocManager* pRadDocManager = DYNAMIC_DOWNCAST(CRadDocManager, m_pDocManager);
    pRadDocManager->UpdateAllViews(nullptr, HINT_UPDATE_SETTINGS);

    CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, m_pMainWnd);
    pMainFrame->NotifySettingsChanged();
}

// CRadNotepadApp message handlers


// CRadNotepadApp customization load/save methods

BOOL CRadNotepadApp::SaveState(LPCTSTR lpszSectionName, CFrameImpl* pFrameImpl)
{
    if (!m_bSaveState && m_SaveSettings)
    {
        m_bSaveState = AfxMessageBox(L"Properties have changed. Do you wish to save?.", MB_YESNO | MB_ICONWARNING) == IDYES;
    }

    if (!m_bSaveState)
    {
        CMainFrame* pMainFrame = DYNAMIC_DOWNCAST(CMainFrame, m_pMainWnd);
        CString strSection = GetRegSectionPath();
        pMainFrame->SaveSearch(strSection);
        return TRUE;
    }
    else
    {
        std::vector<ACCEL> vecAccel = ::CopyAcceleratorTable(IDR_MAINFRAME);
        //std::sort(vecAccel.begin(), vecAccel.end(), fAccelCmp);
        SaveAccelToRegistry(vecAccel);

        return CWinAppEx::SaveState(lpszSectionName, pFrameImpl);
    }
}

void CRadNotepadApp::PreLoadState()
{
    GetContextMenuManager()->AddMenu(IDS_EDIT_MENU, IDR_POPUP_EDIT);
    GetContextMenuManager()->AddMenu(IDS_TAB_MENU, IDR_POPUP_TAB);
}

void CRadNotepadApp::LoadCustomState()
{
    // TODO Load settings
    LoadTheme(&m_Settings.user, &m_Settings.default);
}

void CRadNotepadApp::SaveCustomState()
{
    // TODO Save settings
    if (m_SaveSettings)
        SaveTheme(&m_Settings.user, &m_Settings.default);
}

CDocument* CRadNotepadApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
    return OpenDocumentFile(lpszFileName, TRUE);
}

CDocument* CRadNotepadApp::OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU)
{
    CRadNotepadDoc* ret = nullptr;

    const wchar_t* p = _tcsrchr(lpszFileName, _T(':'));
    int line = p != nullptr ? _tstoi(p + 1) - 1 : -1;
    if (line > 0)
    {
        size_t len = (p - lpszFileName);
        TCHAR szFileName[MAX_PATH];
        _tcsncpy_s(szFileName, lpszFileName, len);
        szFileName[len] = _T('\0');

        ret = DYNAMIC_DOWNCAST(CRadNotepadDoc, CWinAppEx::OpenDocumentFile(szFileName, bAddToMRU));

        if (ret != nullptr && line >= 0)
            ret->GetView()->GetCtrl().GotoLine(line);
    }
    else
    {
        ret = DYNAMIC_DOWNCAST(CRadNotepadDoc, CWinAppEx::OpenDocumentFile(lpszFileName, bAddToMRU));
    }

    if (bAddToMRU && ret != nullptr && !IsInternetUrl(lpszFileName) && !PathFileExists(lpszFileName))
    {
        for (int nIndex = 0; nIndex < m_pRecentFileList->GetSize(); ++nIndex)
        {
            if ((*m_pRecentFileList)[nIndex] == lpszFileName)
            {
                m_pRecentFileList->Remove(nIndex);
                break;
            }
        }
    }

    return ret;
}

// CRadNotepadApp message handlers

// App command to run the dialog
void CRadNotepadApp::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
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
    CRadDocManager* pDocManager = DYNAMIC_DOWNCAST(CRadDocManager, m_pDocManager);
    pDocManager->SaveAll();
}

void CRadNotepadApp::OnUpdateFileSaveAll(CCmdUI *pCmdUI)
{
    CRadDocManager* pDocManager = DYNAMIC_DOWNCAST(CRadDocManager, m_pDocManager);
    pCmdUI->Enable(pDocManager->GetModifiedDocumentCount() > 0);
}

void CRadNotepadApp::OnFileCloseOthers()
{
    CRadDocManager* pDocManager = DYNAMIC_DOWNCAST(CRadDocManager, m_pDocManager);
    CDocument* pDoc = CRadDocManager::GetActiveDocument();
    if (pDoc != nullptr)
        pDocManager->CloseOtherDocuments(pDoc);
}

void CRadNotepadApp::OnUpdateFileCloseOthers(CCmdUI *pCmdUI)
{
    pCmdUI->Enable(CRadDocManager::GetActiveDocument() != nullptr);
}

void CRadNotepadApp::OnUpdateViewSaveSettingsOnExit(CCmdUI *pCmdUI)
{
    pCmdUI->SetCheck(m_bSaveState);
}

void CRadNotepadApp::OnViewSaveSettingsOnExit()
{
    m_bSaveState = !m_bSaveState;
}
