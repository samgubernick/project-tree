
// Project TreeDlg.h : header file
//

#pragma once

#include <set>

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

	void RefreshVirtualCombinedView();

	void LoadCombinedDirectory(
		HTREEITEM hParent,
		const CString & srcDir,
		const CString & includeDir,
		const CString & relativePath
	);
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

	CString m_copiedItemPath;
	bool m_isCopiedItemFolder;
	bool m_isCutOperation;
	CString idePath;

	UINT isFocused;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG * pMsg);

	void SaveExpandedFolders();
	void RestoreExpandedFolders();
	void SaveExpandedStateRecursive(HTREEITEM hItem, std::set<CString> & expandedPaths);
	void RestoreExpandedStateRecursive(HTREEITEM hItem, const std::set<CString> & expandedPaths);

	void LoadCombinedView(HTREEITEM hParent);
	void LoadCombinedSubfolder(HTREEITEM hParent, const CString & folderName);
	void GetExpandedItems(HTREEITEM hItem, CStringArray & paths);
	void SaveWindowState();
	void RestoreWindowState();

	auto CreateNewFolder() -> void;

	auto CreateNewFile() -> void;

	auto DeleteSelectedItem() -> void;
	void UpdateFileInVirtualView(const CString & oldPath, const CString & newPath, const CString & newName);
	void UpdateFileInRealView(const CString & oldPath, const CString & newPath, const CString & newName);
	void SearchAndUpdateFile(HTREEITEM hItem, const CString & oldPath, const CString & newPath, const CString & newName);
	void SearchAndDeleteItem(HTREEITEM hItem, const CString & pathToDelete);
	bool IsItemInVirtualView(HTREEITEM hItem);
	HTREEITEM FindItemParent(HTREEITEM hItem, const CString & pathToFind);

	afx_msg void OnPaint();
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnClose();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTreeDblClick(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeItemExpanding(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg HBRUSH OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor);
	afx_msg void OnTreeRightClick(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeBeginDrag(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeEndDrag(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeBeginLabelEdit(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTreeEndLabelEdit(NMHDR * pNMHDR, LRESULT * pResult);\
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd * pWndOther, BOOL bMinimized);
	afx_msg void OnCustomDraw(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg LRESULT OnRestoreExpandedState(WPARAM wParam, LPARAM lParam);
	#define WM_RESTORE_EXPANDED_STATE (WM_APP + 1)


	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTvnSelchangedTree2(NMHDR * pNMHDR, LRESULT * pResult);
	afx_msg void OnTvnSelchangedTreeControl(NMHDR * pNMHDR, LRESULT * pResult);
};
