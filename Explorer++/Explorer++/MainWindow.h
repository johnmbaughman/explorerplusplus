// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "TabContainerInterface.h"
#include "../Helper/BaseWindow.h"

struct Config;

class MainWindow : CBaseWindow
{
public:

	static MainWindow *Create(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE instance,
		IExplorerplusplus *expp, TabContainerInterface *tabContainer);

private:

	MainWindow(HWND hwnd, std::shared_ptr<Config> config, HINSTANCE instance,
		IExplorerplusplus *expp, TabContainerInterface *tabContainerInterface);
	~MainWindow();

	void OnNavigationCompleted(const Tab &tab);
	void OnTabSelected(const Tab &tab);

	void OnShowFullTitlePathUpdated(BOOL newValue);
	void OnShowUserNameInTitleBarUpdated(BOOL newValue);
	void OnShowPrivilegeLevelInTitleBarUpdated(BOOL newValue);

	void UpdateWindowText();

	HWND m_hwnd;
	std::shared_ptr<Config> m_config;
	HINSTANCE m_instance;
	IExplorerplusplus *m_expp;
	TabContainerInterface *m_tabContainerInterface;

	boost::signals2::connection m_navigationCompletedConnection;
	boost::signals2::connection m_tabSelectedConnection;

	boost::signals2::connection m_showFillTitlePathConnection;
	boost::signals2::connection m_showUserNameInTitleBarConnection;
	boost::signals2::connection m_showPrivilegeLevelInTitleBarConnection;
};