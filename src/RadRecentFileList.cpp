#include "stdafx.h"
#include "RadRecentFileList.h"
#include "RadNotepad.h"

//BOOL AFXAPI AfxFullPath(_Pre_notnull_ _Post_z_ LPTSTR lpszPathOut, LPCTSTR lpszFileIn);
BOOL AFXAPI AfxComparePath(LPCTSTR lpszPath1, LPCTSTR lpszPath2);

UINT AFXAPI AfxGetFileTitle(LPCTSTR lpszPathName, _Out_writes_(nMax) LPTSTR lpszTitle, UINT nMax);
UINT AFXAPI AfxGetFileName(LPCTSTR lpszPathName, _Out_writes_opt_(nMax) LPTSTR lpszTitle, UINT nMax);

AFX_STATIC void AFXAPI _AfxAbbreviateName(_Inout_z_ LPTSTR lpszCanon, int cchMax, BOOL bAtLeastName)
{
	ENSURE_ARG(AfxIsValidString(lpszCanon));

	int cchFullPath, cchFileName, cchVolName;
	const TCHAR* lpszCur;
	const TCHAR* lpszBase;
	const TCHAR* lpszFileName;

	lpszBase = lpszCanon;
	cchFullPath = AtlStrLen(lpszCanon);

	cchFileName = AfxGetFileName(lpszCanon, NULL, 0) - 1;
	lpszFileName = lpszBase + (cchFullPath - cchFileName);

	// If cchMax is more than enough to hold the full path name, we're done.
	// This is probably a pretty common case, so we'll put it first.
	if (cchMax >= cchFullPath)
		return;

	// If cchMax isn't enough to hold at least the basename, we're done
	if (cchMax < cchFileName)
	{
		if (!bAtLeastName)
			lpszCanon[0] = _T('\0');
		else
			Checked::tcscpy_s(lpszCanon, cchFullPath + 1, lpszFileName);
		return;
	}

	// Calculate the length of the volume name.  Normally, this is two characters
	// (e.g., "C:", "D:", etc.), but for a UNC name, it could be more (e.g.,
	// "\\server\share").
	//
	// If cchMax isn't enough to hold at least <volume_name>\...\<base_name>, the
	// result is the base filename.

	const TCHAR* s = _tcschr(lpszBase, _T(':'));
	lpszCur = s != NULL ? s + 1 : lpszBase;                 // Skip "http:"

	if (lpszCur[0] == '/' && lpszCur[1] == '/') // UNC pathname
	{
		lpszCur += 2;
		// First skip to the '\' between the server name and the share name,
		while (*lpszCur != '/')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	// if a UNC get the share name, if a drive get at least one directory
	ASSERT(*lpszCur == '/');
	// make sure there is another directory, not just c:\filename.ext
	if (cchFullPath - cchFileName > 3)
	{
		lpszCur = _tcsinc(lpszCur);
		while (*lpszCur != '/')
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		}
	}
	ASSERT(*lpszCur == '/');

	cchVolName = int(lpszCur - lpszBase);
	if (cchMax < cchVolName + 5 + cchFileName)
	{
		Checked::tcscpy_s(lpszCanon, cchFullPath + 1, lpszFileName);
		return;
	}

	// Now loop through the remaining directory components until something
	// of the form <volume_name>\...\<one_or_more_dirs>\<base_name> fits.
	//
	// Assert that the whole filename doesn't fit -- this should have been
	// handled earlier.

	ASSERT(cchVolName + AtlStrLen(lpszCur) > cchMax);
	while (cchVolName + 4 + AtlStrLen(lpszCur) > cchMax)
	{
		do
		{
			lpszCur = _tcsinc(lpszCur);
			ASSERT(*lpszCur != '\0');
		} while (*lpszCur != '/');
	}

	// Form the resultant string and we're done.
	int cch;
	if (cchVolName >= 0 && cchVolName < cchMax)
		cch = cchVolName;
	else cch = cchMax;
	Checked::memcpy_s(lpszCanon + cch, cchFullPath + 1 - cch, _T("\\..."), sizeof(_T("\\...")));
	Checked::tcscat_s(lpszCanon, cchFullPath + 1, lpszCur);
}

CRadRecentFileList::CRadRecentFileList(const CRecentFileList& rfl)
    : CRecentFileList(rfl)
{
}

void CRadRecentFileList::Add(LPCTSTR lpszPathName)
{
	if (!CRadNotepadApp::IsInternetUrl(lpszPathName))
		CRecentFileList::Add(lpszPathName);
	else
	{
		ASSERT(m_arrNames != NULL);
		ASSERT(AfxIsValidString(lpszPathName));

		// update the MRU list, if an existing MRU string matches file name
		int iMRU;
		for (iMRU = 0; iMRU < m_nSize - 1; iMRU++)
		{
			if (AfxComparePath(m_arrNames[iMRU], lpszPathName))
				break;      // iMRU will point to matching entry
		}
		// move MRU strings before this one down
		for (; iMRU > 0; iMRU--)
		{
			ASSERT(iMRU > 0);
			ASSERT(iMRU < m_nSize);
			m_arrNames[iMRU] = m_arrNames[iMRU - 1];
		}
		// place this one at the beginning
		m_arrNames[0] = lpszPathName;

		// any addition to the MRU should also be added to recent docs
#ifdef UNICODE
		SHAddToRecentDocs(SHARD_PATHW, lpszPathName);
#else
		SHAddToRecentDocs(SHARD_PATHA, lpszPathName);
#endif
	}
}

BOOL CRadRecentFileList::GetDisplayName(CString& strName, int nIndex,
	LPCTSTR lpszCurDir, int nCurDir, BOOL bAtLeastName) const
{
	ENSURE_ARG(lpszCurDir == NULL || AfxIsValidString(lpszCurDir, nCurDir));

	ASSERT(m_arrNames != NULL);
	ENSURE_ARG(nIndex < m_nSize);
	if (lpszCurDir == NULL || m_arrNames[nIndex].IsEmpty())
		return FALSE;

	if (!CRadNotepadApp::IsInternetUrl(m_arrNames[nIndex]))
		return CRecentFileList::GetDisplayName(strName, nIndex, lpszCurDir, nCurDir, bAtLeastName);
	else
	{
		int nLenName = m_arrNames[nIndex].GetLength();
		LPTSTR lpch = strName.GetBuffer(nLenName + 1);
		if (lpch == NULL)
		{
			AfxThrowMemoryException();
		}
		Checked::tcsncpy_s(lpch, nLenName + 1, m_arrNames[nIndex], _TRUNCATE);
		// nLenDir is the length of the directory part of the full path
		int nLenDir = nLenName - (AfxGetFileName(lpch, NULL, 0) - 1);
		if (m_nMaxDisplayLength != -1)
		{
			// strip the extension if the system calls for it
			TCHAR szTemp[_MAX_PATH];
			AfxGetFileTitle(lpch + nLenDir, szTemp, _countof(szTemp));
			Checked::tcsncpy_s(lpch + nLenDir, nLenName + 1 - nLenDir, szTemp, _TRUNCATE);

			// abbreviate name based on what will fit in limited space
			_AfxAbbreviateName(lpch, m_nMaxDisplayLength, bAtLeastName);
		}
		strName.ReleaseBuffer();
		return TRUE;
	}
}
