﻿// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <list>
#include "ShellBrowser/iShellView.h"
#include "../Helper/StatusBar.h"

/* Used to share global data across the
classes within the Explorer++ project. */
namespace NExplorerplusplus
{
	const TCHAR APP_NAME[]			= _T("Explorer++");
	const TCHAR CLASS_NAME[]		= _T("Explorer++");

	const TCHAR HELP_FILE_NAME[]	= _T("Explorer++ Help.chm");

	/* Registry keys used to store program settings. */
	const TCHAR REG_MAIN_KEY[]		= _T("Software\\Explorer++");
	const TCHAR REG_SETTINGS_KEY[]	= _T("Software\\Explorer++\\Settings");

	/* The name of the XML file that preferences are
	saved to/loaded from. */
	const TCHAR XML_FILENAME[]		= _T("config.xml");

	const TCHAR LOG_FILENAME[]		= _T("Explorer++.log");

	/* Command line arguments supplied to the program
	for each jump list task. */
	const TCHAR JUMPLIST_TASK_NEWTAB_ARGUMENT[]	= _T("-open_new_tab");
}

/* Basic interface between Explorerplusplus
and some of the other components (such as the
dialogs and toolbars). */
__interface IExplorerplusplus
{
	HWND			GetActiveListView() const;
	CShellBrowser	*GetActiveShellBrowser() const;

	HWND			GetTreeView() const;

	std::wstring	GetTabName(int iTab);
	void			SetTabName(int iTab,std::wstring strName,BOOL bUseCustomName);
	void			RefreshTab(int iTabId);
	int				GetCurrentTabId() const;

	/* Temporary. */
	void			SetTabSelection(int Index);

	void			OpenItem(LPCITEMIDLIST pidlItem,BOOL bOpenInNewTab,BOOL bOpenInNewWindow);

	CStatusBar		*GetStatusBar();

	HRESULT			BrowseFolder(const TCHAR *szPath,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);
	HRESULT			BrowseFolder(LPCITEMIDLIST pidlDirectory,UINT wFlags,BOOL bOpenInNewTab,BOOL bSwitchToNewTab,BOOL bOpenInNewWindow);

	void			OpenFileItem(LPCITEMIDLIST pidlItem,const TCHAR *szParameters);

	HMENU			BuildViewsMenu();

	BOOL			CanCut() const;
	BOOL			CanCopy() const;
	BOOL			CanRename() const;
	BOOL			CanDelete() const;
	BOOL			CanShowFileProperties() const;
	BOOL			CanPaste() const;
};

/* Used when setting Explorer++ as the default
file manager. */
static const TCHAR SHELL_DEFAULT_INTERNAL_COMMAND_NAME[] = _T("openinexplorer++");

#define WM_APP_ASSOCCHANGED		(WM_APP + 54)
#define WM_APP_KEYDOWN			(WM_APP + 55)

/* Rebar menu id's. */
#define ID_REBAR_MENU_BACK_START	2000
#define ID_REBAR_MENU_BACK_END		2999
#define ID_REBAR_MENU_FORWARD_START	3000
#define ID_REBAR_MENU_FORWARD_END	3999

#define VALIDATE_REALFOLDER_COLUMNS			0
#define VALIDATE_CONTROLPANEL_COLUMNS		1
#define VALIDATE_MYCOMPUTER_COLUMNS			2
#define VALIDATE_RECYCLEBIN_COLUMNS			3
#define VALIDATE_PRINTERS_COLUMNS			4
#define VALIDATE_NETWORKCONNECTIONS_COLUMNS	5
#define VALIDATE_MYNETWORKPLACES_COLUMNS	6

extern BOOL g_bForceLanguageLoad;
extern TCHAR g_szLang[32];

BOOL TestConfigFileInternal(void);