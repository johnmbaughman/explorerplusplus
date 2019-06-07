// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BookmarkHelper.h"
#include "../Helper/Bookmark.h"
#include <unordered_map>

class CBookmarkListView
{
public:

	CBookmarkListView(HWND hListView);
	~CBookmarkListView();

	void							InsertBookmarksIntoListView(const CBookmarkFolder &BookmarkFolder);
	int								InsertBookmarkFolderIntoListView(const CBookmarkFolder &BookmarkFolder, int iPosition);
	int								InsertBookmarkIntoListView(const CBookmark &Bookmark, int iPosition);
	VariantBookmark					&GetBookmarkItemFromListView(CBookmarkFolder &ParentBookmarkFolder, int iItem);
	VariantBookmark					&GetBookmarkItemFromListViewlParam(CBookmarkFolder &ParentBookmarkFolder, LPARAM lParam);

private:

	int								InsertBookmarkItemIntoListView(const std::wstring &strName, const GUID &guid, bool bFolder, int iPosition);

	HWND							m_hListView;
	HIMAGELIST						m_himl;

	std::unordered_map<UINT, GUID>	m_mapID;
	UINT							m_uIDCounter;
};