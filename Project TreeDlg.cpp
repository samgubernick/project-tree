
// Project TreeDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Project Tree.h"
#include "Project TreeDlg.h"
#include "afxdialogex.h"

#include <shellapi.h>

#include <functional>
#include <map>
#include <set>

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
	m_pDragImageList(nullptr),
	m_isCopiedItemFolder(false),
	m_isCutOperation(false),
	idePath{_T("C:/the-path-to-your-IDE/ide.exe")}
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
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN)
		{
			if (m_treeCtrl.GetEditControl() != nullptr)
			{
				return FALSE;
			}

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
						ShellExecute(nullptr, _T("open"), idePath, *pFilePath, nullptr, SW_SHOW);
					}
				}
			}
			return TRUE;  // Prevent default Enter behavior
		}

		if (pMsg->wParam == VK_ESCAPE)
		{
			// Cancel rename if in edit mode
			CEdit * pEdit = m_treeCtrl.GetEditControl();
			if (pEdit != nullptr)
			{
				// Discard the edit - return FALSE from label edit
				m_treeCtrl.EndEditLabelNow(TRUE);  // TRUE = discard changes
				return TRUE;
			}
			return FALSE;
		}

		// Check for Ctrl+C (copy)
		if (pMsg->wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
		{
			HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
			if (hSelectedItem)
			{
				// Don't allow copy from virtual view
				if (IsItemInVirtualView(hSelectedItem))
				{
					MessageBox(_T("Cannot copy from Combined View. Please copy from src or include folders."),
						_T("Invalid Operation"), MB_OK | MB_ICONWARNING);
					return TRUE;
				}

				CString * pPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
				if (pPath)
				{
					m_copiedItemPath = *pPath;
					m_isCopiedItemFolder = (pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));
				}
			}
			return TRUE;
		}

		// Check for Ctrl+V (paste)
		if (pMsg->wParam == 'V' && (GetKeyState(VK_CONTROL) & 0x8000))
		{
			if (m_copiedItemPath.IsEmpty())
			{
				MessageBox(_T("Nothing to paste. Use Ctrl+X or Ctrl+C to copy/cut a file or folder first."),
					_T("Nothing Copied"), MB_OK | MB_ICONINFORMATION);
				return TRUE;
			}

			HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
			if (hSelectedItem)
			{
				// Don't allow paste into virtual view
				if (IsItemInVirtualView(hSelectedItem))
				{
					MessageBox(_T("Cannot paste into Combined View. Please paste into src or include folders."),
						_T("Invalid Operation"), MB_OK | MB_ICONWARNING);
					return TRUE;
				}

				CString * pTargetPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
				if (pTargetPath)
				{
					// Target must be a folder
					bool isTargetFolder = (pTargetPath->GetAt(pTargetPath->GetLength() - 1) == _T('\\'));
					if (!isTargetFolder)
					{
						// Use parent folder
						hSelectedItem = m_treeCtrl.GetParentItem(hSelectedItem);
						if (hSelectedItem)
						{
							pTargetPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
							if (pTargetPath)
								isTargetFolder = true;
						}
					}

					if (isTargetFolder && pTargetPath)
					{
						// Extract filename from source
						int lastBackslash = m_copiedItemPath.ReverseFind(_T('\\'));
						CString fileName;
						if (lastBackslash >= 0)
						{
							fileName = m_copiedItemPath.Mid(lastBackslash + 1);
						}
						else
						{
							fileName = m_copiedItemPath;
						}

						// Build destination path
						CString destPath = *pTargetPath + fileName;

						// Handle source path
						CString sourcePath = m_copiedItemPath;
						if (m_isCopiedItemFolder)
							sourcePath.TrimRight(_T("\\"));

						CString destPathForMove = destPath;
						if (m_isCopiedItemFolder)
							destPathForMove.TrimRight(_T("\\"));

						bool success = false;

						if (m_isCutOperation)
						{
							// Move operation
							if (MoveFile(sourcePath, destPathForMove))
							{
								success = true;
							}
						}
						else
						{
							// Copy operation
							if (CopyFile(sourcePath, destPathForMove, TRUE))
							{
								success = true;
							}
						}

						if (success)
						{
							if (m_isCutOperation)
							{
								// For cut operation, reload all src and include folders
								HTREEITEM hRoot = m_treeCtrl.GetRootItem();
								while (hRoot)
								{
									CString text = m_treeCtrl.GetItemText(hRoot);
									if (text == _T("src") || text == _T("include"))
									{
										CString * pRootPath = (CString *)m_treeCtrl.GetItemData(hRoot);
										if (pRootPath)
										{
											// Clear all children
											HTREEITEM hChild = m_treeCtrl.GetChildItem(hRoot);
											while (hChild)
											{
												HTREEITEM hNext = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
												delete (CString *)m_treeCtrl.GetItemData(hChild);
												m_treeCtrl.DeleteItem(hChild);
												hChild = hNext;
											}
											// Reload from disk
											LoadDirectoryContents(hRoot, *pRootPath);
										}
									}
									hRoot = m_treeCtrl.GetNextItem(hRoot, TVGN_NEXT);
								}
							}
							else
							{
								// For copy, only reload target
								HTREEITEM hChild = m_treeCtrl.GetChildItem(hSelectedItem);
								while (hChild)
								{
									HTREEITEM hNext = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
									delete (CString *)m_treeCtrl.GetItemData(hChild);
									m_treeCtrl.DeleteItem(hChild);
									hChild = hNext;
								}
								LoadDirectoryContents(hSelectedItem, *pTargetPath);
							}

							// DELETE THIS ENTIRE BLOCK - IT'S REDUNDANT AND CAUSES NULL POINTER
							// ...

							// Refresh virtual view
							RefreshVirtualCombinedView();

							// Clear the clipboard
							m_copiedItemPath.Empty();
							m_isCutOperation = false;
						}
						else
						{
							DWORD error = GetLastError();
							CString msg;
							msg.Format(_T("Failed to %s file/folder. Error code: %d"),
								m_isCutOperation ? _T("move") : _T("copy"), error);
							MessageBox(msg, _T("Error"), MB_OK | MB_ICONERROR);
						}
					}
				}
			}
			return TRUE;
		}

		// Check for Ctrl+X (cut)
		if (pMsg->wParam == 'X' && (GetKeyState(VK_CONTROL) & 0x8000))
		{
			HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
			if (hSelectedItem)
			{
				// Don't allow cut from virtual view
				if (IsItemInVirtualView(hSelectedItem))
				{
					MessageBox(_T("Cannot cut from Combined View. Please cut from src or include folders."),
						_T("Invalid Operation"), MB_OK | MB_ICONWARNING);
					return TRUE;
				}

				CString * pPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
				if (pPath)
				{
					m_copiedItemPath = *pPath;
					m_isCopiedItemFolder = (pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));
					m_isCutOperation = true;
				}
			}
			return TRUE;
		}

		// Check for Ctrl+Shift+N
		if (pMsg->wParam == 'N' &&
			(GetKeyState(VK_CONTROL) & 0x8000) &&
			(GetKeyState(VK_SHIFT) & 0x8000))
		{
			CreateNewFolder();
			return TRUE;
		}

		// Check for Ctrl+N (new file)
		if (pMsg->wParam == 'N' &&
			(GetKeyState(VK_CONTROL) & 0x8000) &&
			!(GetKeyState(VK_SHIFT) & 0x8000))
		{
			CreateNewFile();
			return TRUE;
		}

		// Check for Delete key
		if (pMsg->wParam == VK_DELETE)
		{
			if (m_treeCtrl.GetEditControl() != nullptr)
			{
				return FALSE;
			}

			DeleteSelectedItem();
			return TRUE;
		}

		// Check for F2 (rename)
		if (pMsg->wParam == VK_F2)
		{
			HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
			if (hSelectedItem)
			{
				m_treeCtrl.EditLabel(hSelectedItem);
			}
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

HTREEITEM CProjectTreeDlg::FindItemParent(HTREEITEM hItem, const CString & pathToFind)
{
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
	while (hChild)
	{
		CString * pItemPath = (CString *)m_treeCtrl.GetItemData(hChild);
		if (pItemPath)
		{
			CString itemPathNorm = *pItemPath;
			CString findPathNorm = pathToFind;
			itemPathNorm.TrimRight(_T("\\"));
			findPathNorm.TrimRight(_T("\\"));

			if (itemPathNorm == findPathNorm)
			{
				// Found it, return current item as parent
				return hItem;
			}
		}

		// Recurse into child folders
		HTREEITEM hFound = FindItemParent(hChild, pathToFind);
		if (hFound)
			return hFound;

		hChild = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
	}
	return nullptr;
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
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTROL, &CProjectTreeDlg::OnTvnSelchangedTree2)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_CONTROL, OnTreeDblClick)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(NM_RCLICK, IDC_TREE_CONTROL, OnTreeRightClick)
	ON_NOTIFY(TVN_BEGINDRAG, IDC_TREE_CONTROL, OnTreeBeginDrag)
	ON_NOTIFY(TVN_BEGINLABELEDIT, IDC_TREE_CONTROL, OnTreeBeginLabelEdit)
	ON_NOTIFY(TVN_ENDLABELEDIT, IDC_TREE_CONTROL, OnTreeEndLabelEdit)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE_CONTROL, OnTreeItemExpanding)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_ACTIVATE()
	ON_MESSAGE(WM_RESTORE_EXPANDED_STATE, OnRestoreExpandedState)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTROL, &CProjectTreeDlg::OnTvnSelchangedTreeControl)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_TREE_CONTROL, OnCustomDraw)
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

	m_rootPath = _T("D:\\test-folder\\");
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

	// Add combined virtual view
	HTREEITEM hCombined = m_treeCtrl.InsertItem(_T("Combined View"), folderIcon, folderIcon);
	m_treeCtrl.SetItemData(hCombined, (DWORD_PTR)new CString(_T("VIRTUAL:COMBINED")));
	m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hCombined);
}

void CProjectTreeDlg::LoadDirectoryContents(HTREEITEM hParent, const CString & folderPath)
{
	// Remove dummy item
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hParent);
	if (hChild && m_treeCtrl.GetItemText(hChild).IsEmpty())
	{
		m_treeCtrl.DeleteItem(hChild);
	}

	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(folderPath + _T("*"), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
		return;

	do
	{
		if (_tcscmp(findData.cFileName, _T(".")) != 0 &&
			_tcscmp(findData.cFileName, _T("..")) != 0)
		{
			CString itemPath = folderPath + findData.cFileName;
			bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			SHFILEINFO sfi = {0};
			SHGetFileInfo(itemPath, findData.dwFileAttributes, &sfi, sizeof(SHFILEINFO),
				SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
			int icon = m_imageList.Add(sfi.hIcon);
			DestroyIcon(sfi.hIcon);

			if (isDir)
			{
				itemPath += _T("\\");
				HTREEITEM hDir = m_treeCtrl.InsertItem(findData.cFileName, icon, icon, hParent);
				m_treeCtrl.SetItemData(hDir, (DWORD_PTR)new CString(itemPath));

				// Check if subdirectory is empty
				WIN32_FIND_DATA subFindData;
				HANDLE hSubFind = FindFirstFile(itemPath + _T("*"), &subFindData);
				bool hasItems = false;

				if (hSubFind != INVALID_HANDLE_VALUE)
				{
					do
					{
						if (_tcscmp(subFindData.cFileName, _T(".")) != 0 &&
							_tcscmp(subFindData.cFileName, _T("..")) != 0)
						{
							hasItems = true;
							break;
						}
					} while (FindNextFile(hSubFind, &subFindData));
					FindClose(hSubFind);
				}

				// Only add dummy if folder has contents
				if (hasItems)
				{
					m_treeCtrl.InsertItem(_T(""), icon, icon, hDir);
				}
			}
			else
			{
				HTREEITEM hFile = m_treeCtrl.InsertItem(findData.cFileName, icon, icon, hParent);
				m_treeCtrl.SetItemData(hFile, (DWORD_PTR)new CString(itemPath));
			}
		}
	} while (FindNextFile(hFind, &findData));

	FindClose(hFind);
}

afx_msg void CProjectTreeDlg::OnTreeItemExpanding(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;

	CString * pPath = (CString *)m_treeCtrl.GetItemData(hItem);
	if (pPath)
	{
		// Check if already loaded (no dummy child or has actual content)
		HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
		if (!hChild)
		{
			*pResult = 0;
			return;  // No children
		}

		CString childText = m_treeCtrl.GetItemText(hChild);
		if (!childText.IsEmpty())
		{
			*pResult = 0;
			return;  // Already loaded - don't reload
		}

		// Has dummy child, so load content
		if (*pPath == _T("VIRTUAL:COMBINED"))
		{
			LoadCombinedView(hItem);
		}
		else if (pPath->Find(_T("VIRTUAL:COMBINED:")) == 0)
		{
			CString subPath = pPath->Mid(17);
			CString srcDir = m_rootPath + _T("src\\");
			CString includeDir = m_rootPath + _T("include\\");
			LoadCombinedDirectory(hItem, srcDir, includeDir, subPath + _T("\\"));
		}
		else if (pPath->GetAt(pPath->GetLength() - 1) == _T('\\'))
		{
			LoadDirectoryContents(hItem, *pPath);
		}
	}

	*pResult = 0;
}


void CProjectTreeDlg::LoadCombinedView(HTREEITEM hParent)
{
	// Remove dummy item
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hParent);
	if (hChild && m_treeCtrl.GetItemText(hChild).IsEmpty())
	{
		m_treeCtrl.DeleteItem(hChild);
	}

	CString srcPath = m_rootPath + _T("src\\");
	CString includePath = m_rootPath + _T("include\\");

	// Load the combined directory recursively
	LoadCombinedDirectory(hParent, srcPath, includePath, _T(""));
}

void CProjectTreeDlg::LoadCombinedDirectory(HTREEITEM hParent, const CString & srcDir,
	const CString & includeDir, const CString & relativePath)
{
	// Remove dummy item FIRST
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hParent);
	if (hChild && m_treeCtrl.GetItemText(hChild).IsEmpty())
	{
		m_treeCtrl.DeleteItem(hChild);
	}

	std::set<CString> allFiles;
	std::set<CString> folders;
	WIN32_FIND_DATA findData;

	// Scan src subdirectory
	HANDLE hFind = FindFirstFile(srcDir + relativePath + _T("*"), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (_tcscmp(findData.cFileName, _T(".")) != 0 &&
				_tcscmp(findData.cFileName, _T("..")) != 0)
			{
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					folders.insert(findData.cFileName);
				}
				else
				{
					allFiles.insert(findData.cFileName);
				}
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	// Scan include subdirectory
	hFind = FindFirstFile(includeDir + relativePath + _T("*"), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (_tcscmp(findData.cFileName, _T(".")) != 0 &&
				_tcscmp(findData.cFileName, _T("..")) != 0)
			{
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					folders.insert(findData.cFileName);
				}
				else
				{
					allFiles.insert(findData.cFileName);
				}
			}
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	// Process directories first
	for (const auto & folderName : folders)
	{
		CString srcFolderPath = srcDir + relativePath + folderName;

		SHFILEINFO sfi = {0};
		SHGetFileInfo(srcFolderPath, FILE_ATTRIBUTE_DIRECTORY, &sfi,
			sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		int folderIcon = m_imageList.Add(sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		HTREEITEM hDir = m_treeCtrl.InsertItem(folderName, folderIcon, folderIcon, hParent);
		m_treeCtrl.SetItemData(hDir, (DWORD_PTR)new CString(_T("VIRTUAL:COMBINED:") + relativePath + folderName));

		// Check if folder is empty before adding dummy
		CString srcSubPath = srcFolderPath + _T("\\");
		CString includeSubPath = includeDir + relativePath + folderName + _T("\\");

		WIN32_FIND_DATA subFindData;
		bool hasItems = false;

		HANDLE hSubFind = FindFirstFile(srcSubPath + _T("*"), &subFindData);
		if (hSubFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (_tcscmp(subFindData.cFileName, _T(".")) != 0 &&
					_tcscmp(subFindData.cFileName, _T("..")) != 0)
				{
					hasItems = true;
					break;
				}
			} while (FindNextFile(hSubFind, &subFindData));
			FindClose(hSubFind);
		}

		if (!hasItems)
		{
			hSubFind = FindFirstFile(includeSubPath + _T("*"), &subFindData);
			if (hSubFind != INVALID_HANDLE_VALUE)
			{
				do
				{
					if (_tcscmp(subFindData.cFileName, _T(".")) != 0 &&
						_tcscmp(subFindData.cFileName, _T("..")) != 0)
					{
						hasItems = true;
						break;
					}
				} while (FindNextFile(hSubFind, &subFindData));
				FindClose(hSubFind);
			}
		}

		// Only add dummy if folder has contents
		if (hasItems)
		{
			m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hDir);
		}
	}

	// Process files
	for (const auto & fileName : allFiles)
	{
		CString srcFilePath = srcDir + relativePath + fileName;
		CString includeFilePath = includeDir + relativePath + fileName;

		CString filePath = srcFilePath;
		if (!PathFileExists(srcFilePath) && PathFileExists(includeFilePath))
		{
			filePath = includeFilePath;
		}

		SHFILEINFO sfi = {0};
		SHGetFileInfo(filePath, 0, &sfi, sizeof(SHFILEINFO),
			SHGFI_ICON | SHGFI_SMALLICON);
		int fileIcon = m_imageList.Add(sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		HTREEITEM hFile = m_treeCtrl.InsertItem(fileName, fileIcon, fileIcon, hParent);
		m_treeCtrl.SetItemData(hFile, (DWORD_PTR)new CString(filePath));
	}
}

void CProjectTreeDlg::LoadCombinedSubfolder(HTREEITEM hParent, const CString & folderName)
{
	// Remove dummy
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hParent);
	if (hChild && m_treeCtrl.GetItemText(hChild).IsEmpty())
	{
		m_treeCtrl.DeleteItem(hChild);
	}

	CString srcPath = m_rootPath + _T("src\\") + folderName + _T("\\");
	CString includePath = m_rootPath + _T("include\\") + folderName + _T("\\");

	std::map<CString, std::pair<CString, CString>> fileMap;

	// Scan src for .cpp files
	WIN32_FIND_DATA findData;
	HANDLE hFind = FindFirstFile(srcPath + _T("*.cpp"), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			CString fileName = findData.cFileName;
			CString baseName = fileName.Left(fileName.ReverseFind(_T('.')));
			fileMap[baseName].first = srcPath + fileName;
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	// Scan include for .hpp files
	hFind = FindFirstFile(includePath + _T("*.hpp"), &findData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			CString fileName = findData.cFileName;
			CString baseName = fileName.Left(fileName.ReverseFind(_T('.')));
			fileMap[baseName].second = includePath + fileName;
		} while (FindNextFile(hFind, &findData));
		FindClose(hFind);
	}

	// Create grouped tree items
	for (const auto & pair : fileMap)
	{
		CString groupName = pair.first;
		HTREEITEM hGroup = m_treeCtrl.InsertItem(groupName, 0, 0, hParent);

		if (!pair.second.first.IsEmpty())
		{
			SHFILEINFO sfi = {0};
			SHGetFileInfo(pair.second.first, 0, &sfi, sizeof(SHFILEINFO),
						 SHGFI_ICON | SHGFI_SMALLICON);
			int cppIcon = m_imageList.Add(sfi.hIcon);
			DestroyIcon(sfi.hIcon);

			HTREEITEM hCpp = m_treeCtrl.InsertItem(groupName + _T(".cpp"), cppIcon, cppIcon, hGroup);
			m_treeCtrl.SetItemData(hCpp, (DWORD_PTR)new CString(pair.second.first));
		}

		if (!pair.second.second.IsEmpty())
		{
			SHFILEINFO sfi = {0};
			SHGetFileInfo(pair.second.second, 0, &sfi, sizeof(SHFILEINFO),
						 SHGFI_ICON | SHGFI_SMALLICON);
			int hppIcon = m_imageList.Add(sfi.hIcon);
			DestroyIcon(sfi.hIcon);

			HTREEITEM hHpp = m_treeCtrl.InsertItem(groupName + _T(".hpp"), hppIcon, hppIcon, hGroup);
			m_treeCtrl.SetItemData(hHpp, (DWORD_PTR)new CString(pair.second.second));
		}

		m_treeCtrl.Expand(hGroup, TVE_EXPAND);
	}
}

void CProjectTreeDlg::RefreshVirtualCombinedView()
{
	// Find the "Combined View" root item
	HTREEITEM hCombined = m_treeCtrl.GetRootItem();
	while (hCombined)
	{
		CString itemText = m_treeCtrl.GetItemText(hCombined);
		if (itemText == _T("Combined View"))
		{
			// Collapse it first
			m_treeCtrl.Expand(hCombined, TVE_COLLAPSE);

			// Delete all children
			HTREEITEM hChild = m_treeCtrl.GetChildItem(hCombined);
			while (hChild)
			{
				HTREEITEM hNext = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
				m_treeCtrl.DeleteItem(hChild);
				hChild = hNext;
			}

			// Re-add dummy with proper icon
			SHFILEINFO sfi = {0};
			SHGetFileInfo(_T("C:\\"), FILE_ATTRIBUTE_DIRECTORY, &sfi,
				sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
			int folderIcon = m_imageList.Add(sfi.hIcon);
			DestroyIcon(sfi.hIcon);

			m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hCombined);
			break;
		}
		hCombined = m_treeCtrl.GetNextItem(hCombined, TVGN_NEXT);
	}
}

void CProjectTreeDlg::SaveExpandedStateRecursive(HTREEITEM hItem, std::set<CString> & expandedPaths)
{
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
	while (hChild)
	{
		UINT state = m_treeCtrl.GetItemState(hChild, TVIS_EXPANDED);
		if (state & TVIS_EXPANDED)
		{
			CString * pPath = (CString *)m_treeCtrl.GetItemData(hChild);
			if (pPath)
			{
				expandedPaths.insert(*pPath);
			}
			SaveExpandedStateRecursive(hChild, expandedPaths);
		}
		hChild = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
	}
}

void CProjectTreeDlg::RestoreExpandedStateRecursive(HTREEITEM hItem, const std::set<CString> & expandedPaths)
{
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
	while (hChild)
	{
		CString * pPath = (CString *)m_treeCtrl.GetItemData(hChild);
		if (pPath && expandedPaths.find(*pPath) != expandedPaths.end())
		{
			m_treeCtrl.Expand(hChild, TVE_EXPAND);
			RestoreExpandedStateRecursive(hChild, expandedPaths);
		}
		hChild = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
	}
}

LRESULT CProjectTreeDlg::OnRestoreExpandedState(WPARAM wParam, LPARAM lParam)
{
	HTREEITEM hCombined = (HTREEITEM)wParam;
	std::set<CString> * pExpandedPaths = (std::set<CString>*)lParam;

	if (pExpandedPaths)
	{
		RestoreExpandedStateRecursive(hCombined, *pExpandedPaths);
		delete pExpandedPaths;
	}

	return 0;
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
				ShellExecute(nullptr, _T("open"), idePath, *pFilePath, nullptr, SW_SHOW);
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
			ShellExecute(nullptr, _T("open"), idePath, *pPath, nullptr, SW_SHOW);
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

	// Get the edit control
	CEdit * pEdit = m_treeCtrl.GetEditControl();
	if (pEdit)
	{
		// Check if it's a file (not a folder)
		CString * pPath = (CString *)m_treeCtrl.GetItemData(m_hRenameItem);
		bool isFolder = (pPath && !pPath->IsEmpty() &&
						pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));

		if (!isFolder)
		{
			// Find the last dot for the extension
			CString itemText = m_originalName;
			int dotPos = itemText.ReverseFind(_T('.'));

			if (dotPos > 0)  // Found extension and it's not at the start
			{
				// Select only the filename (before the extension)
				pEdit->SetSel(0, dotPos);
			}
			else
			{
				// No extension, select all
				pEdit->SetSel(0, -1);
			}
		}
		else
		{
			// Folder, select all
			pEdit->SetSel(0, -1);
		}
	}

	*pResult = 0;  // Allow editing
}

afx_msg void CProjectTreeDlg::OnTreeEndLabelEdit(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	*pResult = 0;

	if (!pTVDispInfo->item.pszText)
		return;

	CString newName = pTVDispInfo->item.pszText;
	if (newName.IsEmpty())
		return;

	CString * pOldPath = (CString *)m_treeCtrl.GetItemData(m_hRenameItem);
	if (!pOldPath)
		return;

	// Check if this item is in the virtual view by checking parent hierarchy
	bool isVirtualView = false;
	HTREEITEM hParent = m_treeCtrl.GetParentItem(m_hRenameItem);
	while (hParent)
	{
		CString parentText = m_treeCtrl.GetItemText(hParent);
		if (parentText == _T("Combined View"))
		{
			isVirtualView = true;
			break;
		}

		// Also check if parent has VIRTUAL marker
		CString * pParentPath = (CString *)m_treeCtrl.GetItemData(hParent);
		if (pParentPath && pParentPath->Find(_T("VIRTUAL:COMBINED")) == 0)
		{
			isVirtualView = true;
			break;
		}

		hParent = m_treeCtrl.GetParentItem(hParent);
	}

	// Determine if it's a folder
	bool isFolder = (pOldPath->GetAt(pOldPath->GetLength() - 1) == _T('\\'));

	// Build the path to rename FROM
	CString oldPathForMove = *pOldPath;

	// For virtual view, path is already the real path for files
	// We don't need to convert it

	if (isFolder)
		oldPathForMove.TrimRight(_T("\\"));

	// Build new path
	int lastBackslash = oldPathForMove.ReverseFind(_T('\\'));
	CString newPath = oldPathForMove.Left(lastBackslash + 1) + newName;
	if (isFolder)
		newPath += _T("\\");

	// Rename the actual file/folder
	CString newPathForMove = newPath;
	if (isFolder)
		newPathForMove.TrimRight(_T("\\"));

	if (MoveFile(oldPathForMove, newPathForMove))
	{
		m_treeCtrl.SetItemText(m_hRenameItem, newName);

		// Free old memory and store the new path
		delete pOldPath;
		m_treeCtrl.SetItemData(m_hRenameItem, (DWORD_PTR)new CString(newPath));

		*pResult = 1;

		// Update the corresponding view
		if (isVirtualView)
		{
			UpdateFileInRealView(oldPathForMove, newPathForMove, newName);
		}
		else
		{
			UpdateFileInVirtualView(oldPathForMove, newPathForMove, newName);
		}
	}
	else
	{
		DWORD error = GetLastError();
		CString msg;
		msg.Format(_T("Failed to rename file/folder. Error: %d"), error);
		MessageBox(msg, _T("Error"), MB_OK | MB_ICONERROR);
	}
}

void CProjectTreeDlg::UpdateFileInVirtualView(const CString & oldPath, const CString & newPath, const CString & newName)
{
	// Find Combined View root
	HTREEITEM hRoot = m_treeCtrl.GetRootItem();
	while (hRoot)
	{
		if (m_treeCtrl.GetItemText(hRoot) == _T("Combined View"))
		{
			SearchAndUpdateFile(hRoot, oldPath, newPath, newName);
			break;
		}
		hRoot = m_treeCtrl.GetNextItem(hRoot, TVGN_NEXT);
	}
}

void CProjectTreeDlg::UpdateFileInRealView(const CString & oldPath, const CString & newPath, const CString & newName)
{
	// Find root items (include, src, shader_angle, etc.)
	HTREEITEM hRoot = m_treeCtrl.GetRootItem();
	while (hRoot)
	{
		CString itemText = m_treeCtrl.GetItemText(hRoot);
		if (itemText != _T("Combined View"))
		{
			SearchAndUpdateFile(hRoot, oldPath, newPath, newName);
		}
		hRoot = m_treeCtrl.GetNextItem(hRoot, TVGN_NEXT);
	}
}

void CProjectTreeDlg::SearchAndUpdateFile(HTREEITEM hItem, const CString & oldPath, const CString & newPath, const CString & newName)
{
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
	while (hChild)
	{
		CString * pItemPath = (CString *)m_treeCtrl.GetItemData(hChild);
		if (pItemPath)
		{
			bool isMatch = false;

			// Extract the relative path from virtual marker
			CString virtualRelativePath;

			// Direct path match
			if (*pItemPath == oldPath)
			{
				isMatch = true;
			}
			// Virtual folder match - check if the virtual path corresponds to the real path
			else if (pItemPath->Find(_T("VIRTUAL:COMBINED:")) == 0)
			{
				// Extract the relative path from virtual marker
				virtualRelativePath = pItemPath->Mid(17);  // Skip "VIRTUAL:COMBINED:"

				// Check if oldPath ends with this relative path
				if (oldPath.Find(virtualRelativePath) != -1)
				{
					isMatch = true;
				}
			}

			if (isMatch)
			{
				// Found it! Update the name
				m_treeCtrl.SetItemText(hChild, newName);

				// Update the path appropriately
				if (pItemPath->Find(_T("VIRTUAL:COMBINED:")) == 0)
				{
					// Virtual folder - reconstruct virtual path with new name
					CString newVirtualPath = _T("VIRTUAL:COMBINED:");

					// Extract everything except the last component (old folder name)
					int lastBackslash = virtualRelativePath.ReverseFind(_T('\\'));
					if (lastBackslash != -1)
					{
						newVirtualPath += virtualRelativePath.Left(lastBackslash + 1) + newName;
					}
					else
					{
						newVirtualPath += newName;
					}

					delete pItemPath;
					m_treeCtrl.SetItemData(hChild, (DWORD_PTR)new CString(newVirtualPath));
				}
				else
				{
					// Real path - just update normally
					delete pItemPath;
					m_treeCtrl.SetItemData(hChild, (DWORD_PTR)new CString(newPath));
				}
				return;
			}
		}

		// Recurse into child folders
		SearchAndUpdateFile(hChild, oldPath, newPath, newName);
		hChild = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
	}
}

void CProjectTreeDlg::SearchAndDeleteItem(HTREEITEM hItem, const CString & pathToDelete)
{
	HTREEITEM hChild = m_treeCtrl.GetChildItem(hItem);
	while (hChild)
	{
		CString * pItemPath = (CString *)m_treeCtrl.GetItemData(hChild);
		if (pItemPath)
		{
			// Normalize paths for comparison
			CString itemPathNorm = *pItemPath;
			CString deletePathNorm = pathToDelete;
			itemPathNorm.TrimRight(_T("\\"));
			deletePathNorm.TrimRight(_T("\\"));

			if (itemPathNorm == deletePathNorm)
			{
				// Found it! Delete the item
				delete pItemPath;
				m_treeCtrl.DeleteItem(hChild);
				return;
			}
		}

		// Recurse into child folders
		SearchAndDeleteItem(hChild, pathToDelete);
		hChild = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
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

		// Convert point to tree control coordinates
		CPoint treePoint = point;
		ClientToScreen(&treePoint);
		m_treeCtrl.ScreenToClient(&treePoint);

		// Get the drop target item
		UINT flags;
		HTREEITEM hDropTarget = m_treeCtrl.HitTest(treePoint, &flags);

		// Check if we have a valid drop target and it's on an item
		if (hDropTarget && (flags & TVHT_ONITEM) && hDropTarget != m_hDragItem)
		{
			// Check if drag source and drop target are in different view types
			bool sourceIsVirtual = IsItemInVirtualView(m_hDragItem);
			bool targetIsVirtual = IsItemInVirtualView(hDropTarget);

			if (sourceIsVirtual != targetIsVirtual)
			{
				MessageBox(_T("Cannot drag items between real folders and the Combined View."),
					_T("Invalid Operation"), MB_OK | MB_ICONWARNING);
				m_hDragItem = nullptr;
				CDialogEx::OnLButtonUp(nFlags, point);
				return;
			}

			CString * pSourcePath = (CString *)m_treeCtrl.GetItemData(m_hDragItem);
			CString * pTargetPath = (CString *)m_treeCtrl.GetItemData(hDropTarget);

			if (pSourcePath && pTargetPath)
			{
				// Target must be a folder
				bool isTargetFolder = (pTargetPath->GetAt(pTargetPath->GetLength() - 1) == _T('\\'));
				if (!isTargetFolder)
				{
					// If target is a file, use its parent folder
					hDropTarget = m_treeCtrl.GetParentItem(hDropTarget);
					if (hDropTarget)
					{
						pTargetPath = (CString *)m_treeCtrl.GetItemData(hDropTarget);
						if (pTargetPath)
							isTargetFolder = true;
					}
				}

				if (isTargetFolder && pTargetPath)
				{
					// Check that we're not trying to move into a descendant
					HTREEITEM hParent = hDropTarget;
					bool isDescendant = false;
					while (hParent)
					{
						if (hParent == m_hDragItem)
						{
							isDescendant = true;
							break;
						}
						hParent = m_treeCtrl.GetParentItem(hParent);
					}

					if (!isDescendant)
					{
						// Extract filename from source
						int lastBackslash = pSourcePath->ReverseFind(_T('\\'));
						CString fileName;
						if (lastBackslash >= 0)
						{
							fileName = pSourcePath->Mid(lastBackslash + 1);
						}
						else
						{
							fileName = *pSourcePath;
						}

						// Build new path
						CString newPath = *pTargetPath + fileName;

						// Check if source is a folder
						bool isSourceFolder = (pSourcePath->GetAt(pSourcePath->GetLength() - 1) == _T('\\'));

						// Move the file/folder
						if (MoveFile(*pSourcePath, newPath))
						{
							// Remove source item from tree
							m_treeCtrl.DeleteItem(m_hDragItem);

							// Expand target and reload its contents
							m_treeCtrl.Expand(hDropTarget, TVE_EXPAND);

							// Delete children and reload
							HTREEITEM hChild = m_treeCtrl.GetChildItem(hDropTarget);
							while (hChild)
							{
								HTREEITEM hNext = m_treeCtrl.GetNextItem(hChild, TVGN_NEXT);
								m_treeCtrl.DeleteItem(hChild);
								hChild = hNext;
							}

							LoadDirectoryContents(hDropTarget, *pTargetPath);
						}
						else
						{
							DWORD error = GetLastError();
							CString msg;
							msg.Format(_T("Failed to move file/folder. Error code: %d"), error);
							MessageBox(msg, _T("Error"), MB_OK | MB_ICONERROR);
						}
					}
					else
					{
						MessageBox(_T("Cannot move a folder into itself or its subfolder"),
								  _T("Invalid Operation"), MB_OK | MB_ICONWARNING);
					}
				}
			}
		}

		m_hDragItem = nullptr;
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}


bool CProjectTreeDlg::IsItemInVirtualView(HTREEITEM hItem)
{
	HTREEITEM hParent = hItem;
	while (hParent)
	{
		CString text = m_treeCtrl.GetItemText(hParent);
		if (text == _T("Combined View"))
		{
			return true;
		}

		CString * pPath = (CString *)m_treeCtrl.GetItemData(hParent);
		if (pPath && pPath->Find(_T("VIRTUAL:COMBINED")) == 0)
		{
			return true;
		}

		hParent = m_treeCtrl.GetParentItem(hParent);
	}
	return false;
}


void CProjectTreeDlg::CreateNewFolder()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	if (!hSelectedItem)
		return;

	CString * pPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
	if (!pPath)
		return;

	if (pPath->Find(_T("VIRTUAL:COMBINED")) == 0)
	{
		MessageBox(_T("Cannot create folders in the Combined View. Use src or include folders directly."),
			_T("Invalid Operation"), MB_OK | MB_ICONWARNING);
		return;
	}

	// Determine the parent folder
	CString parentFolder;
	bool isFolder = (pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));

	if (isFolder)
	{
		parentFolder = *pPath;
	}
	else
	{
		int lastBackslash = pPath->ReverseFind(_T('\\'));
		if (lastBackslash != -1)
		{
			parentFolder = pPath->Left(lastBackslash + 1);
		}
		else
		{
			return;
		}
	}

	// Find a unique folder name
	CString newFolderName = _T("New Folder");
	CString newFolderPath = parentFolder + newFolderName + _T("\\");
	int counter = 1;

	while (PathFileExists(newFolderPath))
	{
		newFolderName.Format(_T("New Folder (%d)"), counter++);
		newFolderPath = parentFolder + newFolderName + _T("\\");
	}

	// Create the directory (remove trailing backslash for CreateDirectory)
	CString folderToCreate = newFolderPath;
	folderToCreate.TrimRight(_T("\\"));

	if (CreateDirectory(folderToCreate, NULL))
	{
		HTREEITEM hParentItem = isFolder ? hSelectedItem : m_treeCtrl.GetParentItem(hSelectedItem);

		// Get folder icon
		SHFILEINFO sfi = {0};
		SHGetFileInfo(folderToCreate, FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(SHFILEINFO),
			SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		int folderIcon = m_imageList.Add(sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		// Insert new folder item with path INCLUDING trailing backslash
		HTREEITEM hNewFolder = m_treeCtrl.InsertItem(newFolderName, folderIcon, folderIcon, hParentItem);
		m_treeCtrl.SetItemData(hNewFolder, (DWORD_PTR)new CString(newFolderPath));
		m_treeCtrl.InsertItem(_T(""), folderIcon, folderIcon, hNewFolder);

		// Expand parent and select new folder
		m_treeCtrl.Expand(hParentItem, TVE_EXPAND);
		m_treeCtrl.SelectItem(hNewFolder);

		// Start editing the name
		m_treeCtrl.EditLabel(hNewFolder);

		RefreshVirtualCombinedView();
	}
	else
	{
		MessageBox(_T("Failed to create folder"), _T("Error"), MB_OK | MB_ICONERROR);
	}
}


void CProjectTreeDlg::CreateNewFile()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	if (!hSelectedItem)
		return;

	CString * pPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
	if (!pPath)
		return;

	if (pPath->Find(_T("VIRTUAL:COMBINED")) == 0)
	{
		MessageBox(_T("Cannot create files in the Combined View. Use src or include folders directly."),
			_T("Invalid Operation"), MB_OK | MB_ICONWARNING);
		return;
	}

	// Determine the parent folder
	CString parentFolder;
	bool isFolder = (pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));

	if (isFolder)
	{
		// Selected item is a folder, create file inside it
		parentFolder = *pPath;
	}
	else
	{
		// Selected item is a file, create new file in its parent directory
		int lastBackslash = pPath->ReverseFind(_T('\\'));
		if (lastBackslash != -1)
		{
			parentFolder = pPath->Left(lastBackslash + 1);
		}
		else
		{
			return;  // Can't determine parent
		}
	}

	// Find a unique file name
	CString newFileName = _T("New File.txt");
	CString newFilePath = parentFolder + newFileName;
	int counter = 1;

	while (PathFileExists(newFilePath))
	{
		newFileName.Format(_T("New File (%d).txt"), counter++);
		newFilePath = parentFolder + newFileName;
	}

	// Create the empty file
	HANDLE hFile = CreateFile(newFilePath, GENERIC_WRITE, 0, NULL,
							 CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);

		// Get file icon
		SHFILEINFO sfi = {0};
		SHGetFileInfo(newFilePath, 0, &sfi, sizeof(SHFILEINFO),
					 SHGFI_ICON | SHGFI_SMALLICON);
		int fileIcon = m_imageList.Add(sfi.hIcon);
		DestroyIcon(sfi.hIcon);

		// Add to tree
		HTREEITEM hParentItem = isFolder ? hSelectedItem : m_treeCtrl.GetParentItem(hSelectedItem);

		// Insert new file item
		HTREEITEM hNewFile = m_treeCtrl.InsertItem(newFileName, fileIcon, fileIcon, hParentItem);
		m_treeCtrl.SetItemData(hNewFile, (DWORD_PTR)new CString(newFilePath));

		// Expand parent and select new file
		m_treeCtrl.Expand(hParentItem, TVE_EXPAND);
		m_treeCtrl.SelectItem(hNewFile);

		// Start editing the name
		m_treeCtrl.EditLabel(hNewFile);

		RefreshVirtualCombinedView();
	}
	else
	{
		MessageBox(_T("Failed to create file"), _T("Error"), MB_OK | MB_ICONERROR);
	}
}

void CProjectTreeDlg::DeleteSelectedItem()
{
	HTREEITEM hSelectedItem = m_treeCtrl.GetSelectedItem();
	if (!hSelectedItem)
		return;

	CString * pPath = (CString *)m_treeCtrl.GetItemData(hSelectedItem);
	if (!pPath || pPath->IsEmpty())
		return;

	// Get item name for confirmation dialog
	CString itemName = m_treeCtrl.GetItemText(hSelectedItem);
	bool isFolder = (pPath->GetAt(pPath->GetLength() - 1) == _T('\\'));

	// Show confirmation dialog
	CString message;
	if (isFolder)
	{
		message.Format(_T("Are you sure you want to delete the folder '%s' and all its contents?"), itemName);
	}
	else
	{
		message.Format(_T("Are you sure you want to delete '%s'?"), itemName);
	}

	int result = MessageBox(message, _T("Confirm Delete"), MB_YESNO | MB_ICONQUESTION);

	if (result == IDYES)
	{
		BOOL success = FALSE;

		if (isFolder)
		{
			// Delete folder recursively using SHFileOperation
			SHFILEOPSTRUCT fileOp = {0};
			fileOp.hwnd = GetSafeHwnd();
			fileOp.wFunc = FO_DELETE;
			fileOp.fFlags = FOF_NOCONFIRMATION | FOF_SILENT;

			// Must be double null-terminated
			TCHAR szPath[MAX_PATH + 1];
			_tcscpy_s(szPath, MAX_PATH, *pPath);
			szPath[pPath->GetLength()] = 0;
			szPath[pPath->GetLength() + 1] = 0;

			fileOp.pFrom = szPath;

			success = (SHFileOperation(&fileOp) == 0);
		}
		else
		{
			// Delete file
			success = DeleteFile(*pPath);
		}

		if (success)
		{
			// Remove from tree
			m_treeCtrl.DeleteItem(hSelectedItem);
		}
		else
		{
			MessageBox(_T("Failed to delete item"), _T("Error"), MB_OK | MB_ICONERROR);
		}
	}
}

void CProjectTreeDlg::OnActivate(UINT nState, CWnd * pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	if (nState == WA_INACTIVE)
	{
		// Window is losing focus - darken the tree control
		m_treeCtrl.SetBkColor(RGB(5, 5, 5));  // Light gray background
		m_treeCtrl.SetTextColor(RGB(80, 80, 80)); // Darker gray text
	}
	else  // WA_ACTIVE or WA_CLICKACTIVE
	{
		// Window is gaining focus - restore normal colors
		m_treeCtrl.SetBkColor(RGB(11, 11, 11));  // White background
		m_treeCtrl.SetTextColor(RGB(160, 160, 160));      // Black text
	}

	m_treeCtrl.Invalidate();
}

void CProjectTreeDlg::OnCustomDraw(NMHDR * pNMHDR, LRESULT * pResult)
{
	NMTVCUSTOMDRAW * pCustomDraw = (NMTVCUSTOMDRAW *)pNMHDR;

	switch (pCustomDraw->nmcd.dwDrawStage)
	{
		case CDDS_PREPAINT:
			*pResult = CDRF_NOTIFYITEMDRAW;
			break;

		case CDDS_ITEMPREPAINT:
		{
			HTREEITEM hItem = (HTREEITEM)pCustomDraw->nmcd.dwItemSpec;

			// Check if this item is selected
			if (pCustomDraw->nmcd.uItemState & CDIS_SELECTED)
			{
				pCustomDraw->nmcd.uItemState &= ~CDIS_SELECTED;

				// Get item rectangle
				CRect rect;
				m_treeCtrl.GetItemRect(hItem, &rect, FALSE);

				// Draw custom background
				CDC * pDC = CDC::FromHandle(pCustomDraw->nmcd.hdc);
				pDC->FillSolidRect(&rect, RGB(30, 30, 60));  // Black background


				// Custom selection colors
				pCustomDraw->clrText = RGB(205, 205, 205);  // White text
				pCustomDraw->clrTextBk = RGB(0, 50, 0);  // Blue background (Windows 10 style)

				*pResult = CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;  // Tell it we changed colors
			}
			else if (pCustomDraw->nmcd.uItemState & CDIS_HOT)
			{
				// Get item rectangle
				CRect rect;
				m_treeCtrl.GetItemRect(hItem, &rect, FALSE);

				// Draw custom background
				CDC * pDC = CDC::FromHandle(pCustomDraw->nmcd.hdc);
				pDC->FillSolidRect(&rect, RGB(40, 40, 40));  // Black background

				// Custom selection colors
				pCustomDraw->clrText = RGB(205, 205, 205);  // White text
				pCustomDraw->clrTextBk = RGB(100, 0, 0);  // Blue background (Windows 10 style)
				*pResult = CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;  // Tell it we changed colors
			}
			else
			{
				*pResult = CDRF_DODEFAULT;  // Tell it we changed colors
			}
		}
		break;

		case CDDS_ITEMPOSTPAINT:
		{
			HTREEITEM hItem = (HTREEITEM)pCustomDraw->nmcd.dwItemSpec;

			if (m_treeCtrl.GetSelectedItem() == hItem)
			{
				// Get full item rectangle
				CRect rect;
				m_treeCtrl.GetItemRect(hItem, &rect, TRUE);

				// Draw custom background
				CDC * pDC = CDC::FromHandle(pCustomDraw->nmcd.hdc);
				CBrush brush(RGB(40, 40, 110));
				pDC->FillRect(&rect, &brush);

				// Redraw text
				CString text = m_treeCtrl.GetItemText(hItem);
				pDC->SetTextColor(RGB(205, 205, 205));
				pDC->SetBkColor(RGB(0, 100, 0));
				pDC->SetBkMode(TRANSPARENT);
				pDC->DrawText(text, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
			}
			*pResult = CDRF_DODEFAULT;
		}
		break;

		default:
			*pResult = CDRF_DODEFAULT;
			break;
	}
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

void CProjectTreeDlg::OnTvnSelchangedTreeControl(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
