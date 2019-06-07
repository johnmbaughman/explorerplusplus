// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "UiTheming.h"

UiTheming::UiTheming(IExplorerplusplus *expp, CTabContainer *tabContainer, TabContainerInterface *tabContainerInterface) :
	m_expp(expp),
	m_tabContainer(tabContainer),
	m_tabContainerInterface(tabContainerInterface),
	m_customListViewColorsApplied(false)
{
	m_tabCreatedConnection = m_tabContainerInterface->AddTabCreatedObserver(boost::bind(&UiTheming::OnTabCreated, this, _1, _2));
}

UiTheming::~UiTheming()
{
	m_tabCreatedConnection.disconnect();
}

void UiTheming::OnTabCreated(int tabId, BOOL switchToNewTab)
{
	UNREFERENCED_PARAMETER(switchToNewTab);

	const Tab &tab = m_tabContainer->GetTab(tabId);

	if (m_customListViewColorsApplied)
	{
		ApplyListViewColorsForTab(tab, m_listViewBackgroundColor, m_listViewTextColor);
	}
}

bool UiTheming::SetListViewColors(COLORREF backgroundColor, COLORREF textColor)
{
	m_customListViewColorsApplied = true;
	m_listViewBackgroundColor = backgroundColor;
	m_listViewTextColor = textColor;

	bool res = ApplyListViewColorsForAllTabs(backgroundColor, textColor);

	return res;
}

bool UiTheming::ApplyListViewColorsForAllTabs(COLORREF backgroundColor, COLORREF textColor)
{
	bool overallResult = true;

	for (const auto &item : m_tabContainer->GetAllTabs())
	{
		bool res = ApplyListViewColorsForTab(item.second, backgroundColor, textColor);

		if (!res)
		{
			overallResult = false;
		}
	}

	return overallResult;
}

bool UiTheming::ApplyListViewColorsForTab(const Tab &tab, COLORREF backgroundColor, COLORREF textColor)
{
	BOOL bkRes = ListView_SetBkColor(tab.listView, backgroundColor);
	BOOL textBkRes = ListView_SetTextBkColor(tab.listView, backgroundColor);
	BOOL textRes = ListView_SetTextColor(tab.listView, textColor);

	InvalidateRect(tab.listView, nullptr, TRUE);

	if (!bkRes || !textBkRes || !textRes)
	{
		return false;
	}

	return true;
}

void UiTheming::SetTreeViewColors(COLORREF backgroundColor, COLORREF textColor)
{
	TreeView_SetBkColor(m_expp->GetTreeView(), backgroundColor);
	TreeView_SetTextColor(m_expp->GetTreeView(), textColor);

	InvalidateRect(m_expp->GetTreeView(), nullptr, TRUE);
}