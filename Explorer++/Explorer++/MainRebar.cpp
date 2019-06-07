// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "AddressBar.h"
#include "ApplicationToolbar.h"
#include "BookmarksToolbar.h"
#include "Config.h"
#include "DrivesToolbar.h"
#include "MainResource.h"
#include "../Helper/Controls.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/MenuWrapper.h"
#include "../Helper/WindowHelper.h"

LRESULT CALLBACK RebarSubclassStub(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

static const int TOOLBAR_BOOKMARK_START = TOOLBAR_ID_START + 1000;
static const int TOOLBAR_BOOKMARK_END = TOOLBAR_BOOKMARK_START + 1000;
static const int TOOLBAR_DRIVES_ID_START = TOOLBAR_BOOKMARK_END + 1;
static const int TOOLBAR_DRIVES_ID_END = TOOLBAR_DRIVES_ID_START + 1000;
static const int TOOLBAR_APPLICATIONS_ID_START = TOOLBAR_DRIVES_ID_END + 1;
static const int TOOLBAR_APPLICATIONS_ID_END = TOOLBAR_APPLICATIONS_ID_START + 1000;

DWORD BookmarkToolbarStyles = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
TBSTYLE_TOOLTIPS | TBSTYLE_LIST | TBSTYLE_TRANSPARENT |
TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NORESIZE;

DWORD RebarStyles = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
WS_BORDER | CCS_NODIVIDER | CCS_TOP | CCS_NOPARENTALIGN |
RBS_BANDBORDERS | RBS_VARHEIGHT;

void Explorerplusplus::InitializeMainToolbars(void)
{
	/* Initialize the main toolbar styles and settings here. The visibility and gripper
	styles will be set after the settings have been loaded (needed to keep compatibility
	with versions older than 0.9.5.4). */
	m_ToolbarInformation[0].wID = ID_MAINTOOLBAR;
	m_ToolbarInformation[0].fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[0].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[0].cx = 0;
	m_ToolbarInformation[0].cxIdeal = 0;
	m_ToolbarInformation[0].cxMinChild = 0;
	m_ToolbarInformation[0].cyIntegral = 0;
	m_ToolbarInformation[0].cxHeader = 0;
	m_ToolbarInformation[0].lpText = NULL;

	m_ToolbarInformation[1].wID = ID_ADDRESSTOOLBAR;
	m_ToolbarInformation[1].fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE | RBBIM_TEXT;
	m_ToolbarInformation[1].fStyle = RBBS_BREAK;
	m_ToolbarInformation[1].cx = 0;
	m_ToolbarInformation[1].cxIdeal = 0;
	m_ToolbarInformation[1].cxMinChild = 0;
	m_ToolbarInformation[1].cyIntegral = 0;
	m_ToolbarInformation[1].cxHeader = 0;
	m_ToolbarInformation[1].lpText = NULL;

	m_ToolbarInformation[2].wID = ID_BOOKMARKSTOOLBAR;
	m_ToolbarInformation[2].fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[2].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[2].cx = 0;
	m_ToolbarInformation[2].cxIdeal = 0;
	m_ToolbarInformation[2].cxMinChild = 0;
	m_ToolbarInformation[2].cyIntegral = 0;
	m_ToolbarInformation[2].cxHeader = 0;
	m_ToolbarInformation[2].lpText = NULL;

	m_ToolbarInformation[3].wID = ID_DRIVESTOOLBAR;
	m_ToolbarInformation[3].fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[3].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[3].cx = 0;
	m_ToolbarInformation[3].cxIdeal = 0;
	m_ToolbarInformation[3].cxMinChild = 0;
	m_ToolbarInformation[3].cyIntegral = 0;
	m_ToolbarInformation[3].cxHeader = 0;
	m_ToolbarInformation[3].lpText = NULL;

	m_ToolbarInformation[4].wID = ID_APPLICATIONSTOOLBAR;
	m_ToolbarInformation[4].fMask = RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	m_ToolbarInformation[4].fStyle = RBBS_BREAK | RBBS_USECHEVRON;
	m_ToolbarInformation[4].cx = 0;
	m_ToolbarInformation[4].cxIdeal = 0;
	m_ToolbarInformation[4].cxMinChild = 0;
	m_ToolbarInformation[4].cyIntegral = 0;
	m_ToolbarInformation[4].cxHeader = 0;
	m_ToolbarInformation[4].lpText = NULL;
}

void Explorerplusplus::CreateMainControls(void)
{
	SIZE	sz;
	RECT	rc;
	DWORD	ToolbarSize;
	TCHAR	szBandText[32];
	int		i = 0;

	/* If the rebar is locked, prevent bands from
	been rearranged. */
	if (m_config->lockToolbars)
		RebarStyles |= RBS_FIXEDORDER;

	/* Create and subclass the main rebar control. */
	m_hMainRebar = CreateWindowEx(0, REBARCLASSNAME, EMPTY_STRING, RebarStyles,
		0, 0, 0, 0, m_hContainer, NULL, GetModuleHandle(0), NULL);
	SetWindowSubclass(m_hMainRebar, RebarSubclassStub, 0, (DWORD_PTR)this);

	for (i = 0; i < NUM_MAIN_TOOLBARS; i++)
	{
		switch (m_ToolbarInformation[i].wID)
		{
		case ID_MAINTOOLBAR:
			CreateMainToolbar();
			ToolbarSize = (DWORD)SendMessage(m_mainToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_mainToolbar->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM)&sz);

			if (m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_mainToolbar->GetHWND();
			break;

		case ID_ADDRESSTOOLBAR:
			CreateAddressBar();
			LoadString(m_hLanguageModule, IDS_ADDRESSBAR, szBandText, SIZEOF_ARRAY(szBandText));
			GetWindowRect(m_addressBar->GetHWND(), &rc);
			m_ToolbarInformation[i].cyMinChild = GetRectHeight(&rc);
			m_ToolbarInformation[i].lpText = szBandText;
			m_ToolbarInformation[i].hwndChild = m_addressBar->GetHWND();
			break;

		case ID_BOOKMARKSTOOLBAR:
			CreateBookmarksToolbar();
			ToolbarSize = (DWORD)SendMessage(m_hBookmarksToolbar, TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_hBookmarksToolbar, TB_GETMAXSIZE, 0, (LPARAM)&sz);

			if (m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_hBookmarksToolbar;
			break;

		case ID_DRIVESTOOLBAR:
			CreateDrivesToolbar();
			ToolbarSize = (DWORD)SendMessage(m_pDrivesToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_pDrivesToolbar->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM)&sz);

			if (m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_pDrivesToolbar->GetHWND();
			break;

		case ID_APPLICATIONSTOOLBAR:
			CreateApplicationToolbar();
			ToolbarSize = (DWORD)SendMessage(m_pApplicationToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);
			m_ToolbarInformation[i].cyMinChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyMaxChild = HIWORD(ToolbarSize);
			m_ToolbarInformation[i].cyChild = HIWORD(ToolbarSize);
			SendMessage(m_pApplicationToolbar->GetHWND(), TB_GETMAXSIZE, 0, (LPARAM)&sz);

			if (m_ToolbarInformation[i].cx == 0)
				m_ToolbarInformation[i].cx = sz.cx;

			m_ToolbarInformation[i].cxIdeal = sz.cx;
			m_ToolbarInformation[i].hwndChild = m_pApplicationToolbar->GetHWND();
			break;
		}

		m_ToolbarInformation[i].cbSize = sizeof(REBARBANDINFO);
		SendMessage(m_hMainRebar, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&m_ToolbarInformation[i]);
	}
}

LRESULT CALLBACK RebarSubclassStub(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pContainer = (Explorerplusplus *)dwRefData;

	return pContainer->RebarSubclass(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK Explorerplusplus::RebarSubclass(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITMENU:
		SendMessage(m_hContainer, WM_INITMENU, wParam, lParam);
		break;

	case WM_MENUSELECT:
		SendMessage(m_hContainer, WM_MENUSELECT, wParam, lParam);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case NM_RCLICK:
		{
			LPNMMOUSE pnmm = reinterpret_cast<LPNMMOUSE>(lParam);
			OnToolbarRClick(pnmm->hdr.hwndFrom);
		}
		return TRUE;
		break;
		}
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void Explorerplusplus::OnToolbarRClick(HWND sourceWindow)
{
	auto parentMenu = MenuPtr(LoadMenu(m_hLanguageModule, MAKEINTRESOURCE(IDR_TOOLBAR_MENU)));

	if (!parentMenu)
	{
		return;
	}

	HMENU menu = GetSubMenu(parentMenu.get(), 0);

	lCheckMenuItem(menu, IDM_TOOLBARS_ADDRESSBAR, m_config->showAddressBar);
	lCheckMenuItem(menu, IDM_TOOLBARS_MAINTOOLBAR, m_config->showMainToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_BOOKMARKSTOOLBAR, m_config->showBookmarksToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_DRIVES, m_config->showDrivesToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_APPLICATIONTOOLBAR, m_config->showApplicationToolbar);
	lCheckMenuItem(menu, IDM_TOOLBARS_LOCKTOOLBARS, m_config->lockToolbars);

	DWORD dwPos = GetMessagePos();

	POINT ptCursor;
	ptCursor.x = GET_X_LPARAM(dwPos);
	ptCursor.y = GET_Y_LPARAM(dwPos);

	// Give any observers a chance to modify the menu.
	m_toolbarContextMenuSignal(menu, sourceWindow);

	TrackPopupMenu(menu, TPM_LEFTALIGN, ptCursor.x, ptCursor.y, 0, m_hMainRebar, NULL);
}

boost::signals2::connection Explorerplusplus::AddToolbarContextMenuObserver(const ToolbarContextMenuSignal::slot_type &observer)
{
	return m_toolbarContextMenuSignal.connect(observer);
}

void Explorerplusplus::CreateAddressBar()
{
	m_addressBar = AddressBar::Create(m_hMainRebar, this, this, m_mainToolbar);
}

void Explorerplusplus::CreateMainToolbar()
{
	m_mainToolbar = MainToolbar::Create(m_hMainRebar, m_hLanguageModule, this, this, m_config);
}

void Explorerplusplus::CreateBookmarksToolbar(void)
{
	m_hBookmarksToolbar = CreateToolbar(m_hMainRebar, BookmarkToolbarStyles,
		TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DRAWDDARROWS |
		TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_HIDECLIPPEDBUTTONS);

	m_pBookmarksToolbar = new CBookmarksToolbar(m_hBookmarksToolbar, m_hLanguageModule, this,
		this, *m_bfAllBookmarks, m_guidBookmarksToolbar, TOOLBAR_BOOKMARK_START, TOOLBAR_BOOKMARK_END);
}

void Explorerplusplus::CreateDrivesToolbar(void)
{
	m_pDrivesToolbar = CDrivesToolbar::Create(m_hMainRebar, TOOLBAR_DRIVES_ID_START,
		TOOLBAR_DRIVES_ID_END, m_hLanguageModule, this, this);
}

void Explorerplusplus::CreateApplicationToolbar()
{
	m_pApplicationToolbar = CApplicationToolbar::Create(m_hMainRebar, TOOLBAR_APPLICATIONS_ID_START,
		TOOLBAR_APPLICATIONS_ID_END, m_hLanguageModule, this);
}

void Explorerplusplus::AdjustMainToolbarSize(void)
{
	m_mainToolbar->UpdateToolbarSize();

	REBARBANDINFO rbi;
	DWORD dwSize;

	dwSize = (DWORD)SendMessage(m_mainToolbar->GetHWND(), TB_GETBUTTONSIZE, 0, 0);

	rbi.cbSize = sizeof(rbi);
	rbi.fMask = RBBIM_CHILDSIZE;
	rbi.cxMinChild = 0;
	rbi.cyMinChild = HIWORD(dwSize);
	rbi.cyChild = HIWORD(dwSize);
	rbi.cyMaxChild = HIWORD(dwSize);

	SendMessage(m_hMainRebar, RB_SETBANDINFO, 0, (LPARAM)&rbi);
}

HMENU Explorerplusplus::CreateRebarHistoryMenu(BOOL bBack)
{
	HMENU hSubMenu = NULL;
	std::list<LPITEMIDLIST> history;
	int iBase;

	if (bBack)
	{
		iBase = ID_REBAR_MENU_BACK_START;
		history = m_pActiveShellBrowser->GetBackHistory();
	}
	else
	{
		iBase = ID_REBAR_MENU_FORWARD_START;
		history = m_pActiveShellBrowser->GetForwardHistory();
	}

	if (history.size() > 0)
	{
		int i = 0;

		hSubMenu = CreateMenu();

		for (auto itr = history.begin(); itr != history.end(); itr++)
		{
			TCHAR szDisplayName[MAX_PATH];
			GetDisplayName(*itr, szDisplayName, SIZEOF_ARRAY(szDisplayName), SHGDN_INFOLDER);

			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_ID | MIIM_STRING;
			mii.wID = iBase + i + 1;
			mii.dwTypeData = szDisplayName;
			InsertMenuItem(hSubMenu, i, TRUE, &mii);

			i++;

			CoTaskMemFree(*itr);
		}

		history.clear();
	}

	return hSubMenu;
}