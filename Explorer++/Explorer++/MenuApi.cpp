// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "MenuApi.h"

Plugins::MenuApi::MenuApi(PluginMenuManager *pluginMenuManager) :
	m_pluginMenuManager(pluginMenuManager)
{
	m_menuClickedConnection = m_pluginMenuManager->AddMenuClickedObserver(boost::bind(&Plugins::MenuApi::onMenuItemClicked, this, _1));
}

Plugins::MenuApi::~MenuApi()
{
	for (auto item : m_pluginMenuItems)
	{
		m_pluginMenuManager->RemoveItemFromMainMenu(item.first);
	}

	m_menuClickedConnection.disconnect();
}

boost::optional<int> Plugins::MenuApi::create(const std::wstring &text, sol::protected_function callback)
{
	auto menuItemId = m_pluginMenuManager->AddItemToMainMenu(text);

	if (!menuItemId)
	{
		return menuItemId;
	}

	m_pluginMenuItems.insert(std::make_pair(*menuItemId, callback));

	return menuItemId;
}

void Plugins::MenuApi::remove(int menuItemId)
{
	auto itr = m_pluginMenuItems.find(menuItemId);

	if (itr == m_pluginMenuItems.end())
	{
		return;
	}

	m_pluginMenuManager->RemoveItemFromMainMenu(menuItemId);

	m_pluginMenuItems.erase(itr);
}

void Plugins::MenuApi::onMenuItemClicked(int menuItemId)
{
	auto itr = m_pluginMenuItems.find(menuItemId);

	if (itr == m_pluginMenuItems.end())
	{
		return;
	}

	itr->second();
}