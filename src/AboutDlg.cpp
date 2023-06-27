#include "stdafx.h"
#include "AboutDlg.h"

#include <tchar.h>
#include "RadNotepad.h"
#include "..\Resource.h"

struct LANGANDCODEPAGE
{
    WORD wLanguage;
    WORD wCodePage;
};

struct VersionInfo
{
    TCHAR Product[100];
    TCHAR Version[100];
    TCHAR Copyright[100];
};

static void GetVersionData(HINSTANCE hInstance, VersionInfo* vi)
{
    TCHAR	FileName[1024];
    GetModuleFileName(hInstance, FileName, 1024);

    DWORD	Dummy = 0;
    DWORD	Size = GetFileVersionInfoSize(FileName, &Dummy);

    void* Info = Size > 0 ? malloc(Size) : nullptr;

    if (Info != nullptr)
    {
        GetFileVersionInfoEx(FILE_VER_GET_LOCALISED, FileName, 0, Size, Info);

        LANGANDCODEPAGE* lpTranslate;
        UINT	cbTranslate;
        VerQueryValue(Info, TEXT("\\VarFileInfo\\Translation"), (LPVOID*) &lpTranslate, &cbTranslate);

        for (int i = 0; i < (int) (cbTranslate / sizeof(struct LANGANDCODEPAGE)); i++)
        {
            TCHAR SubBlock[50];
            TCHAR	*String;
            UINT	Length;

            String = nullptr;
            _stprintf_s(SubBlock, TEXT("\\StringFileInfo\\%04x%04x\\ProductName"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            VerQueryValue(Info, SubBlock, (LPVOID *) &String, &Length);
            if (String != nullptr)
                _tcscpy_s(vi->Product, String);

            String = nullptr;
            _stprintf_s(SubBlock, TEXT("\\StringFileInfo\\%04x%04x\\FileVersion"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            VerQueryValue(Info, SubBlock, (LPVOID *) &String, &Length);
            if (String != nullptr)
                _tcscpy_s(vi->Version, String);

            String = nullptr;
            _stprintf_s(SubBlock, TEXT("\\StringFileInfo\\%04x%04x\\LegalCopyright"), lpTranslate[i].wLanguage, lpTranslate[i].wCodePage);
            VerQueryValue(Info, SubBlock, (LPVOID *) &String, &Length);
            if (String != nullptr)
                _tcscpy_s(vi->Copyright, String);

            // TODO Exit if found one
        }
        free(Info);
    }
    else
    {
        _tcscpy_s(vi->Product, TEXT("Unknown"));
        _tcscpy_s(vi->Version, TEXT("Unknown"));
        _tcscpy_s(vi->Copyright, TEXT("Unknown"));
    }
}

CAboutDlg::CAboutDlg() : CRadDialog(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CRadDialog::DoDataExchange(pDX);
    if (!pDX->m_bSaveAndValidate)
    {
        VersionInfo viRadNotepad, viScintilla, viLexilla;
        GetVersionData(NULL, &viRadNotepad);
        GetVersionData(theApp.m_hSciDLL, &viScintilla);
        GetVersionData(theApp.m_hLexDLL, &viLexilla);

        TCHAR ProductVersion[100];
        _stprintf_s(ProductVersion, TEXT("%s, Version %s"), viRadNotepad.Product, viRadNotepad.Version);
        SetDlgItemText(ID_VERSION, ProductVersion);
        SetDlgItemText(ID_COPYRIGHT, viRadNotepad.Copyright);

        _stprintf_s(ProductVersion, TEXT("%s, Version %s"), viScintilla.Product, viScintilla.Version);
        SetDlgItemText(ID_VERSION_SCINTILLA, ProductVersion);
        SetDlgItemText(ID_COPYRIGHT_SCINTILLA, viScintilla.Copyright);

        _stprintf_s(ProductVersion, TEXT("%s, Version %s"), viLexilla.Product, viLexilla.Version);
        SetDlgItemText(ID_VERSION_LEXILLA, ProductVersion);
        SetDlgItemText(ID_COPYRIGHT_LEXILLA, viLexilla.Copyright);
    }
}

BEGIN_MESSAGE_MAP(CAboutDlg, CRadDialog)
END_MESSAGE_MAP()
