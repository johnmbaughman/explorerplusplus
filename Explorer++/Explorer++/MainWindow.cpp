// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MainWindow.h"
#include "Config.h"
#include "Explorer++_internal.h"
#include "MainResource.h"
#include "TabContainer.h"
#include "../Helper/PIDLWrapper.h"
#include "../Helper/ProcessHelper.h"

MainWindow *MainWindow::Create(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE instance,
	IExplorerplusplus *expp, TabContainerInterface *tabContainer)
{
	return new MainWindow(hwnd, config, instance, expp, tabContainer);
}

MainWindow::MainWindow(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE instance,
	IExplorerplusplus *expp, TabContainerInterface *tabContainerInterface) :
	CBaseWindow(hwnd),
	m_hwnd(hwnd),
	m_config(config),
	m_instance(instance),
	m_expp(expp),
	m_tabContainerInterface(tabContainerInterface)
{
	m_navigationCompletedConnection = m_tabContainerInterface->AddNavigationCompletedObserver(boost::bind(&MainWindow::OnNavigationCompleted, this, _1));
	m_tabSelectedConnection = m_tabContainerInterface->AddTabSelectedObserver(boost::bind(&MainWindow::OnTabSelected, this, _1));

	m_showFillTitlePathConnection = m_config->showFullTitlePath.addObserver(boost::bind(&MainWindow::OnShowFullTitlePathUpdated, this, _1));
	m_showUserNameInTitleBarConnection = m_config->showUserNameInTitleBar.addObserver(boost::bind(&MainWindow::OnShowUserNameInTitleBarUpdated, this, _1));
	m_showPrivilegeLevelInTitleBarConnection = m_config->showPrivilegeLevelInTitleBar.addObserver(boost::bind(&MainWindow::OnShowPrivilegeLevelInTitleBarUpdated, this, _1));
}

MainWindow::~MainWindow()
{
	m_navigationCompletedConnection.disconnect();
	m_tabSelectedConnection.disconnect();

	m_showFillTitlePathConnection.disconnect();
	m_showUserNameInTitleBarConnection.disconnect();
	m_showPrivilegeLevelInTitleBarConnection.disconnect();
}

void MainWindow::OnNavigationCompleted(const Tab &tab)
{
	if (m_expp->GetTabContainer()->IsTabSelected(tab))
	{
		UpdateWindowText();
	}
}

void MainWindow::OnTabSelected(const Tab &tab)
{
	UNREFERENCED_PARAMETER(tab);

	UpdateWindowText();
}

void MainWindow::OnShowFullTitlePathUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void MainWindow::OnShowUserNameInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void MainWindow::OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue)
{
	UNREFERENCED_PARAMETER(newValue);

	UpdateWindowText();
}

void MainWindow::UpdateWindowText()
{
	const Tab &tab = m_expp->GetTabContainer()->GetSelectedTab();
	PIDLPointer pidlDirectory(tab.GetShellBrowser()->QueryCurrentDirectoryIdl());

	TCHAR szFolderDisplayName[MAX_PATH];

	/* Don't show full paths for virtual folders (as only the folders
	GUID will be shown). */
	if (m_config->showFullTitlePath.get() && !tab.GetShellBrowser()->InVirtualFolder())
	{
		GetDisplayName(pidlDirectory.get(), szFolderDisplayName, SIZEOF_ARRAY(szFolderDisplayName), SHGDN_FORPARSING);
	}
	else
	{
		GetDisplayName(pidlDirectory.get(), szFolderDisplayName, SIZEOF_ARRAY(szFolderDisplayName), SHGDN_NORMAL);
	}

	TCHAR szTitle[512];

	TCHAR szTemp[64];
	LoadString(m_instance, IDS_MAIN_WINDOW_TITLE, szTemp, SIZEOF_ARRAY(szTemp));
	StringCchPrintf(szTitle, SIZEOF_ARRAY(szTitle),
		szTemp, szFolderDisplayName, NExplorerplusplus::APP_NAME);

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), _T(" ["));
	}

	if (m_config->showUserNameInTitleBar.get())
	{
		TCHAR szOwner[512];
		GetProcessOwner(GetCurrentProcessId(), szOwner, SIZEOF_ARRAY(szOwner));

		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), szOwner);
	}

	if (m_config->showPrivilegeLevelInTitleBar.get())
	{
		TCHAR szPrivilegeAddition[64];
		TCHAR szPrivilege[64];

		if (CheckGroupMembership(GROUP_ADMINISTRATORS))
		{
			LoadString(m_instance, IDS_PRIVILEGE_LEVEL_ADMINISTRATORS, szPrivilege, SIZEOF_ARRAY(szPrivilege));
		}
		else if (CheckGroupMembership(GROUP_POWERUSERS))
		{
			LoadString(m_instance, IDS_PRIVILEGE_LEVEL_POWER_USERS, szPrivilege, SIZEOF_ARRAY(szPrivilege));
		}
		else if (CheckGroupMembership(GROUP_USERS))
		{
			LoadString(m_instance, IDS_PRIVILEGE_LEVEL_USERS, szPrivilege, SIZEOF_ARRAY(szPrivilege));
		}
		else if (CheckGroupMembership(GROUP_USERSRESTRICTED))
		{
			LoadString(m_instance, IDS_PRIVILEGE_LEVEL_USERS_RESTRICTED, szPrivilege, SIZEOF_ARRAY(szPrivilege));
		}

		if (m_config->showUserNameInTitleBar.get())
		{
			StringCchPrintf(szPrivilegeAddition, SIZEOF_ARRAY(szPrivilegeAddition),
				_T(" - %s"), szPrivilege);
		}
		else
		{
			StringCchPrintf(szPrivilegeAddition, SIZEOF_ARRAY(szPrivilegeAddition),
				_T("%s"), szPrivilege);
		}

		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), szPrivilegeAddition);
	}

	if (m_config->showUserNameInTitleBar.get() || m_config->showPrivilegeLevelInTitleBar.get())
	{
		StringCchCat(szTitle, SIZEOF_ARRAY(szTitle), _T("]"));
	}

	SetWindowText(m_hwnd, szTitle);
}