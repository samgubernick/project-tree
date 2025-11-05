
// Project TreeDlg.h : header file
//

#pragma once


// CProjectTreeDlg dialog
class CProjectTreeDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CProjectTreeDlg)

// Construction
public:
	CProjectTreeDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CProjectTreeDlg();

// Dialog Data
//#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROJECT_TREE_DIALOG };
//#endif

	auto PopulateTree() -> void;
	void LoadDirectoryContents(HTREEITEM hParent, const CString & strPath);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Tree control for file explorer
	CTreeCtrl m_treeCtrl;

	CImageList m_imageList;

	CFont m_treeFont;

	CString m_rootPath;

	BOOL m_bDragging;
	HTREEITEM m_hDragItem;
	CImageList * m_pDragImageList;

	HTREEITEM m_hRenameItem;
	CString m_originalName;


	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG * pMsg);

	void SaveExpandedFolders();
	void RestoreExpandedFolders();
	void GetExpandedItems(HTREEITEM hItem, CStringArray & paths);
	void SaveWindowState();
	void RestoreWindowState();

	auto CreateNewFolder() -> void;

	auto CreateNewFile() -> void;

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnClose();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTreeDblClick(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeExpanding(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg HBRUSH OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor);
	afx_msg void OnTreeRightClick(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeBeginDrag(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeEndDrag(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeBeginLabelEdit(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeEndLabelEdit(NMHDR * pNMHDR, LRESULT * pResult);\
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTvnSelchangedTree2(NMHDR * pNMHDR, LRESULT * pResult);
};
