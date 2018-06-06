
#pragma once

class CRadNotepadDoc : public CScintillaDoc
{
protected: // create from serialization only
	CRadNotepadDoc();
	DECLARE_DYNCREATE(CRadNotepadDoc)

// Attributes
public:
    int GetLineEndingMode() const { return m_nLineEndingMode; }

// Operations
public:
    void CheckUpdated();
    void CheckReadOnly();
    void SyncModified();

// Overrides
public:
	virtual BOOL OnNewDocument() override;
    virtual BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	virtual void Serialize(CArchive& ar) override;
    virtual void SetTitle(LPCTSTR lpszTitle) override;
    virtual void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE) override;
    virtual CFile* GetFile(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError) override;
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CRadNotepadDoc();
#ifdef _DEBUG
	virtual void AssertValid() const override;
	virtual void Dump(CDumpContext& dc) const override;
#endif

protected:
    Encoding m_eEncoding;
    int m_nLineEndingMode;
    FILETIME m_ftWrite;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
    afx_msg void OnFileRevert();
    afx_msg void OnUpdateFileRevert(CCmdUI *pCmdUI);
    afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
    afx_msg void OnFileReadOnly();
    afx_msg void OnUpdateFileReadOnly(CCmdUI *pCmdUI);
    afx_msg void OnEncoding(UINT nID);
    afx_msg void OnUpdateEncoding(CCmdUI *pCmdUI);
};
