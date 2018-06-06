/*
Module : ScintillaDocView.cpp
Purpose: Defines the implementation for MFC CView and CDocument derived wrapper classes for the Scintilla 
         edit control (www.scintilla.org).
Created: PJN / 19-03-2004
History: PJN / 12-08-2004 1. Made all the remaining non virtual functions related to Find and Replace virtual.
                          2. TextNotFound function is now passed the parameters used for the search. Thanks 
                          to Dmitry Naumov for this update.
                          3. Removed the OnTextNotFound function as all the work for it is achieved using the
                          TextNotFound function.
                          4. Creation of the find / replace dialog now takes place in a new virtual function
                          "CreateFindReplaceDialog". Again thanks to Dmitry Naumov for this update. 
         PJN / 04-03-2005 1. Fix in CScintillaView::PrintPage which now sorts out the issue of print preview
                          not working in the MFC doc / view wrappers for scintilla. Thanks to Frank Kiesel
                          for reporting this fix.
         PJN / 10-07-2005 1. Addition of a virtual function namely OnAutoCSelection which handles the
                          SCN_AUTOCSELECTION notification message which was introduced in Scintilla v1.63.
                          Thanks to Dan Petitt for suggesting this update.
                          2. A new boolean variable called "m_bUseROFileAttributeDuringLoading" has been added 
                          to CScintillaView. If this value is set then the code will check the read only file
                          attribute of the file being loaded and if it is set, then the document is marked as 
                          read only by Scintilla. By doing this any attempts to modify the document will cause 
                          the OnModifyAttemptRO virtual function to be called. This allows you to prompt to 
                          make the file read write or to check out the file from a version control system. 
                          Thanks to Dan Petitt for suggesting this update.
                          3. Demo app now demonstrates how to use the SCN_MODIFYATTEMPTRO notification.
                          4. Fixed a number of warnings when the code is compiled using Visual Studio .NET 2003.
                          5. SetSavePoint and SetReadOnly calls are now made in CScintillaDoc::OnSaveDocument
                          instead of CScintillaView::Serialize. This ensures that the control is only reset
                          upon a successful save.
         PJN / 03-01-2006 1. Fixed a bug where the separator line in the header was not being drawn due to 
                          issues in the code which calculates the coordinates. 
                          2. Added two member variables which decide whether printer headers and footers should
                          be printed (CScintillaView::m_bPrintHeader and CScintillaView::m_bPrintFooter). You
                          could of course achieve the same result by deriving a class from CScintillaView and 
                          implementing empty PrintHeader and PrinterFooter methods!!!. Thanks to Jason Elliott
                          for suggesting this update.
                          3. Optimized the construction of member variables in CScintillaView::CScintillaView
                          and in CScintillaFindReplaceDlg::CScintillaFindReplaceDlg
                          4. CScintillaView::OnReplaceAll sets the selection to 0,0 before doing text replacement. 
                          This ensures that all text in a document is replaced. Also this function now does not 
                          bother calling TextNotFound at the end of the function if replacements took place. 
                          Thanks to Jason Elliott for this nice update. 
                          5. A parameter which indicates whether text is being replaced or found is now sent to
                          CScintillaView::TextNotFound.
         PJN / 06-01-2006 1. Removed some unnecessary code from CScintillaView::OnEndPrinting. Thanks to 
                          Greg Smith for spotting this issue.
         PJN / 08-01-2006 1. Find / Replace dialog and associated state is now maintained outside of CScintillaView.
                          This means that if you have multiple CScintillaView's in your app, that they share the
                          one find / replace dialog which is the standard type of UI you would normally expect for 
                          this. Thanks to Greg Smith for reporting this issue.
                          2. Find / replace dialog is now closed when the last CScintillaView is destroyed. Thanks
                          to Greg Smith for reporting this issue.
         PJN / 16-01-2006 1. Removed an unused "rSetup" variable in CScintillaView::PrintPage
                          2. Optimized code in CScintillaView::PrintPage and CScintillaView::OnFilePageSetup which
                          determines if metric or imperial measurements are being used. Now a boolean member variable
                          of CScintillaView called m_bUsingMetric which by default picks up the control panel
                          preference is provided. This allows client code to change this value to customize how
                          measurements are specified. Thanks to Greg Smith for reporting this issue.
                          3. Fixed a small typo in CScintillaView::PrintHeader and PrintFooter. Also explicitly uses
                          the TA_TOP flag in combination with TA_LEFT when setting alignment settings for header
                          and footer text. Again thanks to Greg Smith for reporting this.
                          4. Scintilla find / replace state is now stored in a standard global variable instead of
                          using the CProcessLocal template which it was using previously. This is no longer required
                          since it was used to provide Win32s support for MFC on older versions of MFC. Since Win32s
                          is no longer supported by MFC, there is no need to use this mechanism any more.
         PJN / 02-02-2006 1. Minor update to CScintillaView to allow deletion of text when there is nothing selected. 
                          Thanks to Alexander Kirillov for reporting this bug. 
         PJN / 14-03-2006 1. Updated the internal function CScintillaView::SameAsSelected to support regular 
                          expressions. Thanks to Greg Smith for this update.
         PJN / 22-05-2006 1. Fixed a flicker issue when the view is resized. Thanks to Michael Gunlock for reporting 
                          this issue.
         PJN / 29-06-2005 1. Code now uses new C++ style casts rather than old style C casts where necessary. 
                          2. Code now supports persisting margin settings. Thanks to Krasimir Stoychev for this update
                          3. Updated the code to clean compile in VC 2005
         PJN / 17-09-2006 1. Updated OnReplaceSel and OnReplaceAll implementations to use TargetFromSelection and 
                          ReplaceTargetRE/ReplaceTarget. Thanks to Matt Spear for reporting this issue.
         PJN / 11-06-2007 1. Addition of a SCINTILLADOCVIEW_EXT_CLASS preprocessor macro to allow the classes to be 
                          more easily used in an extension DLL.
         PJN / 19-03-2008 1. Fixed a copy and paste bug in CScintillaView::PrintPage where the incorrect margin value
                          was being used in the calculation of "rfPrint.rc.left". Thanks to Steve Arnold for reporting
                          this bug.
         PJN / 20-01-2009 1. Updated copyright details
                          2. Fixed a bug in CScintillaView::SameAsSelected where it did not correctly handle the fact
                          that GetSelectionStart() and GetSelectionEnd() returns the positions in UTF-8 encoded text
                          which would result in failures in the logic for multibyte encoded characters. This made it
                          impossible to replace multibyte character sequences in find / replace operations. Thanks to
                          Alexei Letov for reporting this bug.
         PJN / 01-04-2011 1. Updated CScintillaFindReplaceDlg::Create to use AfxFindResourceHandle instead of 
                          AfxGetResourceHandle. Thanks to Niﬂl Markus for reporting this nice addition.
         PJN / 18-01-2013 1. Updated copryight details.
                          2. Added virtual methods for SCN_INDICATORCLICK, SCN_INDICATORRELEASE, SCN_AUTOCCHARDELETED, 
                          SCN_AUTOCCANCELLED & SCN_HOTSPOTRELEASECLICK notifications. Thanks to Markus Niﬂl for 
                          prompting this update.
                          3. The classes are now encapsulated in a Scintilla namespace if the SCI_NAMESPACE define
                          is defined. This is consistent with how the scintilla.h header file operates in the 
                          presence of this define. Thanks to Markus Niﬂl for prompting this update.
         PJN / 26-01-2015 1. Updated copyright details.
                          2. Added virtual methods for SCN_FOCUSIN & SCN_FOCUSOUT notifications.
         PJN / 19-09-2015 1. Added virtual methods for SCN_AUTOCCOMPLETED notification.
         PJN / 25-07-2016 1. Added SAL annotations to all the code
         PJN / 20-11-2016 1. Addition of a new CScintillaView::CreateControl virtual method. This allows the 
                          customization of the creation of the CScintillaCtrl instance at runtime in the view class.
                          Thanks to Markus Nissl for providing this nice addition.
         PJN / 20-12-2016 1. Updated code to use Sci_RangeToFormat typedef instead of RangeToFormat
                          2. Updated code to use Sci_TextToFind typedef instead of TextToFind
                          3. Updated code to use Sci_TextRange typedef instead of TextRange
                          4. Updated code to no longer use Scintilla namespace which has been removed from 
                          Scintilla.h
                          5. Added a new member variable called "m_bCPP11Regex" which decides if Scintilla's support
                          for C++11 regex should be used in the find / replace functionality. Thanks to Markus Nissl
                          for suggesting this update.
         PJN / 04-03-2017 1. Reworked the CScintillaView::OnActivateView to change the find / replace dialog owner 
                          to correctly handle cases where there are multiple top level scintilla views in your 
                          application. Thanks to to Kenny Lau for reporting this issue.
                          2. Refactored the logic in CScintillaView::OnDestroy which decides if the find / replace 
                          dialog should be destroyed into a new virtual ShouldDestroyFindReplaceDialog method.
                          3. Implemented a WM_NCDESTROY message handler for the find / replace dialog. This helps 
                          avoid problems where you would sometimes get an ASSERT thrown from 
                          CScintillaView::AssertValid. The code to validate the find / replace dialog has also been 
                          removed from CScintillaView::AssertValid. Thanks to Kenny Lau for reporting this issue.
         PJN / 12-06-2017 1. Made _SCINTILLA_EDIT_STATE class available in ScintillaDocView.h and renamed it to 
                          "CScintillaEditState". Thanks to Michael for requesting this addition.
                          2. Updated CScintillaView::FindTextSimple to track the initial position and restart 
                          searches from the beginning (or end if searching backwards) until the original start 
                          position was located. Thanks to Michael Thompson for provided this nice addition.
                          3. Updated the demo app included in the download to show how CScintillaView::CreateControl
                          can be used. Thanks to Michael Thompson for prompting this update.
         PJN / 28-01-2018 1. Added code to OnEditRepeat, OnFindNext and OnReplaceSel methods to ensure that the text 
                          just found or replaced is visible. This is necessary because sometimes the found text could 
                          be hidden in folded text. Thanks to Michael Thompson for reporting this issue.
                          2. The OnEditRepeat method now calls AdjustFindDialogPosition to ensure it is moved if 
                          necessary. Thanks to Michael Thompson for reporting this issue.
         PJN / 03-05-2018 1. Added support for SCN_MARGINRIGHTCLICK notification. Thanks to Karagoez Yusuf for 
                          reporting this issue.

Copyright (c) 2004 - 2018 by PJ Naughter (Web: www.naughter.com, Email: pjna@naughter.com)

All rights reserved.

Copyright / Usage Details:

You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise) 
when your product is released in binary form. You are allowed to modify the source code in any way you want 
except you cannot modify the copyright details at the top of each module. If you want to distribute source 
code with your application, then you are only allowed to distribute versions released by the author. This is 
to maintain a single distribution point for the source code. 

*/


///////////////////////////////// Includes ////////////////////////////////////

#include "stdafx.h"
#include "ScintillaDocView.h"
#ifndef __AFXDISP_H__
#pragma message("To avoid this message, please put afxdisp.h in your pre compiled header (normally stdafx.h)")
#include <afxdisp.h>
#endif //#ifndef __AFXDISP_H__
#include "resource.h"


//////////////////////////////// Statics / Macros /////////////////////////////

#ifdef _DEBUG
#define new DEBUG_NEW
#endif //#ifdef _DEBUG


////////////////////////////////// Implementation /////////////////////////////

CScintillaEditState g_scintillaEditState;


CScintillaEditState::CScintillaEditState() : bRegularExpression(FALSE),
                                             pFindReplaceDlg(nullptr),
                                             bWord(FALSE),
                                             bFindOnly(TRUE),
                                             bCase(FALSE),
                                             bNext(TRUE)
{
}


BEGIN_MESSAGE_MAP(CScintillaFindReplaceDlg, CFindReplaceDialog)
  ON_BN_CLICKED(IDC_REGULAR_EXPRESSION, &CScintillaFindReplaceDlg::OnRegularExpression)
  ON_WM_NCDESTROY()
END_MESSAGE_MAP()

CScintillaFindReplaceDlg::CScintillaFindReplaceDlg(): m_bRegularExpression(FALSE)
{
}

BOOL CScintillaFindReplaceDlg::Create(BOOL bFindDialogOnly, LPCTSTR lpszFindWhat, LPCTSTR lpszReplaceWith, DWORD dwFlags, CWnd* pParentWnd)
{
  //Tell Windows to use our dialog instead of the standard one
  m_fr.Flags |= FR_ENABLETEMPLATE;
  if (bFindDialogOnly)
    m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_SCINTILLA_FINDDLGORD);
  else
    m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_SCINTILLA_REPLACEDLGORD);
  m_fr.hInstance = AfxFindResourceHandle(m_fr.lpTemplateName, RT_DIALOG);

  //Let the base class do its thing
  return CFindReplaceDialog::Create(bFindDialogOnly,	lpszFindWhat, lpszReplaceWith, dwFlags, pParentWnd);
}

BOOL CScintillaFindReplaceDlg::OnInitDialog() 
{
  //let the base class do its thing
  BOOL bReturn = CFindReplaceDialog::OnInitDialog();

  //Should we check the regular expression check box
  CButton* pCtrl = static_cast<CButton*>(GetDlgItem(IDC_REGULAR_EXPRESSION));
  AFXASSUME(pCtrl != nullptr);
  pCtrl->SetCheck(m_bRegularExpression);
  
  return bReturn;
}

void CScintillaFindReplaceDlg::OnRegularExpression() 
{
  //Save the state of the Regular expression checkbox into a member variable
  CButton* pCtrl = static_cast<CButton*>(GetDlgItem(IDC_REGULAR_EXPRESSION));
  AFXASSUME(pCtrl != nullptr);
  m_bRegularExpression = (pCtrl->GetCheck() == 1);
}

void CScintillaFindReplaceDlg::OnNcDestroy()
{
  //Force the find replace dialog pointer to null
  g_scintillaEditState.pFindReplaceDlg = nullptr;

  //Let the base class do its thing
  __super::OnNcDestroy();
}


IMPLEMENT_DYNCREATE(CScintillaView, CView)

const UINT _ScintillaMsgFindReplace = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CScintillaView, CView)
  ON_WM_PAINT()
  ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, &CScintillaView::OnUpdateNeedSel)
  ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CScintillaView::OnUpdateNeedPaste)
  ON_UPDATE_COMMAND_UI(ID_EDIT_FIND, &CScintillaView::OnUpdateNeedText)
  ON_UPDATE_COMMAND_UI(ID_EDIT_REPEAT, &CScintillaView::OnUpdateNeedFind)
  ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CScintillaView::OnUpdateEditUndo)
  ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CScintillaView::OnUpdateEditRedo)
  ON_COMMAND(ID_EDIT_CUT, &CScintillaView::OnEditCut)
  ON_COMMAND(ID_EDIT_COPY, &CScintillaView::OnEditCopy)
  ON_COMMAND(ID_EDIT_PASTE, &CScintillaView::OnEditPaste)
  ON_COMMAND(ID_EDIT_CLEAR, &CScintillaView::OnEditClear)
  ON_COMMAND(ID_EDIT_UNDO, &CScintillaView::OnEditUndo)
  ON_COMMAND(ID_EDIT_REDO, &CScintillaView::OnEditRedo)
  ON_COMMAND(ID_EDIT_SELECT_ALL, &CScintillaView::OnEditSelectAll)
  ON_COMMAND(ID_EDIT_FIND, &CScintillaView::OnEditFind)
  ON_COMMAND(ID_EDIT_REPLACE, &CScintillaView::OnEditReplace)
  ON_COMMAND(ID_EDIT_REPEAT, &CScintillaView::OnEditRepeat)
  ON_WM_SETFOCUS()
  ON_WM_SIZE()
  ON_WM_ERASEBKGND()
  ON_WM_CREATE()
  ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, &CScintillaView::OnUpdateNeedSel)
  ON_UPDATE_COMMAND_UI(ID_EDIT_CLEAR, &CScintillaView::OnUpdateNeedTextAndFollowingText)
  ON_UPDATE_COMMAND_UI(ID_EDIT_SELECT_ALL, &CScintillaView::OnUpdateNeedText)
  ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACE, &CScintillaView::OnUpdateNeedText)
  ON_COMMAND(ID_FILE_PAGE_SETUP, &CScintillaView::OnFilePageSetup)
  ON_WM_DESTROY()
  ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
  ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CView::OnFilePrintPreview)
  ON_REGISTERED_MESSAGE(_ScintillaMsgFindReplace, &CScintillaView::OnFindReplaceCmd)
END_MESSAGE_MAP()

CScintillaView::CScintillaView() : m_rMargin(0, 0, 0, 0), 
                                   m_bFirstSearch(TRUE),
                                   m_bChangeFindRange(FALSE),
                                   m_lInitialSearchPos(0),
                                   m_bUseROFileAttributeDuringLoading(TRUE),
                                   m_bPrintHeader(TRUE), 
                                   m_bPrintFooter(TRUE),
                                   m_bUsingMetric(UserWantsMetric()),
                                   m_bPersistMarginSettings(TRUE),
                                   m_bCPP11Regex(TRUE)
{
}

CScintillaView::~CScintillaView()
{
}

CScintillaCtrl& CScintillaView::GetCtrl()
{ 
  if (m_pEdit == nullptr)
    m_pEdit = CreateControl();

  return *m_pEdit; 
}

std::unique_ptr<CScintillaCtrl> CScintillaView::CreateControl()
{
#if _MSC_VER >= 1800
  return std::make_unique<CScintillaCtrl>();
#else
  return std::unique_ptr<CScintillaCtrl>(new CScintillaCtrl());
#endif
}

void CScintillaView::LoadMarginSettings(const CString& sSection)
{
  //Get the margin values 
  CWinApp* pApp = AfxGetApp();
  AFXASSUME(pApp != nullptr);
  m_rMargin.left = pApp->GetProfileInt(sSection, _T("LeftMargin"), m_rMargin.left);
  m_rMargin.right = pApp->GetProfileInt(sSection, _T("RightMargin"), m_rMargin.right);
  m_rMargin.top = pApp->GetProfileInt(sSection, _T("TopMargin"), m_rMargin.top);
  m_rMargin.bottom = pApp->GetProfileInt(sSection, _T("BottomMargin"), m_rMargin.bottom);
}

void CScintillaView::SaveMarginSettings(const CString& sSection)
{
  //Write out the margin values 
  CWinApp* pApp = AfxGetApp();
  AFXASSUME(pApp != nullptr);
  pApp->WriteProfileInt(sSection, _T("LeftMargin"), m_rMargin.left);
  pApp->WriteProfileInt(sSection, _T("RightMargin"), m_rMargin.right);
  pApp->WriteProfileInt(sSection, _T("TopMargin"), m_rMargin.top);
  pApp->WriteProfileInt(sSection, _T("BottomMargin"), m_rMargin.bottom);
}

BOOL CScintillaView::ShouldDestroyFindReplaceDialog()
{
  //What will be the return value from this method
  BOOL bDestroy = FALSE;

  if (g_scintillaEditState.pFindReplaceDlg != nullptr)
  {
    //Count up the number of CScintillaView's we have (excluding this one)
    int nScintillaViews = 0;

    CWinApp* pApp = AfxGetApp();
    AFXASSUME(pApp != nullptr);

    //no doc manager - no templates
    if (pApp->m_pDocManager != nullptr)
    {
      //walk all templates
      CDocTemplate* pTemplate;
      POSITION pos = pApp->m_pDocManager->GetFirstDocTemplatePosition();
      while (pos && (nScintillaViews == 0))
      {
        pTemplate = pApp->m_pDocManager->GetNextDocTemplate(pos);
        AFXASSUME(pTemplate != nullptr);

        //walk all documents in the template
        POSITION pos2 = pTemplate->GetFirstDocPosition();
        while (pos2 && (nScintillaViews == 0))
        {
          CDocument* pDoc = pTemplate->GetNextDoc(pos2);
          AFXASSUME(pDoc != nullptr);

          //walk all views in the document
          POSITION pos3 = pDoc->GetFirstViewPosition();
          while (pos3 && (nScintillaViews == 0))
          {
            CView* pView = pDoc->GetNextView(pos3);
            AFXASSUME(pView != nullptr);

            if (pView->IsKindOf(RUNTIME_CLASS(CScintillaView)) && (pView != this) && ::IsWindow(pView->GetSafeHwnd()))
              ++nScintillaViews;
          }
        }
      }
    }

    bDestroy = (nScintillaViews == 0);
  }

  return bDestroy;
}

void CScintillaView::OnDestroy()
{
  //Close Find/Replace dialog if necessary
  if (ShouldDestroyFindReplaceDialog())
  {
    AFXASSUME(g_scintillaEditState.pFindReplaceDlg != nullptr);

    if (::IsWindow(g_scintillaEditState.pFindReplaceDlg->m_hWnd))
      g_scintillaEditState.pFindReplaceDlg->SendMessage(WM_CLOSE);
    g_scintillaEditState.pFindReplaceDlg = nullptr;
  }

  //Let the base class do its thing
  CView::OnDestroy();
}

void CScintillaView::DeleteContents()
{
  ASSERT_VALID(this);
  ASSERT(m_hWnd != nullptr);

  CScintillaCtrl& rCtrl = GetCtrl();
  rCtrl.ClearAll();
  rCtrl.EmptyUndoBuffer();
}

void CScintillaView::OnDraw(CDC*)
{
  ASSERT(FALSE);
}

void CScintillaView::OnPaint()
{
  //this is done to avoid CView::OnPaint
  Default();
}

BOOL CScintillaView::OnPreparePrinting(CPrintInfo* pInfo)
{
  //Determine if we should allow selection printing
  CScintillaCtrl& rCtrl = GetCtrl();

  Sci_Position nStartChar = rCtrl.GetSelectionStart();
  Sci_Position nEndChar = rCtrl.GetSelectionEnd();
  if (nStartChar != nEndChar)
  {
    // Enable the Selection button
    pInfo->m_pPD->m_pd.Flags &= ~PD_NOSELECTION;

    // Check the Selection button
    pInfo->m_pPD->m_pd.Flags |= PD_SELECTION;
  }

  //Let the base class do its thing
  return DoPreparePrinting(pInfo);
}

void CScintillaView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* pInfo)
{
  //Validate our parameters
  ASSERT_VALID(this);

  CScintillaCtrl& rCtrl = GetCtrl();

  //initialize page start vector
  ASSERT(m_aPageStart.GetSize() == 0);
  if (pInfo->m_pPD->PrintSelection())
    m_aPageStart.Add(rCtrl.GetSelectionStart());
  else
    m_aPageStart.Add(0);
  ASSERT(m_aPageStart.GetSize() > 0);

  ASSERT_VALID(this);
}

BOOL CScintillaView::PaginateTo(CDC* pDC, CPrintInfo* pInfo)
{
  //Validate our parameters
  ASSERT_VALID(this);
  ASSERT_VALID(pDC);

  CRect rectSave = pInfo->m_rectDraw;
  UINT nPageSave = pInfo->m_nCurPage;
  ASSERT(nPageSave > 1);
  ASSERT(nPageSave >= static_cast<UINT>(m_aPageStart.GetSize()));
  VERIFY(pDC->SaveDC() != 0);
  pDC->IntersectClipRect(0, 0, 0, 0);
  pInfo->m_nCurPage = static_cast<UINT>(m_aPageStart.GetSize());
  while (pInfo->m_nCurPage < nPageSave)
  {
    ASSERT(pInfo->m_nCurPage == static_cast<UINT>(m_aPageStart.GetSize()));
    OnPrepareDC(pDC, pInfo);
    ASSERT(pInfo->m_bContinuePrinting);
    pInfo->m_rectDraw.SetRect(0, 0,	pDC->GetDeviceCaps(HORZRES), pDC->GetDeviceCaps(VERTRES));
    OnPrint(pDC, pInfo);
    if (pInfo->m_nCurPage == static_cast<UINT>(m_aPageStart.GetSize()))
      break;
    ++pInfo->m_nCurPage;
  }
  BOOL bResult = pInfo->m_nCurPage == nPageSave;
  pDC->RestoreDC(-1);
  pInfo->m_nCurPage = nPageSave;
  pInfo->m_rectDraw = rectSave;
  ASSERT_VALID(this);
  return bResult;
}

void CScintillaView::OnPrepareDC(CDC* pDC, CPrintInfo* pInfo)
{
  //Validate our parameters
  ASSERT_VALID(this);
  ASSERT_VALID(pDC);
  AFXASSUME(pInfo != nullptr);

  if (pInfo->m_nCurPage <= pInfo->GetMaxPage())
  {
    if ((pInfo->m_nCurPage > static_cast<UINT>(m_aPageStart.GetSize())) &&	!PaginateTo(pDC, pInfo))
    {
      //can't paginate to that page, thus cannot print it.
      pInfo->m_bContinuePrinting = FALSE;
    }
    ASSERT_VALID(this);
  }
  else
  {
    //Reached the max page to print
    pInfo->m_bContinuePrinting = FALSE;
  }
}

void CScintillaView::PrintHeader(CDC* pDC, CPrintInfo* /*pInfo*/, Sci_RangeToFormat& frPrint)
{
  //By Default we print "Document Name - Printed on Date" as well as a line separator below the text
  //Derived classes are of course free to implement their own version of PrintHeader
  CString sHeader;
  AfxFormatString2(sHeader, IDS_SCINTILLA_DEFAULT_PRINT_HEADER, GetDocument()->GetTitle(), COleDateTime::GetCurrentTime().Format());

  //Setup the DC
  pDC->SetTextColor(RGB(0, 0, 0));
  UINT nAlign = pDC->SetTextAlign(TA_LEFT | TA_TOP);

  //Draw the header
  CSize sizeText = pDC->GetTextExtent(sHeader);
  int nHeaderDepth = 2*sizeText.cy;
  CRect rLine(frPrint.rcPage.left, frPrint.rcPage.top, frPrint.rcPage.right, frPrint.rcPage.top + nHeaderDepth);
  pDC->ExtTextOut(frPrint.rcPage.left, frPrint.rcPage.top + nHeaderDepth/3, 0, &rLine, sHeader, nullptr);

  //Draw a line underneath the text
  pDC->MoveTo(frPrint.rcPage.left, frPrint.rcPage.top + nHeaderDepth - 5);
  pDC->LineTo(frPrint.rcPage.right, frPrint.rcPage.top + nHeaderDepth - 5);

  //Restore the DC
  pDC->SetTextAlign(nAlign);

  //Adjust the place where scintilla will draw the text
  if (frPrint.rc.top < (frPrint.rcPage.top + nHeaderDepth))
    frPrint.rc.top = frPrint.rcPage.top + nHeaderDepth;
}

void CScintillaView::PrintFooter(CDC* pDC, CPrintInfo* pInfo, Sci_RangeToFormat& frPrint)
{
  //By Default we print "Page X" as well as a line separator above the text
  //Derived classes are of course free to implement their own version of PrintFooter
  CString sPage;
  sPage.Format(_T("%u"), pInfo->m_nCurPage);
  CString sFooter;
  AfxFormatString1(sFooter, IDS_SCINTILLA_DEFAULT_PRINT_FOOTER, sPage);

  //Setup the DC
  pDC->SetTextColor(RGB(0, 0, 0));
  UINT nAlign = pDC->SetTextAlign(TA_LEFT | TA_TOP);
  
  //Draw the header
  CSize sizeText = pDC->GetTextExtent(sFooter);
  int nFooterDepth = 2*sizeText.cy;
  CRect rLine(frPrint.rcPage.left, frPrint.rcPage.bottom - nFooterDepth, frPrint.rcPage.right, frPrint.rcPage.bottom);
  pDC->ExtTextOut(frPrint.rcPage.left, frPrint.rcPage.bottom - nFooterDepth*2/3, 0, &rLine, sFooter, nullptr);

  //Draw a line above the text
  pDC->MoveTo(frPrint.rcPage.left, frPrint.rcPage.bottom - nFooterDepth + 5);
  pDC->LineTo(frPrint.rcPage.right, frPrint.rcPage.bottom - nFooterDepth + 5);

  //Restore the DC
  pDC->SetTextAlign(nAlign);

  //Adjust the place where scintilla will draw the text
  if (frPrint.rc.bottom > (frPrint.rcPage.bottom - nFooterDepth))
    frPrint.rc.bottom = frPrint.rcPage.bottom - nFooterDepth;
}

BOOL CScintillaView::UserWantsMetric()
{
  TCHAR localeInfo[3];
  localeInfo[0] = _T('\0');
  GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IMEASURE, localeInfo, 3);
  return (localeInfo[0] == _T('0')) ;
}

Sci_Position CScintillaView::PrintPage(CDC* pDC, CPrintInfo* pInfo, Sci_Position nIndexStart, Sci_Position nIndexStop)
{
  //Validate our parameters
  ASSERT_VALID(this);
  ASSERT_VALID(pDC);

  Sci_RangeToFormat rfPrint;
  rfPrint.hdc = pDC->m_hDC;
  rfPrint.hdcTarget = pDC->m_hAttribDC;

  //Take into account the specified margins
  CRect rMargins;
  if ((m_rMargin.left) != 0 || (m_rMargin.right) != 0 || (m_rMargin.top) != 0 || (m_rMargin.bottom != 0)) 
  {
    //Get printer resolution
    CPoint pDpi;
    pDpi.x = pDC->GetDeviceCaps(LOGPIXELSX);  //DPI in X direction
    pDpi.y = pDC->GetDeviceCaps(LOGPIXELSY);  //DPI in Y direction

    //Convert the hundredths of millimeters or thousandths of inches margin values
    //from the Page Setup dialog to device units.
    int iScale = m_bUsingMetric ? 2540 : 1000;    //scale factor for margin scaling;
    rMargins.left   = MulDiv(m_rMargin.left, pDpi.x, iScale);
    rMargins.top    = MulDiv(m_rMargin.top, pDpi.y, iScale);
    rMargins.right	= MulDiv(m_rMargin.right, pDpi.x, iScale);
    rMargins.bottom	= MulDiv(m_rMargin.bottom, pDpi.y, iScale);
  } 
  else 
    rMargins = m_rMargin;

  //We take the page size from the pInfo member variable (decrement the right and
  //bottom values by 1 to suit Scintilla)
  rfPrint.rcPage.left = pInfo->m_rectDraw.left;
  rfPrint.rcPage.top = pInfo->m_rectDraw.top;
  rfPrint.rcPage.right = pInfo->m_rectDraw.right - 1;
  rfPrint.rcPage.bottom = pInfo->m_rectDraw.bottom - 1;

  //Fill in the area to print
  rfPrint.rc.left = rfPrint.rcPage.left + rMargins.left;
  rfPrint.rc.top = rfPrint.rcPage.top + rMargins.top;
  rfPrint.rc.right = rfPrint.rcPage.right - rMargins.right;
  rfPrint.rc.bottom = rfPrint.rcPage.bottom - rMargins.bottom;

  //Fill in the text to print
  rfPrint.chrg.cpMin = static_cast<Sci_PositionCR>(nIndexStart);
  rfPrint.chrg.cpMax = static_cast<Sci_PositionCR>(nIndexStop);

  //Print the header (if requested to)
  if (m_bPrintHeader)
    PrintHeader(pDC, pInfo, rfPrint);

  //Print the footer (if requested to)
  if (m_bPrintFooter)
    PrintFooter(pDC, pInfo, rfPrint);

  //Finally ask the control to print the text
  return GetCtrl().FormatRange(TRUE, &rfPrint);
}

void CScintillaView::OnPrint(CDC* pDC, CPrintInfo* pInfo)
{
  //Validate our parameters
  ASSERT_VALID(this);
  ASSERT_VALID(pDC);
  AFXASSUME(pInfo != nullptr);
  ASSERT(pInfo->m_bContinuePrinting);

  UINT nPage = pInfo->m_nCurPage;
  ASSERT(nPage <= (UINT) m_aPageStart.GetSize());
  Sci_Position nIndex = m_aPageStart[nPage-1];

  //Determine where we should end the printing
  Sci_Position nEndPrint = 0;
  if (pInfo->m_pPD->PrintSelection())
    nEndPrint = GetCtrl().GetSelectionEnd();
  else
    nEndPrint = GetCtrl().GetTextLength();

  //print as much as possible in the current page.
  nIndex = PrintPage(pDC, pInfo, nIndex, nEndPrint);
  if (nIndex >= nEndPrint)
  {
    TRACE0("End of Document\n");
    pInfo->SetMaxPage(nPage);
  }

  //update pagination information for page just printed
  if (nPage == (UINT) m_aPageStart.GetSize())
  {
    if (nIndex < nEndPrint)
      m_aPageStart.Add(nIndex);
  }
  else
  {
    ASSERT(nPage+1 <= static_cast<UINT>(m_aPageStart.GetSize()));
    ASSERT(nIndex == m_aPageStart[nPage+1-1]);
  }
}

void CScintillaView::OnEndPrinting(CDC*, CPrintInfo*)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  m_aPageStart.RemoveAll();
}

void CScintillaView::OnUpdateNeedPaste(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  pCmdUI->Enable(GetCtrl().CanPaste());
}

void CScintillaView::OnUpdateNeedText(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  pCmdUI->Enable(GetCtrl().GetTextLength() != 0);
}

void CScintillaView::OnUpdateNeedTextAndFollowingText(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);

  CScintillaCtrl& rCtrl = GetCtrl();
  int nLength = rCtrl.GetTextLength();
  Sci_Position nStartChar = rCtrl.GetSelectionStart();

  pCmdUI->Enable(nLength && (nStartChar != nLength));
}

void CScintillaView::OnUpdateNeedFind(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  pCmdUI->Enable(GetCtrl().GetLength() != 0 && !g_scintillaEditState.strFind.IsEmpty());
}

void CScintillaView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  pCmdUI->Enable(GetCtrl().CanUndo());
}

void CScintillaView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  pCmdUI->Enable(GetCtrl().CanRedo());
}

void CScintillaView::OnUpdateNeedSel(CCmdUI* pCmdUI)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  CScintillaCtrl& rCtrl = GetCtrl();
  Sci_Position nStartChar = rCtrl.GetSelectionStart();
  Sci_Position nEndChar = rCtrl.GetSelectionEnd();
  pCmdUI->Enable(nStartChar != nEndChar);
}

void CScintillaView::OnEditCut()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().Cut();
}

void CScintillaView::OnEditCopy()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().Copy();
}

void CScintillaView::OnEditPaste()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().Paste();
}

void CScintillaView::OnEditClear()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().Clear();
}

void CScintillaView::OnEditUndo()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().Undo();
}

void CScintillaView::OnEditRedo()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().Redo();
}

void CScintillaView::OnEditSelectAll()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  GetCtrl().SelectAll();
}

void CScintillaView::OnEditFind()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  OnEditFindReplace(TRUE);
}

void CScintillaView::OnEditReplace()
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  OnEditFindReplace(FALSE);
}

void CScintillaView::OnEditRepeat()
{
  //Validate our parameters
  ASSERT_VALID(this);

  if (!FindText(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression))
    TextNotFound(g_scintillaEditState.strFind, g_scintillaEditState.bNext, g_scintillaEditState.bCase, g_scintillaEditState.bWord, g_scintillaEditState.bRegularExpression, FALSE);
  else
  {
    CScintillaCtrl& rCtrl = GetCtrl();
    int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
    rCtrl.EnsureVisible(nLine);
    if (g_scintillaEditState.pFindReplaceDlg != nullptr)
      AdjustFindDialogPosition();
  }
}

void CScintillaView::AdjustFindDialogPosition()
{
  //Validate our parameters
  AFXASSUME(g_scintillaEditState.pFindReplaceDlg != nullptr);
  
  CScintillaCtrl& rCtrl = GetCtrl();
  Sci_Position nStart = rCtrl.GetSelectionStart();
  CPoint point;
  point.x = rCtrl.PointXFromPosition(nStart);
  point.y = rCtrl.PointYFromPosition(nStart);
  ClientToScreen(&point);
  CRect rectDlg;
  g_scintillaEditState.pFindReplaceDlg->GetWindowRect(&rectDlg);
  if (rectDlg.PtInRect(point))
  {
    if (point.y > rectDlg.Height())
      rectDlg.OffsetRect(0, point.y - rectDlg.bottom - 20);
    else
    {
      int nVertExt = GetSystemMetrics(SM_CYSCREEN);
      if (point.y + rectDlg.Height() < nVertExt)
        rectDlg.OffsetRect(0, 40 + point.y - rectDlg.top);
    }
    g_scintillaEditState.pFindReplaceDlg->MoveWindow(&rectDlg);
  }
}

CScintillaFindReplaceDlg* CScintillaView::CreateFindReplaceDialog()
{
  return new CScintillaFindReplaceDlg;
}

void CScintillaView::OnEditFindReplace(_In_ BOOL bFindOnly)
{
  //Validate our parameters
  ASSERT_VALID(this);

  m_bFirstSearch = TRUE;
  if (g_scintillaEditState.pFindReplaceDlg != nullptr)
  {
    if (g_scintillaEditState.bFindOnly == bFindOnly)
    {
      g_scintillaEditState.pFindReplaceDlg->SetActiveWindow();
      g_scintillaEditState.pFindReplaceDlg->ShowWindow(SW_SHOW);
      return;
    }
    else
    {
      ASSERT(g_scintillaEditState.bFindOnly != bFindOnly);
      g_scintillaEditState.pFindReplaceDlg->SendMessage(WM_CLOSE);
      ASSERT(g_scintillaEditState.pFindReplaceDlg == nullptr);
      ASSERT_VALID(this);
    }
  }
  CScintillaCtrl& rCtrl = GetCtrl();
  CString strFind(rCtrl.GetSelText());
  //if selection is empty or spans multiple lines use old find text
  if (strFind.IsEmpty() || (strFind.FindOneOf(_T("\n\r")) != -1))
    strFind = g_scintillaEditState.strFind;

  CString strReplace(g_scintillaEditState.strReplace);
  g_scintillaEditState.pFindReplaceDlg = CreateFindReplaceDialog();
  AFXASSUME(g_scintillaEditState.pFindReplaceDlg != nullptr);
  DWORD dwFlags = 0;
  if (g_scintillaEditState.bNext)
    dwFlags |= FR_DOWN;
  if (g_scintillaEditState.bCase)
    dwFlags |= FR_MATCHCASE;
  if (g_scintillaEditState.bWord)
    dwFlags |= FR_WHOLEWORD;
  if (g_scintillaEditState.bRegularExpression)
    g_scintillaEditState.pFindReplaceDlg->SetRegularExpression(TRUE);

  if (!g_scintillaEditState.pFindReplaceDlg->Create(bFindOnly, strFind, strReplace, dwFlags, this))
  {
    g_scintillaEditState.pFindReplaceDlg = nullptr;
    ASSERT_VALID(this);
    return;
  }
  ASSERT(g_scintillaEditState.pFindReplaceDlg != nullptr);
  g_scintillaEditState.bFindOnly = bFindOnly;
  g_scintillaEditState.pFindReplaceDlg->SetActiveWindow();
  g_scintillaEditState.pFindReplaceDlg->ShowWindow(SW_SHOW);
  ASSERT_VALID(this);
}

void CScintillaView::OnFindNext(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  g_scintillaEditState.strFind = lpszFind;
  g_scintillaEditState.bCase = bCase;
  g_scintillaEditState.bWord = bWord;
  g_scintillaEditState.bNext = bNext;
  g_scintillaEditState.bRegularExpression = bRegularExpression;

  if (!FindText(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression))
    TextNotFound(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, FALSE);
  else
  {
    CScintillaCtrl& rCtrl = GetCtrl();
    int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
    rCtrl.EnsureVisible(nLine);
    if (g_scintillaEditState.pFindReplaceDlg != nullptr)
      AdjustFindDialogPosition();
  }
  ASSERT_VALID(this);
}

void CScintillaView::OnReplaceSel(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression, _In_z_ LPCTSTR lpszReplace)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  g_scintillaEditState.strFind = lpszFind;
  g_scintillaEditState.strReplace = lpszReplace;
  g_scintillaEditState.bCase = bCase;
  g_scintillaEditState.bWord = bWord;
  g_scintillaEditState.bNext = bNext;
  g_scintillaEditState.bRegularExpression = bRegularExpression;

  CScintillaCtrl& rCtrl = GetCtrl();

  if (!SameAsSelected(g_scintillaEditState.strFind, bCase, bWord, bRegularExpression))
  {
    if (!FindText(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression))
      TextNotFound(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, TRUE);
    else
    {
      int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
      rCtrl.EnsureVisible(nLine);
      if (g_scintillaEditState.pFindReplaceDlg != nullptr)
        AdjustFindDialogPosition();
    }
    return;
  }

  if (bRegularExpression)
  {
    rCtrl.TargetFromSelection();
    rCtrl.ReplaceTargetRE(g_scintillaEditState.strReplace.GetLength(), g_scintillaEditState.strReplace);    
  }
  else
    GetCtrl().ReplaceSel(g_scintillaEditState.strReplace);
  if (!FindText(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression))
    TextNotFound(g_scintillaEditState.strFind, bNext, bCase, bWord, bRegularExpression, TRUE);
  else
  {
    int nLine = rCtrl.LineFromPosition(rCtrl.GetSelectionStart());
    rCtrl.EnsureVisible(nLine);
    if (g_scintillaEditState.pFindReplaceDlg != nullptr)
      AdjustFindDialogPosition();
  }
  ASSERT_VALID(this);
}

void CScintillaView::OnReplaceAll(_In_z_ LPCTSTR lpszFind, _In_z_ LPCTSTR lpszReplace, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  g_scintillaEditState.strFind = lpszFind;
  g_scintillaEditState.strReplace = lpszReplace;
  g_scintillaEditState.bCase = bCase;
  g_scintillaEditState.bWord = bWord;
  g_scintillaEditState.bNext = TRUE;
  g_scintillaEditState.bRegularExpression = bRegularExpression;

  CWaitCursor wait;

  //Set the selection to the begining of the document to ensure all text is replaced in the document
  CScintillaCtrl& rCtrl = GetCtrl();
  rCtrl.SetSel(0, 0);

  //Do the replacments
  rCtrl.HideSelection(TRUE);
  BOOL bFoundSomething = FALSE;
  while (FindTextSimple(g_scintillaEditState.strFind, g_scintillaEditState.bNext, bCase, bWord, bRegularExpression))
  {
    bFoundSomething = TRUE;
    if (bRegularExpression)
    {
      rCtrl.TargetFromSelection();
      rCtrl.ReplaceTargetRE(g_scintillaEditState.strReplace.GetLength(), g_scintillaEditState.strReplace);
    }
    else
      rCtrl.ReplaceSel(g_scintillaEditState.strReplace);
  }

  //Restore the old selection
  rCtrl.HideSelection(FALSE);

  //Inform the user if we could not find anything
  if (!bFoundSomething)
    TextNotFound(g_scintillaEditState.strFind, g_scintillaEditState.bNext, bCase, bWord, bRegularExpression, TRUE);

  ASSERT_VALID(this);
}

LRESULT CScintillaView::OnFindReplaceCmd(WPARAM /*wParam*/, LPARAM lParam)
{
  //Validate our parameters
  ASSERT_VALID(this);

  CScintillaFindReplaceDlg* pDialog = static_cast<CScintillaFindReplaceDlg*>(CFindReplaceDialog::GetNotifier(lParam));
  AFXASSUME(pDialog != nullptr);
  ASSERT(pDialog == g_scintillaEditState.pFindReplaceDlg);

  if (pDialog->IsTerminating())
    g_scintillaEditState.pFindReplaceDlg = nullptr;
  else if (pDialog->FindNext())
    OnFindNext(pDialog->GetFindString(), pDialog->SearchDown(), pDialog->MatchCase(), pDialog->MatchWholeWord(), pDialog->GetRegularExpression());
  else if (pDialog->ReplaceCurrent())
  {
    ASSERT(!g_scintillaEditState.bFindOnly);
    OnReplaceSel(pDialog->GetFindString(), pDialog->SearchDown(), pDialog->MatchCase(), pDialog->MatchWholeWord(), pDialog->GetRegularExpression(), pDialog->GetReplaceString());
  }
  else if (pDialog->ReplaceAll())
  {
    ASSERT(!g_scintillaEditState.bFindOnly);
    OnReplaceAll(pDialog->GetFindString(), pDialog->GetReplaceString(), pDialog->MatchCase(), pDialog->MatchWholeWord(), pDialog->GetRegularExpression());
  }
  ASSERT_VALID(this);

  return 0;
}

BOOL CScintillaView::SameAsSelected(_In_z_ LPCTSTR lpszCompare, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
  CScintillaCtrl& rCtrl = GetCtrl();

  //check length first
  Sci_Position nStartChar = rCtrl.GetSelectionStart(); //get the selection size
  Sci_Position nEndChar = rCtrl.GetSelectionEnd();
  size_t nLen = _tcslen(lpszCompare); //get the #chars to search for
  
  //Calculate the logical length of the selection. This logic handles the case where Scintilla is hosting multibyte characters
  size_t nCnt = 0;
  for (Sci_Position nPos=nStartChar; nPos<nEndChar; nPos=rCtrl.PositionAfter(nPos))
    nCnt++;
  
  //if not a regular expression then sizes must match
  if (!bRegularExpression && (nLen != nCnt))
    return FALSE;

  //Now use the advanced search functionality of scintilla to determine the result
  int nFlags = bCase ? SCFIND_MATCHCASE : 0;
  nFlags |= bWord ? SCFIND_WHOLEWORD : 0;
  if (bRegularExpression)
    nFlags |= m_bCPP11Regex ? SCFIND_REGEXP | SCFIND_CXX11REGEX : SCFIND_REGEXP;
  rCtrl.SetSearchFlags(nFlags);
  rCtrl.TargetFromSelection();                     //set target
  if (rCtrl.SearchInTarget(static_cast<int>(nLen), lpszCompare) < 0) //see what we got
    return FALSE;                                  //no match

  //If we got a match, the target is set to the found text
  return (rCtrl.GetTargetStart() == nStartChar) && (rCtrl.GetTargetEnd() == nEndChar);
}

BOOL CScintillaView::FindText(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  CWaitCursor wait;
  return FindTextSimple(lpszFind, bNext, bCase, bWord, bRegularExpression);
}

BOOL CScintillaView::FindTextSimple(_In_z_ LPCTSTR lpszFind, _In_ BOOL bNext, _In_ BOOL bCase, _In_ BOOL bWord, _In_ BOOL bRegularExpression)
{
  //Validate our parameters
  ASSERT(lpszFind != nullptr);

  CScintillaCtrl& rCtrl = GetCtrl();

  Sci_TextToFind ft;
  ft.chrg.cpMin = static_cast<Sci_PositionCR>(rCtrl.GetSelectionStart());
  ft.chrg.cpMax = static_cast<Sci_PositionCR>(rCtrl.GetSelectionEnd());
  if (m_bFirstSearch)
  {
    if (bNext)
      m_lInitialSearchPos = ft.chrg.cpMin;
    else
      m_lInitialSearchPos = ft.chrg.cpMax;

    m_bFirstSearch = FALSE;
    m_bChangeFindRange = FALSE;
  }

#ifdef _UNICODE
  CStringA sUTF8Text(CScintillaCtrl::W2UTF8(lpszFind, -1));
  ft.lpstrText = sUTF8Text.GetBuffer(sUTF8Text.GetLength());
#else
  CStringA sAsciiText(lpszFind);
  ft.lpstrText = sAsciiText.GetBuffer(sAsciiText.GetLength());
#endif //#ifdef _UNICODE
  if (ft.chrg.cpMin != ft.chrg.cpMax) // i.e. there is a selection
  {
  #ifndef _UNICODE
    //If byte at beginning of selection is a DBCS lead byte,
    //increment by one extra byte.
    TEXTRANGE textRange;
    TCHAR ch[2];
    textRange.chrg.cpMin = ft.chrg.cpMin;
    textRange.chrg.cpMax = ft.chrg.cpMin + 1;
    textRange.lpstrText = ch;
    rCtrl.SendMessage(EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textRange));
    if (_istlead(ch[0]))
    {
      ASSERT(ft.chrg.cpMax - ft.chrg.cpMin >= 2);

      if (bNext)
        ft.chrg.cpMin++;
      else
        ft.chrg.cpMax = ft.chrg.cpMin - 1;
    }
  #endif //#ifndef _UNICODE

    if (bNext)  
      ft.chrg.cpMin++;
    else
      ft.chrg.cpMax = ft.chrg.cpMin - 1;
  }

  int nLength = rCtrl.GetLength();
  if (bNext)
    ft.chrg.cpMax = nLength;
  else
    ft.chrg.cpMin = 0;

  DWORD dwFlags = bCase ? SCFIND_MATCHCASE : 0;
  dwFlags |= bWord ? SCFIND_WHOLEWORD : 0;
  if (bRegularExpression)
    dwFlags |= m_bCPP11Regex ? SCFIND_REGEXP | SCFIND_CXX11REGEX : SCFIND_REGEXP;

  if (!bNext)
  {
    //Swap the start and end positions which Scintilla uses to flag backward searches
    int ncpMinTemp = ft.chrg.cpMin;
    ft.chrg.cpMin = ft.chrg.cpMax;
    ft.chrg.cpMax = ncpMinTemp;
  }

  //if we find the text return TRUE
  BOOL bFound = (FindAndSelect(dwFlags, ft) != -1);

  if (bFound)
  {
    m_bChangeFindRange = TRUE;
  }
  //if the original starting point was not the beginning of the buffer
  //and we haven't already been here
  else if (!m_bChangeFindRange)
  {
    m_bChangeFindRange = TRUE;

    if (bNext)
    {
      ft.chrg.cpMin = 0;
      ft.chrg.cpMax = static_cast<Sci_PositionCR>(min(m_lInitialSearchPos + CString(lpszFind).GetLength(), rCtrl.GetLength()));
      m_lInitialSearchPos = 0;
    }
    else
    {
      ft.chrg.cpMin = rCtrl.GetLength();
      ft.chrg.cpMax = static_cast<Sci_PositionCR>(max(m_lInitialSearchPos - CString(lpszFind).GetLength(), 0));
      m_lInitialSearchPos = ft.chrg.cpMin;
    }

    bFound = (FindAndSelect(dwFlags, ft) != -1);
  }
  
#ifdef _UNICODE
  sUTF8Text.ReleaseBuffer();
#else
  sAsciiText.ReleaseBuffer();
#endif //#ifdef _UNICODE
  
  return bFound;	
}

Sci_Position CScintillaView::FindAndSelect(_In_ DWORD dwFlags, _Inout_ Sci_TextToFind& ft)
{
  CScintillaCtrl& rCtrl = GetCtrl();
  Sci_Position index = rCtrl.FindText(dwFlags, &ft);
  if (index != -1) // i.e. we found something
    rCtrl.SetSel(ft.chrgText.cpMin, ft.chrgText.cpMax);
  return index;
}

void CScintillaView::TextNotFound(_In_z_ LPCTSTR /*lpszFind*/, _In_ BOOL /*bNext*/, _In_ BOOL /*bCase*/, _In_ BOOL /*bWord*/, _In_ BOOL /*bRegularExpression*/, _In_ BOOL /*bReplaced*/)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  m_bFirstSearch = TRUE;
  MessageBeep(MB_ICONHAND);
}

void CScintillaView::OnSetFocus(CWnd* /*pOldWnd*/) 
{
  //Validate our parameters
  ASSERT(m_pEdit != nullptr);

  //Give the focus to the child control
  m_pEdit->SetFocus();
}

void CScintillaView::OnSize(UINT nType, int cx, int cy) 
{
  //Validate our parameters
  ASSERT(m_pEdit != nullptr);

  //Let the base class do its thing
  CView::OnSize(nType, cx, cy);

  //Resize the edit control to be the size of the client area
  CRect r;
  GetClientRect(&r);
  m_pEdit->MoveWindow(&r);	
}

BOOL CScintillaView::OnEraseBkgnd(CDC* /*pDC*/) 
{
  //We do nothing here, because the scintilla control takes up the entire 
  //client area of our view;
  return TRUE;
}

void CScintillaView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
  //Change the owner of the Find/Replace dialog if necessary
  if (g_scintillaEditState.pFindReplaceDlg && bActivate && (pActivateView != nullptr) && pActivateView->IsKindOf(RUNTIME_CLASS(CScintillaView)))
    g_scintillaEditState.pFindReplaceDlg->m_fr.hwndOwner = pActivateView->GetSafeHwnd();

  //let the base class do its thing
  CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

int CScintillaView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
  //let the base class do its thing
  if (CView::OnCreate(lpCreateStruct) == -1)
    return -1;
  
  //Create the scintilla edit control  
  CRect r;
  if (!GetCtrl().Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP, r, this, 0))
    return -1;
  
  return 0;
}

#ifdef _DEBUG
void CScintillaView::AssertValid() const
{
  //Let the base class do its thing
  CView::AssertValid();

  //Validate our parameters
  ASSERT_VALID(&m_aPageStart);
}

void CScintillaView::Dump(CDumpContext& dc) const
{
  //Let the base class do its thing
  CView::Dump(dc);

  dc << _T("\nm_aPageStart ") << &m_aPageStart;
  dc << _T("\nbUseROFileAttributeDuringLoading = ") << m_bUseROFileAttributeDuringLoading;

  dc << _T("\n Static Member Data:");
  if (g_scintillaEditState.pFindReplaceDlg != nullptr)
  {
    dc << _T("\npFindReplaceDlg = ") <<	static_cast<void*>(g_scintillaEditState.pFindReplaceDlg);
    dc << _T("\nbFindOnly = ") << g_scintillaEditState.bFindOnly;
  }
  dc << _T("\nstrFind = ") << g_scintillaEditState.strFind;
  dc << _T("\nstrReplace = ") << g_scintillaEditState.strReplace;
  dc << _T("\nbCase = ") << g_scintillaEditState.bCase;
  dc << _T("\nbWord = ") << g_scintillaEditState.bWord;
  dc << _T("\nbNext = ") << g_scintillaEditState.bNext;
  dc << _T("\nbRegularExpression = ") << g_scintillaEditState.bRegularExpression;
}
#endif //#ifdef _DEBUG

void CScintillaView::Serialize(CArchive& ar)
{
  //Validate our parameters
  ASSERT_VALID(this);
  
  CScintillaCtrl& rCtrl = GetCtrl();

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
    }
    while (nBytesRead);

    //Set the read only state if required
    if (m_bUseROFileAttributeDuringLoading && ((GetFileAttributes(pFile->GetFilePath()) & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY))
      rCtrl.SetReadOnly(TRUE);
    else
      rCtrl.SetReadOnly(FALSE);

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
    for (int i=0; i<nDocLength; i += 4095) //4095 because data will be returned nullptr terminated
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
  }
}

void CScintillaView::OnFilePageSetup() 
{
  //Display a standard page setup dialog
  CPageSetupDialog dlg;

  //Allow the margin settings to be tweaked
  dlg.m_psd.Flags |= PSD_MARGINS;

  //Are we using the metric or imperial system	
  if (m_bUsingMetric)
    dlg.m_psd.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
  else
    dlg.m_psd.Flags |= PSD_INTHOUSANDTHSOFINCHES;

  if (m_bPersistMarginSettings)
    LoadMarginSettings();

  //Set the current margin settings to the current value from m_rectMargin 
  dlg.m_psd.rtMargin = m_rMargin;

  //get the current device from the app
  PRINTDLG pd;
  pd.hDevNames = nullptr;
  pd.hDevMode = nullptr;
  CWinApp* pApp = AfxGetApp();
  pApp->GetPrinterDeviceDefaults(&pd);
  dlg.m_psd.hDevNames = pd.hDevNames;
  dlg.m_psd.hDevMode = pd.hDevMode;

  if (dlg.DoModal() == IDOK)
  {
    //Save the new margin value in to the member variable
    m_rMargin = dlg.m_psd.rtMargin;

    if (m_bPersistMarginSettings)
      SaveMarginSettings();

    //Update the printer settings
    pApp->SelectPrinter(dlg.m_psd.hDevNames, dlg.m_psd.hDevMode);
  }
}

void CScintillaView::OnStyleNeeded(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnCharAdded(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnSavePointReached(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnSavePointLeft(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnModifyAttemptRO(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnDoubleClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnUpdateUI(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnModified(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnMacroRecord(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnMarginClick(_Inout_ SCNotification* pSCNotification)
{
  //By default get the line where the click occurred and toggle its fold state
  CScintillaCtrl& rCtrl = GetCtrl();
  int nLine = rCtrl.LineFromPosition(pSCNotification->position);
  rCtrl.ToggleFold(nLine);
}

void CScintillaView::OnMarginRightClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnNeedShown(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnPainted(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnUserListSelection(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnDwellStart(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnDwellEnd(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnZoom(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnHotSpotClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnHotSpotDoubleClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnCallTipClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnAutoCSelection(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnIndicatorClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnIndicatorRelease(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnAutoCCharDeleted(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnAutoCCancelled(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnHotspotReleaseClick(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnFocusIn(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnFocusOut(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnAutoCCompleted(_Inout_ SCNotification* /*pSCNotification*/)
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnChange()
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnSetFocus()
{
  //By default do nothing, derived classes may want to do something
}

void CScintillaView::OnKillFocus()
{
  //By default do nothing, derived classes may want to do something
}

BOOL CScintillaView::OnCommand(WPARAM wParam, LPARAM lParam)
{
  HWND hWndControl = reinterpret_cast<HWND>(lParam);
  
  if (hWndControl == GetCtrl().GetSafeHwnd())
  {
    WORD wNotification = HIWORD(wParam);
    switch (wNotification)
    {
      case SCEN_CHANGE:
      {
        OnChange();
        break;
      }
      case SCEN_SETFOCUS:
      {
        OnSetFocus();
        break;
      }
      case SCEN_KILLFOCUS:
      {
        OnKillFocus();
        break;
      }
      default:
      {
        break;
      }
    }

    return TRUE;
  }

  //let the base class do its thing
  return CView::OnCommand(wParam, lParam);
}

BOOL CScintillaView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
  NMHDR* pNMHdr = reinterpret_cast<NMHDR*>(lParam);
  AFXASSUME(pNMHdr != nullptr);

  //Is it a notification from the embedded control
  CScintillaCtrl& rCtrl = GetCtrl();
  if (pNMHdr->hwndFrom == rCtrl.GetSafeHwnd())
  {
    SCNotification* pSCNotification = reinterpret_cast<SCNotification*>(lParam);
    switch (pNMHdr->code)
    {
      case SCN_STYLENEEDED:
      {
        OnStyleNeeded(pSCNotification);
        break;
      }
      case SCN_CHARADDED:
      {
        OnCharAdded(pSCNotification);
        break;
      }
      case SCN_SAVEPOINTREACHED:
      {
        OnSavePointReached(pSCNotification);
        break;
      }
      case SCN_SAVEPOINTLEFT:
      {
        OnSavePointLeft(pSCNotification);
        break;
      }
      case SCN_MODIFYATTEMPTRO:
      {
        OnModifyAttemptRO(pSCNotification);
        break;
      }
      case SCN_DOUBLECLICK:
      {
        OnDoubleClick(pSCNotification);
        break;
      }
      case SCN_UPDATEUI:
      {
        OnUpdateUI(pSCNotification);
        break;
      }
      case SCN_MODIFIED:
      {
        OnModified(pSCNotification);
        break;
      }
      case SCN_MACRORECORD:
      {
        OnMacroRecord(pSCNotification);
        break;
      }
      case SCN_MARGINCLICK:
      {
        OnMarginClick(pSCNotification);
        break;
      }
      case SCN_MARGINRIGHTCLICK:
      {
        OnMarginRightClick(pSCNotification);
        break;
      }
      case SCN_NEEDSHOWN:
      {
        OnNeedShown(pSCNotification);
        break;
      }
      case SCN_PAINTED:
      {
        OnPainted(pSCNotification);
        break;
      }
      case SCN_USERLISTSELECTION:
      {
        OnUserListSelection(pSCNotification);
        break;
      }
      case SCN_DWELLSTART:
      {
        OnDwellStart(pSCNotification);
        break;
      }
      case SCN_DWELLEND:
      {
        OnDwellEnd(pSCNotification);
        break;
      }
      case SCN_ZOOM:
      {
        OnZoom(pSCNotification);
        break;
      }
      case SCN_HOTSPOTCLICK:
      {
        OnHotSpotClick(pSCNotification);
        break;
      }
      case SCN_HOTSPOTDOUBLECLICK:
      {
        OnHotSpotDoubleClick(pSCNotification);
        break;
      }
      case SCN_HOTSPOTRELEASECLICK:
      {
        OnHotspotReleaseClick(pSCNotification);
        break;
      }
      case SCN_INDICATORCLICK:
      {
        OnIndicatorClick(pSCNotification);
        break;
      }
      case SCN_INDICATORRELEASE:
      {
        OnIndicatorRelease(pSCNotification);
        break;
      }
      case SCN_CALLTIPCLICK:
      {
        OnCallTipClick(pSCNotification);
        break;
      }
      case SCN_AUTOCSELECTION:
      {
        OnAutoCSelection(pSCNotification);
        break;
      }
      case SCN_AUTOCCANCELLED:
      {
        OnAutoCCancelled(pSCNotification);
        break;
      }
      case SCN_AUTOCCHARDELETED:
      {
        OnAutoCCharDeleted(pSCNotification);
        break;
      }
      case SCN_FOCUSIN:
      {
        OnFocusIn(pSCNotification);
        break;
      }
      case SCN_FOCUSOUT:
      {
        OnFocusOut(pSCNotification);
        break;
      }
      case SCN_AUTOCCOMPLETED:
      {
        OnAutoCCompleted(pSCNotification);
        break;
      }
      default:
      {
        break;
      }
    }
    return TRUE; // we processed the message
  }
  else
  {
    //let the base class do its thing
    return CView::OnNotify(wParam, lParam, pResult);
  }
}


IMPLEMENT_DYNAMIC(CScintillaDoc, CDocument)

CScintillaDoc::CScintillaDoc()
{
  ASSERT_VALID(this);
}

CScintillaView* CScintillaDoc::GetView() const
{
  //find the first view - if there are no views
  //we must return nullptr
  POSITION pos = GetFirstViewPosition();
  if (pos == nullptr)
    return nullptr;

  //find the first view that is a CScintillaView
  while (pos != nullptr)
  {
    CView* pView = GetNextView(pos);
    if (pView->IsKindOf(RUNTIME_CLASS(CScintillaView)))
      return static_cast<CScintillaView*>(pView);
  }

  //can't find one, then return nullptr
  return nullptr;
}

void CScintillaDoc::SetModifiedFlag(BOOL bModified)
{
  CScintillaView* pView = GetView();
  AFXASSUME(pView != nullptr);

  if (bModified == FALSE)
    pView->GetCtrl().SetSavePoint();

  m_bModified = bModified;
}

BOOL CScintillaDoc::IsModified()
{
  CScintillaView* pView = GetView();
  AFXASSUME(pView != nullptr);

  return m_bModified || pView->GetCtrl().GetModify();
}

void CScintillaDoc::DeleteContents()
{
  //let the base class do its thing
  CDocument::DeleteContents();

  //Ask our accompanying view to delete its contents
  CWaitCursor wait;
  CScintillaView* pView = GetView();
  if (pView)
    pView->DeleteContents();
}

void CScintillaDoc::Serialize(CArchive& ar)
{
  CScintillaView* pView = GetView();
  AFXASSUME(pView != nullptr);
  
  pView->Serialize(ar);
}

BOOL CScintillaDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
  //Let the base class do its thing
  BOOL bSuccess = CDocument::OnSaveDocument(lpszPathName);
  if (bSuccess)
  {
    CScintillaView* pView = GetView();
    AFXASSUME(pView != nullptr);
    
    CScintillaCtrl& rCtrl = pView->GetCtrl();

    //Tell the control that the document has now been saved
    rCtrl.SetSavePoint();
    rCtrl.SetReadOnly(FALSE);
  }

  return bSuccess;
} 

#ifdef _DEBUG
void CScintillaDoc::AssertValid() const
{
  //Let the base class do its thing
  CDocument::AssertValid();
}

void CScintillaDoc::Dump(CDumpContext& dc) const
{
  //let the base class do its thing
  CDocument::Dump(dc);
}
#endif //#ifdef _DEBUG
