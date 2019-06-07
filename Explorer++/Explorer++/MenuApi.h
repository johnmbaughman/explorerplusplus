// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PluginMenuManager.h"
#include "../ThirdParty/Sol/sol.hpp"
#include <boost/signals2.hpp>
#include <unordered_map>

namespace Plugins
{
	class MenuApi
	{
	public:

		MenuApi(PluginMenuManager *pluginMenuManager);
		~MenuApi();

		boost::optional<int> create(const std::wstring &text, sol::protected_function callback);
		void remove(int menuItemId);

	private:

		void onMenuItemClicked(int menuItemId);

		PluginMenuManager *m_pluginMenuManager;

		boost::signals2::connection m_menuClickedConnection;
		std::unordered_map<int, sol::protected_function> m_pluginMenuItems;
	};
}