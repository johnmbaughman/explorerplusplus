// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "ApplicationToolbar.h"
#include "BookmarksToolbar.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "ToolbarButtons.h"
#include "../Helper/Controls.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/WindowHelper.h"

HWND Explorerplusplus::CreateTabToolbar(HWND hParent,int idCommand,TCHAR *szTip)
{
	HWND TabToolbar = CreateToolbar(hParent,WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|
		TBSTYLE_TOOLTIPS|TBSTYLE_LIST|TBSTYLE_TRANSPARENT|TBSTYLE_FLAT|CCS_NODIVIDER|
		CCS_NOPARENTALIGN|CCS_NORESIZE,TBSTYLE_EX_MIXEDBUTTONS|TBSTYLE_EX_DOUBLEBUFFER);
	
	SendMessage(TabToolbar,TB_SETBITMAPSIZE,0,MAKELONG(7,7));
	SendMessage(TabToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
	SendMessage(TabToolbar,TB_SETBUTTONSIZE,0,MAKELPARAM(16,16));

	/* TODO: The image list is been leaked. */
	HIMAGELIST himl = ImageList_Create(7,7,ILC_COLOR32|ILC_MASK,0,2);
	HBITMAP hb = LoadBitmap(GetModuleHandle(0),MAKEINTRESOURCE(IDB_TABTOOLBAR_CLOSE));
	int iIndex = ImageList_Add(himl,hb,NULL);
	SendMessage(TabToolbar,TB_SETIMAGELIST,0,reinterpret_cast<LPARAM>(himl));
	DeleteObject(hb);

	/* Add the close button, used to close tabs. */
	TBBUTTON tbButton;
	tbButton.iBitmap	= iIndex;
	tbButton.idCommand	= idCommand;
	tbButton.fsState	= TBSTATE_ENABLED;
	tbButton.fsStyle	= TBSTYLE_BUTTON|TBSTYLE_AUTOSIZE;
	tbButton.dwData		= 0;
	tbButton.iString	= reinterpret_cast<INT_PTR>(szTip);
	SendMessage(TabToolbar,TB_INSERTBUTTON,0,reinterpret_cast<LPARAM>(&tbButton));

	SendMessage(TabToolbar,TB_AUTOSIZE,0,0);

	return TabToolbar;
}

void Explorerplusplus::ResizeWindows(void)
{
	RECT rc;

	GetClientRect(m_hContainer,&rc);
	SendMessage(m_hContainer,WM_SIZE,
	SIZE_RESTORED,(LPARAM)MAKELPARAM(rc.right,rc.bottom));
}

/* TODO: This should be linked to OnSize(). */
void Explorerplusplus::SetListViewInitialPosition(HWND hListView)
{
	RECT			rc;
	int				MainWindowWidth;
	int				MainWindowHeight;
	int				IndentBottom = 0;
	int				IndentTop = 0;
	int				IndentLeft = 0;
	int				iIndentRebar = 0;

	GetClientRect(m_hContainer,&rc);

	MainWindowWidth = GetRectWidth(&rc);
	MainWindowHeight = GetRectHeight(&rc);

	if(m_hMainRebar)
	{
		GetWindowRect(m_hMainRebar,&rc);
		iIndentRebar += GetRectHeight(&rc);
	}

	if(m_config->showStatusBar)
	{
		GetWindowRect(m_hStatusBar,&rc);
		IndentBottom += GetRectHeight(&rc);
	}

	if(m_config->showDisplayWindow)
	{
		IndentBottom += m_config->displayWindowHeight;
	}

	if(m_config->showFolders)
	{
		GetClientRect(m_hHolder,&rc);
		IndentLeft = GetRectWidth(&rc);
	}

	IndentTop = iIndentRebar;

	if(m_bShowTabBar)
	{
		if(!m_config->showTabBarAtBottom)
		{
			IndentTop += TAB_WINDOW_HEIGHT;
		}
	}

	if(!m_config->showTabBarAtBottom)
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
	else
	{
		SetWindowPos(hListView,NULL,IndentLeft,IndentTop,
			MainWindowWidth - IndentLeft,MainWindowHeight -
			IndentBottom - IndentTop - TAB_WINDOW_HEIGHT,
			SWP_HIDEWINDOW|SWP_NOZORDER);
	}
}

void Explorerplusplus::ToggleFolders(void)
{
	m_config->showFolders = !m_config->showFolders;
	lShowWindow(m_hHolder, m_config->showFolders);
	lShowWindow(m_hTreeView, m_config->showFolders);

	SendMessage(m_mainToolbar->GetHWND(),TB_CHECKBUTTON,(WPARAM)TOOLBAR_FOLDERS,(LPARAM)m_config->showFolders);
	ResizeWindows();
}

void Explorerplusplus::UpdateLayout()
{
	RECT rc;
	GetClientRect(m_hContainer, &rc);
	SendMessage(m_hContainer, WM_SIZE, SIZE_RESTORED,
		MAKELPARAM(rc.right, rc.bottom));
}