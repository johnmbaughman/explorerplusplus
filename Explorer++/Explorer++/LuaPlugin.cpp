// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LuaPlugin.h"
#include "APIBinding.h"

int Plugins::LuaPlugin::idCounter = 1;

inline int onPanic(lua_State *L);

Plugins::LuaPlugin::LuaPlugin(const std::wstring &directory, const Manifest &manifest,
	PluginInterface *pluginInterface) :
	m_directory(directory),
	m_manifest(manifest),
	m_lua(onPanic),
	m_id(idCounter++)
{
	BindAllApiMethods(m_id, m_lua, pluginInterface);
}

Plugins::LuaPlugin::~LuaPlugin()
{

}

int Plugins::LuaPlugin::GetId() const
{
	return m_id;
}

std::wstring Plugins::LuaPlugin::GetDirectory() const
{
	return m_directory;
}

Plugins::Manifest Plugins::LuaPlugin::GetManifest() const
{
	return m_manifest;
}

sol::state &Plugins::LuaPlugin::GetLuaState()
{
	return m_lua;
}

inline int onPanic(lua_State *L)
{
	UNREFERENCED_PARAMETER(L);

	throw Plugins::LuaPanicException("A Lua panic occurred.");
}