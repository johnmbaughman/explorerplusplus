// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ViewModes.h"
#include "MainResource.h"

int GetViewModeMenuId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::Thumbnails:
		return IDM_VIEW_THUMBNAILS;
		break;

	case ViewMode::Tiles:
		return IDM_VIEW_TILES;
		break;

	case ViewMode::ExtraLargeIcons:
		return IDM_VIEW_EXTRALARGEICONS;
		break;

	case ViewMode::LargeIcons:
		return IDM_VIEW_LARGEICONS;
		break;

	case ViewMode::Icons:
		return IDM_VIEW_ICONS;
		break;

	case ViewMode::SmallIcons:
		return IDM_VIEW_SMALLICONS;
		break;

	case ViewMode::List:
		return IDM_VIEW_LIST;
		break;

	case ViewMode::Details:
		return IDM_VIEW_DETAILS;
		break;
	}

	return -1;
}

int GetViewModeMenuStringId(ViewMode viewMode)
{
	switch (viewMode)
	{
	case ViewMode::Thumbnails:
		return IDS_VIEW_THUMBNAILS;
		break;

	case ViewMode::Tiles:
		return IDS_VIEW_TILES;
		break;

	case ViewMode::ExtraLargeIcons:
		return IDS_VIEW_EXTRALARGEICONS;
		break;

	case ViewMode::LargeIcons:
		return IDS_VIEW_LARGEICONS;
		break;

	case ViewMode::Icons:
		return IDS_VIEW_MEDIUMICONS;
		break;

	case ViewMode::SmallIcons:
		return IDS_VIEW_SMALLICONS;
		break;

	case ViewMode::List:
		return IDS_VIEW_LIST;
		break;

	case ViewMode::Details:
		return IDS_VIEW_DETAILS;
		break;
	}

	return -1;
}