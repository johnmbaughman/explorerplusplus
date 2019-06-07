// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "LuaPlugin.h"
#include "PluginInterface.h"
#include <boost/filesystem.hpp>

namespace Plugins
{
	class PluginManager
	{
	public:

		PluginManager(PluginInterface *pluginInterface);
		~PluginManager();

		void loadAllPlugins(const boost::filesystem::path &pluginDirectory);

	private:

		static const std::wstring MANIFEST_NAME;

		bool attemptToLoadPlugin(const boost::filesystem::path &directory);
		bool registerPlugin(const boost::filesystem::path &directory, const Manifest &manifest);

		PluginInterface *m_pluginInterface;

		std::vector<std::unique_ptr<Plugins::LuaPlugin>> m_plugins;
	};
}