
// Project TreeDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Project Tree.h"
#include "Project TreeDlg.h"
#include "afxdialogex.h"

#include <shellapi.h>

#include <functional>

IMPLEMENT_DYNAMIC(CProjectTreeDlg, CDialogEx)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CProjectTreeDlg dialog


CProjectTreeDlg::~CProjectTreeDlg() = default;

CProjectTreeDlg::CProjectTreeDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PROJECT_TREE_DIALOG, pParent),
	m_bDragging(FALSE),
	m_hDragItem(nullptr),
	m_pDragImageList(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProjectTreeDlg::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;

	PostQuitMessage(0);
}

BOOL CProjectTreeDlg::PreTranslateMessage(MSG * pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		// Open the selected item
		HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
		if (hSelectedItem)
		{
			CString * pFilePath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
			if (pFilePath)
			{
				// Check if it's a folder (ends with backslash)
				if (!pFilePath->IsEmpty() && pFilePath->GetAt(pFilePath->GetLength() - 1) == _T('\\'))
				{
					// It's a folder, toggle expand/collapse
					UINT state = m_treeCtrl.GetItemState(hSelectedItem, TVIS_EXPANDED);
					if (state & TVIS_EXPANDED)
					{
						m_treeCtrl.Expand(hSelectedItem, TVE_COLLAPSE);
					}
					else
					{
						m_treeCtrl.Expand(hSelectedItem, TVE_EXPAND);
					}
				}
				else
				{
					// It's a file, open it
					ShellExecute(nullptr, _T("open"), *pFilePath, nullptr, nullptr, SW_SHOW);
				}
			}
		}
		return TRUE;  // Prevent default Enter behavior
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
	{
		return TRUE;
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

afx_msg void CProjectTreeDlg::OnClose()
{
	SaveExpandedFolders();
	SaveWindowState();
	DestroyWindow();
}

void CProjectTreeDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// Resize tree control to fill dialog
	if (m_treeCtrl.GetSafeHwnd())
	{
		RECT rc;
		GetClientRect(&rc);
		m_treeCtrl.MoveWindow(&rc);
	}
}

void CProjectTreeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CONTROL, m_treeCtrl);
}

BEGIN_MESSAGE_MAP(CProjectTreeDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE2, &CProjectTreeDlg::OnTvnSelchangedTree2)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_CONTROL, OnTreeDblClick)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE_CONTROL, OnTreeExpanding)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CONTROL, OnTreeRightClick)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE_CONTROL, OnTreeBeginDrag)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE_CONTROL, OnTreeBeginLabelEdit)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE_CONTROL, OnTreeEndLabelEdit)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// CProjectTreeDlg message handlers

BOOL CProjectTreeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	SetWindowText(_T("D:/lanterns"));

	BOOL value = TRUE;
	DwmSetWindowAttribute(GetSafeHwnd(), DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

	RestoreWindowState();

	if (m_treeCtrl.GetSafeHwnd())
	{
		// Create image list
		m_imageList.Create(32, 32, ILC_COLOR32 | ILC_MASK, 0, 10);

		// Set the image list to the tree control
		m_treeCtrl.SetImageList(&m_imageList, TVSIL_NORMAL);

		m_treeCtrl.SetItemHeight(40);

		// Create and set custom font
		m_treeFont.CreatePointFont(16 * 10, _T("Segoe UI"));  // 12 point Segoe UI
		m_treeCtrl.SetFont(&m_treeFont);

		m_treeCtrl.SetBkColor(RGB(45, 45, 45));
		m_treeCtrl.SetTextColor(RGB(255, 255, 255));
		//SetWindowTheme(m_treeCtrl.GetSafeHwnd(), L"Explorer", nullptr);
		SetWindowTheme(m_treeCtrl.GetSafeHwnd(), L"DarkMode_Explorer", nullptr);

		PopulateTree();
		RestoreExpandedFolders();

		RECT rc;
		GetClientRect(&rc);
		m_treeCtrl.MoveWindow(&rc);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CProjectTreeDlg::RestoreExpandedFolders()
{
	// Get AppData/Roaming path
	TCHAR appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
	{
		CString configFile;
		configFile.Format(_T("%s\\ProjectTree\\ExpandedFolders.txt"), appDataPath);

		// Read from file
		FILE * pFile = nullptr;
		if (_tfopen_s(&pFile, configFile, _T("r")) == 0 && pFile)
		{
			TCHAR folderPath[MAX_PATH];

			while (_fgetts(folderPath, MAX_PATH, pFile))
			{
				// Remove newline
				int len = _tcslen(folderPath);
				if (len > 0 && folderPath[len - 1] == _T('\n'))
				{
					folderPath[len - 1] = _T('\0');
				}

				CString folderToExpand = folderPath;

				// Find and expand this folder in the tree
				HTREEITEM hRoot = m_treeCtrl.GetRootItem();
				if (hRoot)
				{
					std::function<bool(HTREEITEM)> findAndExpand = [&](HTREEITEM hItem) -> bool
					{
						while (hItem)
						{
							CString * pPath = (CString *)m_treeCtrl.GetItemData(hItem);
							if (pPath && *pPath == folderToExpand)
							{
								m_treeCtrl.Expand(hItem, TVE_EXPAND);
								return true;
							}

							HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
							if (hChild && findAndExpand(hChild))
								return true;

							hItem = m_treeCtrl.GetNextItem(hItem, TVGN_NEXT);
						}
						return false;
					};

					findAndExpand(hRoot);
				}
			}

			fclose(pFile);
		}
	}
}

void CProjectTreeDlg::GetExpandedItems(HTREEITEM hItem, CStringArray & paths)
{
	while (hItem)
	{
		// Check if this item is expanded
		UINT state = m_treeCtrl.GetItemState(hItem, TVIS_EXPANDED);
		if (state & TVIS_EXPANDED)
		{
			CString * pPath = (CString *)m_treeCtrl.GetItemData(hItem);
			if (pPath && !pPath->IsEmpty())
			{
				paths.Add(*pPath);
			}

			// Recursively check children
			HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
			if (hChild)
			{
				GetExpandedItems(hChild, paths);
			}
		}

		hItem = m_treeCtrl.GetNextItem(hItem, TVGN_NEXT);
	}
}

void CProjectTreeDlg::SaveExpandedFolders()
{
	// Get AppData/Roaming path
	TCHAR appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
	{
		// Create subdirectory if it doesn't exist
		CString configDir;
		configDir.Format(_T("%s\\ProjectTree"), appDataPath);
		CreateDirectory(configDir, NULL);

		// Create config file path
		CString configFile;
		configFile.Format(_T("%s\\ExpandedFolders.txt"), configDir);

		// Collect expanded folders
		CStringArray expandedPaths;
		HTREEITEM hRoot = m_treeCtrl.GetRootItem();
		if (hRoot)
		{
			GetExpandedItems(hRoot, expandedPaths);
		}

		// Write to file
		FILE * pFile = nullptr;
		if (_tfopen_s(&pFile, configFile, _T("w")) == 0 && pFile)
		{
			for (int i = 0; i < expandedPaths.GetCount(); i++)
			{
				_ftprintf(pFile, _T("%s\n"), expandedPaths[i]);
			}
			fclose(pFile);
		}
	}
}

void CProjectTreeDlg::PopulateTree()
{
	m_treeCtrl.DeleteAllItems();

	m_rootPath = _T("C:\\source\\");
	CString const includePath = m_rootPath + "include\\";
	CString const shaderPath = m_rootPath + "shader_angle\\";
	CString const srcPath = m_rootPath + "src\\";

	CString displayName = m_rootPath;
	int lastBackslash = displayName.ReverseFind(_T('\\'));
	if (lastBackslash != -1)
	{
		displayName = displayName.Mid(lastBackslash + 1);
	}
	if (displayName.IsEmpty())
	{
		displayName = m_rootPath;
	}

	SHFILEINFO sfi = {0};
	SHGetFileInfo(includePath, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(SHFILEINFO),
				 SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
	int folderIcon = m_imageList.Add(sfi.hIcon);
	DestroyIcon(sfi.hIcon);

	// Create root items with just the folder names
	HTREEITEM hInclude = m_treeCtrl.InsertItem(_T("include"), folderIcon, folderIcon);
	m_treeCtrl.SetItemData(hInclude, (DWORD_PTR)new CString(includePath));
	m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hInclude);

	HTREEITEM hShader = m_treeCtrl.InsertItem(_T("shader_angle"), folderIcon, folderIcon);
	m_treeCtrl.SetItemData(hShader, (DWORD_PTR)new CString(shaderPath));
	m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hShader);

	HTREEITEM hSrc = m_treeCtrl.InsertItem(_T("src"), folderIcon, folderIcon);
	m_treeCtrl.SetItemData(hSrc, (DWORD_PTR)new CString(srcPath));
	m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hSrc);
}

void CProjectTreeDlg::LoadDirectoryContents(HTREEITEM hParent, const CString & strPath)
{
	// Remove dummy item if it exists
	HTREEITEM hFirstChild = m_treeCtrl.GetChildItem(hParent);
	if (hFirstChild && m_treeCtrl.GetItemText(hFirstChild).IsEmpty())
	{
		m_treeCtrl.DeleteItem(hFirstChild);
	}

	WIN32_FIND_DATA findData;
	HANDLE hFind;
	CString searchPath = strPath + _T("*.*");

	hFind = FindFirstFile(searchPath, &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		// Skip "." and ".."
		if (_tcscmp(findData.cFileName, _T(".")) == 0 ||
			_tcscmp(findData.cFileName, _T("..")) == 0)
			continue;

		CString fullPath = strPath + findData.cFileName;

		// Get system icon for this item
		SHFILEINFO sfi = {0};
		SHGetFileInfo(fullPath, 0, &sfi, sizeof(SHFILEINFO),
					 SHGFI_ICON | SHGFI_SMALLICON);

		int iconIndex = m_imageList.Add(sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			// It's a folder
			HTREEITEM hFolder = m_treeCtrl.InsertItem(findData.cFileName, iconIndex, iconIndex, hParent);
			m_treeCtrl.SetItemData(hFolder, (DWORD_PTR)new CString(fullPath + _T("\\")));

			// Add dummy child so expand arrow appears
			m_treeCtrl.InsertItem(_T(""), iconIndex, iconIndex, hFolder);
		}
		else
		{
			// It's a file
			HTREEITEM hFile = m_treeCtrl.InsertItem(findData.cFileName, iconIndex, iconIndex, hParent);
			m_treeCtrl.SetItemData(hFile, (DWORD_PTR)new CString(fullPath));
		}
	} while (FindNextFile(hFind, &findData));

	FindClose(hFind);
}

afx_msg void CProjectTreeDlg::OnTreeExpanding(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	if (pNMTreeView->action == TVE_EXPAND)
	{
		HTREEITEM hItem = pNMTreeView->itemNew.hItem;
		CString * pPath = (CString *)m_treeCtrl.GetItemData(hItem);

		if (pPath)
		{
			// Check if already loaded (has real children, not dummy)
			HTREEITEM hFirstChild = m_treeCtrl.GetChildItem(hItem);
			if (hFirstChild && m_treeCtrl.GetItemText(hFirstChild).IsEmpty())
			{
				// Still has dummy, so load for real
				LoadDirectoryContents(hItem, *pPath);
			}
		}
	}

	*pResult = 0;
}

afx_msg void CProjectTreeDlg::OnTreeDblClick(NMHDR * pNMHDR, LRESULT * pResult)
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();

	if (hSelectedItem)
	{
		CString * pFilePath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
		if (pFilePath)
		{
			// Check if it's a file (doesn't end with backslash)
			if (!pFilePath->IsEmpty() && pFilePath->GetAt(pFilePath->GetLength() - 1) != _T('\\'))
			{
				ShellExecute(nullptr, _T("open"), *pFilePath, nullptr, nullptr, SW_SHOW);
			}
		}
	}

	*pResult = 0;
}

HBRUSH CProjectTreeDlg::OnCtlColor(CDC * pDC, CWnd * pWnd, UINT nCtlColor)
{
	static CBrush darkBrush(RGB(30, 30, 30));  // Dark background
	static CBrush treeBrush(RGB(45, 45, 45));  // Slightly lighter for tree

	// Apply dark theme to tree control
	if (pWnd == &m_treeCtrl || pWnd->GetDlgCtrlID() == IDC_TREE_CONTROL)
	{
		pDC->SetBkColor(RGB(45, 45, 45));      // Dark background
		pDC->SetTextColor(RGB(255, 255, 255)); // White text
		return (HBRUSH)treeBrush.GetSafeHandle();
	}

	if (nCtlColor == CTLCOLOR_DLG)
	{
		pDC->SetBkColor(RGB(30, 30, 30));
		return (HBRUSH)darkBrush.GetSafeHandle();
	}

	return CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CProjectTreeDlg::SaveWindowState()
{
	// Get window rect
	RECT rect;
	GetWindowRect(&rect);

	TCHAR appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
	{
		CString configDir;
		configDir.Format(_T("%s\\ProjectTree"), appDataPath);
		CreateDirectory(configDir, NULL);

		CString configFile;
		configFile.Format(_T("%s\\WindowState.txt"), configDir);

		FILE * pFile = nullptr;
		if (_tfopen_s(&pFile, configFile, _T("w")) == 0 && pFile)
		{
			_ftprintf(pFile, _T("%d\n"), rect.left);
			_ftprintf(pFile, _T("%d\n"), rect.top);
			_ftprintf(pFile, _T("%d\n"), rect.right - rect.left);
			_ftprintf(pFile, _T("%d\n"), rect.bottom - rect.top);

			fclose(pFile);
		}
	}
}

void CProjectTreeDlg::RestoreWindowState()
{
	TCHAR appDataPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, appDataPath)))
	{
		CString configFile;
		configFile.Format(_T("%s\\ProjectTree\\WindowState.txt"), appDataPath);

		CStdioFile file;
		if (file.Open(configFile, CFile::modeRead | CFile::typeText))
		{
			CString line;
			int x = 0, y = 0, width = 0, height = 0;

			if (file.ReadString(line)) x = _ttoi(line);
			if (file.ReadString(line)) y = _ttoi(line);
			if (file.ReadString(line)) width = _ttoi(line);
			if (file.ReadString(line)) height = _ttoi(line);

			file.Close();

			// Validate and apply
			int screenWidth = GetSystemMetrics(SM_CXSCREEN);
			int screenHeight = GetSystemMetrics(SM_CYSCREEN);

			if (x + width > 0 && x < screenWidth &&
				y + height > 0 && y < screenHeight &&
				width > 100 && height > 100)
			{
				MoveWindow(x, y, width, height);
			}
		}
	}
}

afx_msg void CProjectTreeDlg::OnTreeRightClick(NMHDR * pNMHDR, LRESULT * pResult)
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	if (!hSelectedItem)
		return;

	// Create context menu
	CMenu menu;
	menu.CreatePopupMenu();

	CString * pPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
	bool isFolder = (pPath && !pPath->IsEmpty() && pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));

	if (isFolder)
	{
		menu.AppendMenu(MF_STRING, 1, _T("Open in Explorer"));
		menu.AppendMenu(MF_SEPARATOR);
	}

	menu.AppendMenu(MF_STRING, 2, _T("Open"));
	menu.AppendMenu(MF_STRING, 3, _T("Copy Path"));

	// Get cursor position
	POINT pt;
	GetCursorPos(&pt);

	// Show menu
	int result = menu.TrackPopupMenu(TPM_RETURNCMD, pt.x, pt.y, this);

	// Handle menu selection
	switch (result)
	{
	case 1:  // Open in Explorer
		if (pPath)
			ShellExecute(nullptr, _T("explore"), *pPath, nullptr, nullptr, SW_SHOW);
		break;
	case 2:  // Open
		if (pPath && !isFolder)
			ShellExecute(nullptr, _T("open"), *pPath, nullptr, nullptr, SW_SHOW);
		break;
	case 3:  // Copy Path
		if (pPath)
		{
			if (OpenClipboard())
			{
				EmptyClipboard();
				size_t len = pPath->GetLength();
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(TCHAR));
				if (hMem)
				{
					LPTSTR pMem = (LPTSTR)GlobalLock(hMem);
					_tcscpy_s(pMem, len + 1, *pPath);
					GlobalUnlock(hMem);
					SetClipboardData(CF_UNICODETEXT, hMem);
				}
				CloseClipboard();
			}
		}
		break;
	}

	*pResult = 0;
}

afx_msg void CProjectTreeDlg::OnTreeBeginLabelEdit(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	m_hRenameItem = pTVDispInfo->item.hItem;
	m_originalName = m_treeCtrl.GetItemText(m_hRenameItem);

	*pResult = 0;  // Allow editing
}

afx_msg void CProjectTreeDlg::OnTreeEndLabelEdit(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);

	*pResult = 0;  // Don't accept edit by default

	if (pTVDispInfo->item.pszText == nullptr)
		return;  // Edit was cancelled

	CString newName = pTVDispInfo->item.pszText;
	if (newName.IsEmpty())
		return;  // Empty name not allowed

	CString * pOldPath = (CString *)m_treeCtrl.GetItemData(m_hRenameItem);
	if (!pOldPath)
		return;

	// Build new path
	int lastBackslash = pOldPath->ReverseFind(_T('\\'));
	CString newPath = pOldPath->Left(lastBackslash + 1) + newName;

	// Handle folder vs file
	bool isFolder = (pOldPath->GetAt(pOldPath->GetLength() - 1) == _T('\\'));
	if (isFolder)
		newPath += _T("\\");

	// Rename the actual file/folder
	if (MoveFile(*pOldPath, newPath))
	{
		// Update tree item
		m_treeCtrl.SetItemText(m_hRenameItem, newName);
		m_treeCtrl.SetItemData(m_hRenameItem, (DWORD_PTR)new CString(newPath));

		*pResult = 1;  // Accept the edit
	}
	else
	{
		MessageBox(_T("Failed to rename file/folder"), _T("Error"));
	}
}

afx_msg void CProjectTreeDlg::OnTreeBeginDrag(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	CString * pPath = (CString *)m_treeCtrl.GetItemData(hItem);

	if (pPath && !pPath->IsEmpty())
	{
		m_bDragging = TRUE;
		m_hDragItem = hItem;

		m_pDragImageList = m_treeCtrl.CreateDragImage(hItem);
		if (m_pDragImageList)
		{
			m_pDragImageList->BeginDrag(0, CPoint(0, 0));

			POINT pt;
			GetCursorPos(&pt);
			m_pDragImageList->DragEnter(nullptr, pt);

			SetCapture();
		}
	}

	*pResult = 0;
}

afx_msg void CProjectTreeDlg::OnTreeEndDrag(NMHDR * pNMHDR, LRESULT * pResult)
{
	if (m_bDragging)
	{
		m_bDragging = FALSE;
		ReleaseCapture();

		if (m_pDragImageList)
		{
			m_pDragImageList->EndDrag();
			delete m_pDragImageList;
			m_pDragImageList = nullptr;
		}

		// Get the drop target item
		HTREEITEM hDropTarget = m_treeCtrl.GetSelectedItem();
		if (hDropTarget && hDropTarget != m_hDragItem)
		{
			CString * pSourcePath = (CString *)m_treeCtrl.GetItemData(m_hDragItem);
			CString * pTargetPath = (CString *)m_treeCtrl.GetItemData(hDropTarget);

			if (pSourcePath && pTargetPath)
			{
				// Target must be a folder
				bool isTargetFolder = (pTargetPath->GetAt(pTargetPath->GetLength() - 1) == _T('\\'));
				if (isTargetFolder)
				{
					// Extract filename from source
					int lastBackslash = pSourcePath->ReverseFind(_T('\\'));
					CString fileName = pSourcePath->Mid(lastBackslash + 1);

					CString newPath = *pTargetPath + fileName;

					// Check if it's a folder
					bool isSourceFolder = (pSourcePath->GetAt(pSourcePath->GetLength() - 1) == _T('\\'));
					if (isSourceFolder)
						newPath += _T("\\");

					// Move the file/folder
					if (MoveFile(*pSourcePath, newPath))
					{
						// Delete the source item
						m_treeCtrl.DeleteItem(m_hDragItem);

						// Reload the target folder's contents
						LoadDirectoryContents(hDropTarget, *pTargetPath);
						m_treeCtrl.Expand(hDropTarget, TVE_EXPAND);
					}
					else
					{
						MessageBox(_T("Failed to move file/folder"), _T("Error"));
					}
				}
			}
		}

		m_hDragItem = nullptr;
	}

	*pResult = 0;
}

afx_msg void CProjectTreeDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bDragging && m_pDragImageList)
	{
		POINT screenPoint = point;
		ClientToScreen(&screenPoint);
		m_pDragImageList->DragMove(screenPoint);
	}

	CDialogEx::OnMouseMove(nFlags, point);
}

afx_msg void CProjectTreeDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		m_bDragging = FALSE;
		ReleaseCapture();

		if (m_pDragImageList)
		{
			m_pDragImageList->EndDrag();
			delete m_pDragImageList;
			m_pDragImageList = nullptr;
		}

		m_hDragItem = nullptr;
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}


// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProjectTreeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProjectTreeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CProjectTreeDlg::OnTvnSelchangedTree2(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
