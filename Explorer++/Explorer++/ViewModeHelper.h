// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellBrowser/ViewModes.h"

template<std::size_t N>
ViewMode GetNextViewMode(const std::array<ViewMode, N> &viewModes, ViewMode viewMode)
{
	auto itr = std::find(viewModes.begin(), viewModes.end(), viewMode);
	assert(itr != viewModes.end());

	itr++;

	if (itr == viewModes.end())
	{
		itr = viewModes.begin();
	}

	return *itr;
}

template<std::size_t N>
ViewMode GetPreviousViewMode(const std::array<ViewMode, N> &viewModes, ViewMode viewMode)
{
	auto itr = std::find(viewModes.begin(), viewModes.end(), viewMode);
	assert(itr != viewModes.end());

	if (itr == viewModes.begin())
	{
		itr = viewModes.end() - 1;
	}
	else
	{
		itr--;
	}

	return *itr;
}