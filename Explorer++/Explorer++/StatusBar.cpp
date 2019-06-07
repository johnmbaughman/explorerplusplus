// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "MainResource.h"
#include "../Helper/Controls.h"
#include "../Helper/WindowHelper.h"

void Explorerplusplus::CreateStatusBar(void)
{
	UINT Style = WS_CHILD | WS_CLIPSIBLINGS | SBARS_SIZEGRIP | WS_CLIPCHILDREN;

	if (m_config->showStatusBar)
	{
		Style |= WS_VISIBLE;
	}

	m_hStatusBar = ::CreateStatusBar(m_hContainer, Style);
	m_pStatusBar = new CStatusBar(m_hStatusBar);

	int width = 0;

	RECT rc;
	BOOL res = GetWindowRect(m_hContainer, &rc);

	if (res)
	{
		width = GetRectWidth(&rc);
	}

	SetStatusBarParts(width);
}

void Explorerplusplus::SetStatusBarParts(int width)
{
	int Parts[3];

	Parts[0] = (int)(0.50 * width);
	Parts[1] = (int)(0.75 * width);
	Parts[2] = width;

	SendMessage(m_hStatusBar, SB_SETPARTS, 3, (LPARAM)Parts);
}

LRESULT Explorerplusplus::StatusBarMenuSelect(WPARAM wParam, LPARAM lParam)
{
	/* Is the menu being closed? .*/
	if (HIWORD(wParam) == 0xFFFF && lParam == 0)
	{
		m_pStatusBar->HandleStatusBarMenuClose();
	}
	else
	{
		m_pStatusBar->HandleStatusBarMenuOpen();

		TCHAR szBuffer[512];
		LoadString(m_hLanguageModule, LOWORD(wParam),
			szBuffer, SIZEOF_ARRAY(szBuffer));
		SetWindowText(m_hStatusBar, szBuffer);
	}

	return 0;
}

HRESULT Explorerplusplus::UpdateStatusBarText(void)
{
	FolderInfo_t	FolderInfo;
	int				nTotal;
	int				nFilesSelected;
	int				nFoldersSelected;
	TCHAR			szItemsSelected[64];
	TCHAR			lpszSizeBuffer[32];
	TCHAR			szBuffer[64];
	TCHAR			szTemp[64];
	TCHAR			*szNumSelected = NULL;
	int				res;

	nTotal = m_pActiveShellBrowser->GetNumItems();
	nFilesSelected = m_pActiveShellBrowser->GetNumSelectedFiles();
	nFoldersSelected = m_pActiveShellBrowser->GetNumSelectedFolders();

	if ((nFilesSelected + nFoldersSelected) != 0)
	{
		szNumSelected = PrintComma(nFilesSelected + nFoldersSelected);

		if ((nFilesSelected + nFoldersSelected) == 1)
		{
			LoadString(m_hLanguageModule, IDS_GENERAL_SELECTED_ONEITEM, szTemp,
				SIZEOF_ARRAY(szTemp));

			/* One item selected. Form:
			1 item selected */
			StringCchPrintf(szItemsSelected, SIZEOF_ARRAY(szItemsSelected),
				_T("%s %s"), szNumSelected, szTemp);
		}
		else
		{
			LoadString(m_hLanguageModule, IDS_GENERAL_SELECTED_MOREITEMS, szTemp,
				SIZEOF_ARRAY(szTemp));

			/* More than one item selected. Form:
			n items selected */
			StringCchPrintf(szItemsSelected, SIZEOF_ARRAY(szItemsSelected),
				_T("%s %s"), szNumSelected, szTemp);
		}
	}
	else
	{
		szNumSelected = PrintComma(nTotal);

		if (nTotal == 1)
		{
			LoadString(m_hLanguageModule, IDS_GENERAL_ONEITEM, szTemp,
				SIZEOF_ARRAY(szTemp));

			/* Text: '1 item' */
			StringCchPrintf(szItemsSelected, SIZEOF_ARRAY(szItemsSelected),
				_T("%s %s"), szNumSelected, szTemp);
		}
		else
		{
			LoadString(m_hLanguageModule, IDS_GENERAL_MOREITEMS, szTemp,
				SIZEOF_ARRAY(szTemp));

			/* Text: 'n Items' */
			StringCchPrintf(szItemsSelected, SIZEOF_ARRAY(szItemsSelected),
				_T("%s %s"), szNumSelected, szTemp);
		}
	}

	SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)0 | 0, (LPARAM)szItemsSelected);

	if (m_pActiveShellBrowser->InVirtualFolder())
	{
		LoadString(m_hLanguageModule, IDS_GENERAL_VIRTUALFOLDER, lpszSizeBuffer,
			SIZEOF_ARRAY(lpszSizeBuffer));
	}
	else
	{
		m_pActiveShellBrowser->GetFolderInfo(&FolderInfo);

		if ((nFilesSelected + nFoldersSelected) == 0)
		{
			/* No items(files or folders) selected. */
			FormatSizeString(FolderInfo.TotalFolderSize, lpszSizeBuffer,
				SIZEOF_ARRAY(lpszSizeBuffer), m_config->globalFolderSettings.forceSize,
				m_config->globalFolderSettings.sizeDisplayFormat);
		}
		else
		{
			if (nFilesSelected == 0)
			{
				/* Only folders selected. Don't show any size in the status bar. */
				StringCchCopy(lpszSizeBuffer, SIZEOF_ARRAY(lpszSizeBuffer), EMPTY_STRING);
			}
			else
			{
				/* Mixture of files and folders selected. Show size of currently
				selected files. */
				FormatSizeString(FolderInfo.TotalSelectionSize, lpszSizeBuffer,
					SIZEOF_ARRAY(lpszSizeBuffer), m_config->globalFolderSettings.forceSize,
					m_config->globalFolderSettings.sizeDisplayFormat);
			}
		}
	}

	SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)1 | 0, (LPARAM)lpszSizeBuffer);

	res = CreateDriveFreeSpaceString(m_CurrentDirectory, szBuffer, SIZEOF_ARRAY(szBuffer));

	if (res == -1)
		StringCchCopy(szBuffer, SIZEOF_ARRAY(szBuffer), EMPTY_STRING);

	SendMessage(m_hStatusBar, SB_SETTEXT, (WPARAM)2 | 0, (LPARAM)szBuffer);

	return S_OK;
}