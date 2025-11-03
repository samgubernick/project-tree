
// Project Tree.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "Project TreeDlg.h"


// CProjectTreeApp:
// See Project Tree.cpp for the implementation of this class
//

class CProjectTreeApp : public CWinApp
{
public:
	CProjectTreeApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
private:
	CProjectTreeDlg * m_pMainDlg;
};

extern CProjectTreeApp theApp;
