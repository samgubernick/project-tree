
// Project Tree.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "Project Tree.h"
#include "Project TreeDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CProjectTreeApp

BEGIN_MESSAGE_MAP(CProjectTreeApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CProjectTreeApp construction

CProjectTreeApp::CProjectTreeApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CProjectTreeApp object

CProjectTreeApp theApp;


// CProjectTreeApp initialization

BOOL CProjectTreeApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Activate "Windows Native" visual manager for enabling themes in MFC controls
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	//CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
	//CMFCVisualManager::GetInstance()->SetStyle(CMFCVisualManagerOffice2007::Office2007_Black);

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	m_pMainDlg = new CProjectTreeDlg();
	m_pMainDlg->Create(IDD_PROJECT_TREE_DIALOG, nullptr);
	m_pMainDlg->ShowWindow(SW_SHOW);
	m_pMainWnd = m_pMainDlg;

	// Delete the shell manager created above.
	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	return TRUE;
}

