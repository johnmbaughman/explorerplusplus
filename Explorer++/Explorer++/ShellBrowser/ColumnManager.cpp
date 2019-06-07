// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IShellView.h"
#include "ColumnDataRetrieval.h"
#include "Columns.h"
#include "Config.h"
#include "iShellBrowser_internal.h"
#include "MainResource.h"
#include "SortModes.h"
#include "ViewModes.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include <cassert>
#include <list>

void CShellBrowser::QueueColumnTask(int itemInternalIndex, int columnIndex)
{
	auto columnID = GetColumnIdByIndex(columnIndex);

	if (!columnID)
	{
		return;
	}

	int columnResultID = m_columnResultIDCounter++;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(itemInternalIndex);
	GlobalFolderSettings globalFolderSettings = m_config->globalFolderSettings;

	auto result = m_columnThreadPool.push([this, columnResultID, columnID, itemInternalIndex, basicItemInfo, globalFolderSettings](int id) {
		UNREFERENCED_PARAMETER(id);

		return GetColumnTextAsync(m_hListView, columnResultID, *columnID, itemInternalIndex, basicItemInfo, globalFolderSettings);
	});

	// The function call above might finish before this line runs,
	// but that doesn't matter, as the results won't be processed
	// until a message posted to the main thread has been handled
	// (which can only occur after this function has returned).
	m_columnResults.insert({ columnResultID, std::move(result) });
}

CShellBrowser::ColumnResult_t CShellBrowser::GetColumnTextAsync(HWND listView, int columnResultId, unsigned int ColumnID,
	int InternalIndex, const BasicItemInfo_t &basicItemInfo, const GlobalFolderSettings &globalFolderSettings)
{
	std::wstring columnText = GetColumnText(ColumnID, basicItemInfo, globalFolderSettings);

	// This message may be delivered before this function has returned.
	// That doesn't actually matter, since the message handler will
	// simply wait for the result to be returned.
	PostMessage(listView, WM_APP_COLUMN_RESULT_READY, columnResultId, 0);

	ColumnResult_t result;
	result.itemInternalIndex = InternalIndex;
	result.columnID = ColumnID;
	result.columnText = columnText;

	return result;
}

void CShellBrowser::ProcessColumnResult(int columnResultId)
{
	auto itr = m_columnResults.find(columnResultId);

	if (itr == m_columnResults.end())
	{
		// This result is for a previous folder. It can be ignored.
		return;
	}

	if (m_folderSettings.viewMode != +ViewMode::Details)
	{
		return;
	}

	auto result = itr->second.get();

	auto index = LocateItemByInternalIndex(result.itemInternalIndex);

	if (!index)
	{
		// This is a valid state. The item may simply have been deleted.
		return;
	}

	auto columnIndex = GetColumnIndexById(result.columnID);

	if (!columnIndex)
	{
		// This is also a valid state. The column may have been removed.
		return;
	}

	auto columnText = std::make_unique<TCHAR[]>(result.columnText.size() + 1);
	StringCchCopy(columnText.get(), result.columnText.size() + 1, result.columnText.c_str());
	ListView_SetItemText(m_hListView, *index, *columnIndex, columnText.get());

	m_columnResults.erase(itr);
}

boost::optional<int> CShellBrowser::GetColumnIndexById(unsigned int id) const
{
	HWND header = ListView_GetHeader(m_hListView);

	int numItems = Header_GetItemCount(header);

	for (int i = 0; i < numItems; i++)
	{
		HDITEM hdItem;
		hdItem.mask = HDI_LPARAM;
		BOOL res = Header_GetItem(header, i, &hdItem);

		if (!res)
		{
			continue;
		}

		if (static_cast<unsigned int>(hdItem.lParam) == id)
		{
			return i;
		}
	}

	return boost::none;
}

boost::optional<unsigned int> CShellBrowser::GetColumnIdByIndex(int index) const
{
	HWND hHeader = ListView_GetHeader(m_hListView);

	HDITEM hdItem;
	hdItem.mask = HDI_LPARAM;
	BOOL res = Header_GetItem(hHeader, index, &hdItem);

	if (!res)
	{
		return boost::none;
	}

	return static_cast<unsigned int>(hdItem.lParam);
}

void CShellBrowser::PlaceColumns(void)
{
	std::list<Column_t>::iterator	itr;
	int							iColumnIndex = 0;
	int							i = 0;

	m_nActiveColumns = 0;

	if(m_pActiveColumnList != NULL)
	{
		for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
		{
			if(itr->bChecked)
			{
				InsertColumn(itr->id,iColumnIndex,itr->iWidth);

				/* Do NOT set column widths here. For some reason, this causes list mode to
				break. (If this code is active, and the listview starts of in details mode
				and is then switched to list mode, no items will be shown; they appear to
				be placed off the left edge of the listview). */
				//ListView_SetColumnWidth(m_hListView,iColumnIndex,LVSCW_AUTOSIZE_USEHEADER);

				iColumnIndex++;
				m_nActiveColumns++;
			}
		}

		for(i = m_nCurrentColumns + m_nActiveColumns;i >= m_nActiveColumns;i--)
		{
			ListView_DeleteColumn(m_hListView,i);
		}

		m_nCurrentColumns = m_nActiveColumns;
	}
}

void CShellBrowser::InsertColumn(unsigned int ColumnId,int iColumnIndex,int iWidth)
{
	HWND		hHeader;
	HDITEM		hdItem;
	LV_COLUMN	lvColumn;
	TCHAR		szText[64];
	int			iActualColumnIndex;
	int			iStringIndex;

	iStringIndex = LookupColumnNameStringIndex(ColumnId);

	LoadString(m_hResourceModule,iStringIndex,
		szText,SIZEOF_ARRAY(szText));

	lvColumn.mask		= LVCF_TEXT|LVCF_WIDTH;
	lvColumn.pszText	= szText;
	lvColumn.cx			= iWidth;

	if(ColumnId == CM_SIZE || ColumnId == CM_REALSIZE || 
		ColumnId == CM_TOTALSIZE || ColumnId == CM_FREESPACE)
	{
		lvColumn.mask	|= LVCF_FMT;
		lvColumn.fmt	= LVCFMT_RIGHT;
	}

	iActualColumnIndex = ListView_InsertColumn(m_hListView,iColumnIndex,&lvColumn);

	hHeader = ListView_GetHeader(m_hListView);

	/* Store the column's ID with the column itself. */
	hdItem.mask		= HDI_LPARAM;
	hdItem.lParam	= ColumnId;

	Header_SetItem(hHeader,iActualColumnIndex,&hdItem);
}

void CShellBrowser::SetActiveColumnSet(void)
{
	std::list<Column_t> *pActiveColumnList = NULL;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		/* Control panel. */
		pActiveColumnList = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		/* My Computer. */
		pActiveColumnList = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		/* Recycle Bin. */
		pActiveColumnList = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		/* Printers virtual folder. */
		pActiveColumnList = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		/* Network connections virtual folder. */
		pActiveColumnList = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
	{
		/* My Network Places (Network on Vista) virtual folder. */
		pActiveColumnList = &m_MyNetworkPlacesColumnList;
	}
	else
	{
		/* Real folder. */
		pActiveColumnList = &m_RealFolderColumnList;
	}

	/* If the current set of columns are different
	from the previous set of columns (i.e. the
	current folder and previous folder are of a
	different 'type'), set the new columns, and
	place them (else do nothing). */
	if(m_pActiveColumnList != pActiveColumnList)
	{
		m_pActiveColumnList = pActiveColumnList;
		m_bColumnsPlaced = FALSE;
	}
}

SortMode CShellBrowser::DetermineColumnSortMode(int iColumnId)
{
	switch(iColumnId)
	{
		case CM_NAME:
			return SortMode::Name;
			break;

		case CM_TYPE:
			return SortMode::Type;
			break;

		case CM_SIZE:
			return SortMode::Size;
			break;

		case CM_DATEMODIFIED:
			return SortMode::DateModified;
			break;

		case CM_ATTRIBUTES:
			return SortMode::Attributes;
			break;

		case CM_REALSIZE:
			return SortMode::RealSize;
			break;

		case CM_SHORTNAME:
			return SortMode::ShortName;
			break;

		case CM_OWNER:
			return SortMode::Owner;
			break;

		case CM_PRODUCTNAME:
			return SortMode::ProductName;
			break;

		case CM_COMPANY:
			return SortMode::Company;
			break;

		case CM_DESCRIPTION:
			return SortMode::Description;
			break;

		case CM_FILEVERSION:
			return SortMode::FileVersion;
			break;

		case CM_PRODUCTVERSION:
			return SortMode::ProductVersion;
			break;

		case CM_SHORTCUTTO:
			return SortMode::ShortcutTo;
			break;

		case CM_HARDLINKS:
			return SortMode::HardLinks;
			break;

		case CM_EXTENSION:
			return SortMode::Extension;
			break;

		case CM_CREATED:
			return SortMode::Created;
			break;

		case CM_ACCESSED:
			return SortMode::Accessed;
			break;

		case CM_TITLE:
			return SortMode::Title;
			break;

		case CM_SUBJECT:
			return SortMode::Subject;
			break;

		case CM_AUTHORS:
			return SortMode::Authors;
			break;

		case CM_KEYWORDS:
			return SortMode::Keywords;
			break;

		case CM_COMMENT:
			return SortMode::Comments;
			break;

		case CM_CAMERAMODEL:
			return SortMode::CameraModel;
			break;

		case CM_DATETAKEN:
			return SortMode::DateTaken;
			break;

		case CM_WIDTH:
			return SortMode::Width;
			break;

		case CM_HEIGHT:
			return SortMode::Height;
			break;

		case CM_VIRTUALCOMMENTS:
			return SortMode::VirtualComments;
			break;

		case CM_TOTALSIZE:
			return SortMode::TotalSize;
			break;

		case CM_FREESPACE:
			return SortMode::FreeSpace;
			break;

		case CM_FILESYSTEM:
			return SortMode::FileSystem;
			break;

		case CM_ORIGINALLOCATION:
			return SortMode::OriginalLocation;
			break;

		case CM_DATEDELETED:
			return SortMode::DateDeleted;
			break;

		case CM_NUMPRINTERDOCUMENTS:
			return SortMode::NumPrinterDocuments;
			break;

		case CM_PRINTERSTATUS:
			return SortMode::PrinterStatus;
			break;

		case CM_PRINTERCOMMENTS:
			return SortMode::PrinterComments;
			break;

		case CM_PRINTERLOCATION:
			return SortMode::PrinterLocation;
			break;

		case CM_NETWORKADAPTER_STATUS:
			return SortMode::NetworkAdapterStatus;
			break;

		case CM_MEDIA_BITRATE:
			return SortMode::MediaBitrate;
			break;

		case CM_MEDIA_COPYRIGHT:
			return SortMode::MediaCopyright;
			break;

		case CM_MEDIA_DURATION:
			return SortMode::MediaDuration;
			break;

		case CM_MEDIA_PROTECTED:
			return SortMode::MediaProtected;
			break;

		case CM_MEDIA_RATING:
			return SortMode::MediaRating;
			break;

		case CM_MEDIA_ALBUMARTIST:
			return SortMode::MediaAlbumArtist;
			break;

		case CM_MEDIA_ALBUM:
			return SortMode::MediaAlbum;
			break;

		case CM_MEDIA_BEATSPERMINUTE:
			return SortMode::MediaBeatsPerMinute;
			break;

		case CM_MEDIA_COMPOSER:
			return SortMode::MediaComposer;
			break;

		case CM_MEDIA_CONDUCTOR:
			return SortMode::MediaConductor;
			break;

		case CM_MEDIA_DIRECTOR:
			return SortMode::MediaDirector;
			break;

		case CM_MEDIA_GENRE:
			return SortMode::MediaGenre;
			break;

		case CM_MEDIA_LANGUAGE:
			return SortMode::MediaLanguage;
			break;

		case CM_MEDIA_BROADCASTDATE:
			return SortMode::MediaBroadcastDate;
			break;

		case CM_MEDIA_CHANNEL:
			return SortMode::MediaChannel;
			break;

		case CM_MEDIA_STATIONNAME:
			return SortMode::MediaStationName;
			break;

		case CM_MEDIA_MOOD:
			return SortMode::MediaMood;
			break;

		case CM_MEDIA_PARENTALRATING:
			return SortMode::MediaParentalRating;
			break;

		case CM_MEDIA_PARENTALRATINGREASON:
			return SortMode::MediaParentalRatingReason;
			break;

		case CM_MEDIA_PERIOD:
			return SortMode::MediaPeriod;
			break;

		case CM_MEDIA_PRODUCER:
			return SortMode::MediaProducer;
			break;

		case CM_MEDIA_PUBLISHER:
			return SortMode::MediaPublisher;
			break;

		case CM_MEDIA_WRITER:
			return SortMode::MediaWriter;
			break;

		case CM_MEDIA_YEAR:
			return SortMode::MediaYear;
			break;

		default:
			assert(false);
			break;
	}

	return SortMode::Name;
}

int CShellBrowser::LookupColumnNameStringIndex(int iColumnId)
{
	switch (iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_NAME_NAME;
		break;

	case CM_TYPE:
		return IDS_COLUMN_NAME_TYPE;
		break;

	case CM_SIZE:
		return IDS_COLUMN_NAME_SIZE;
		break;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_NAME_DATEMODIFIED;
		break;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_NAME_ATTRIBUTES;
		break;

	case CM_REALSIZE:
		return IDS_COLUMN_NAME_REALSIZE;
		break;

	case CM_SHORTNAME:
		return IDS_COLUMN_NAME_SHORTNAME;
		break;

	case CM_OWNER:
		return IDS_COLUMN_NAME_OWNER;
		break;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_NAME_PRODUCTNAME;
		break;

	case CM_COMPANY:
		return IDS_COLUMN_NAME_COMPANY;
		break;

	case CM_DESCRIPTION:
		return IDS_COLUMN_NAME_DESCRIPTION;
		break;

	case CM_FILEVERSION:
		return IDS_COLUMN_NAME_FILEVERSION;
		break;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_NAME_PRODUCTVERSION;
		break;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_NAME_SHORTCUTTO;
		break;

	case CM_HARDLINKS:
		return IDS_COLUMN_NAME_HARDLINKS;
		break;

	case CM_EXTENSION:
		return IDS_COLUMN_NAME_EXTENSION;
		break;

	case CM_CREATED:
		return IDS_COLUMN_NAME_CREATED;
		break;

	case CM_ACCESSED:
		return IDS_COLUMN_NAME_ACCESSED;
		break;

	case CM_TITLE:
		return IDS_COLUMN_NAME_TITLE;
		break;

	case CM_SUBJECT:
		return IDS_COLUMN_NAME_SUBJECT;
		break;

	case CM_AUTHORS:
		return IDS_COLUMN_NAME_AUTHORS;
		break;

	case CM_KEYWORDS:
		return IDS_COLUMN_NAME_KEYWORDS;
		break;

	case CM_COMMENT:
		return IDS_COLUMN_NAME_COMMENT;
		break;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_NAME_CAMERAMODEL;
		break;

	case CM_DATETAKEN:
		return IDS_COLUMN_NAME_DATETAKEN;
		break;

	case CM_WIDTH:
		return IDS_COLUMN_NAME_WIDTH;
		break;

	case CM_HEIGHT:
		return IDS_COLUMN_NAME_HEIGHT;
		break;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_NAME_VIRTUALCOMMENTS;
		break;

	case CM_TOTALSIZE:
		return IDS_COLUMN_NAME_TOTALSIZE;
		break;

	case CM_FREESPACE:
		return IDS_COLUMN_NAME_FREESPACE;
		break;

	case CM_FILESYSTEM:
		return IDS_COLUMN_NAME_FILESYSTEM;
		break;

	case CM_ORIGINALLOCATION:
		return IDS_COLUMN_NAME_ORIGINALLOCATION;
		break;

	case CM_DATEDELETED:
		return IDS_COLUMN_NAME_DATEDELETED;
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_NAME_NUMPRINTERDOCUMENTS;
		break;

	case CM_PRINTERSTATUS:
		return IDS_COLUMN_NAME_PRINTERSTATUS;
		break;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_NAME_PRINTERCOMMENTS;
		break;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_NAME_PRINTERLOCATION;
		break;

	case CM_PRINTERMODEL:
		return IDS_COLUMN_NAME_PRINTERMODEL;
		break;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_NAME_NETWORKADAPTER_STATUS;
		break;

	case CM_MEDIA_BITRATE:
		return IDS_COLUMN_NAME_BITRATE;
		break;

	case CM_MEDIA_COPYRIGHT:
		return IDS_COLUMN_NAME_COPYRIGHT;
		break;

	case CM_MEDIA_DURATION:
		return IDS_COLUMN_NAME_DURATION;
		break;

	case CM_MEDIA_PROTECTED:
		return IDS_COLUMN_NAME_PROTECTED;
		break;

	case CM_MEDIA_RATING:
		return IDS_COLUMN_NAME_RATING;
		break;

	case CM_MEDIA_ALBUMARTIST:
		return IDS_COLUMN_NAME_ALBUMARTIST;
		break;

	case CM_MEDIA_ALBUM:
		return IDS_COLUMN_NAME_ALBUM;
		break;

	case CM_MEDIA_BEATSPERMINUTE:
		return IDS_COLUMN_NAME_BEATSPERMINUTE;
		break;

	case CM_MEDIA_COMPOSER:
		return IDS_COLUMN_NAME_COMPOSER;
		break;

	case CM_MEDIA_CONDUCTOR:
		return IDS_COLUMN_NAME_CONDUCTOR;
		break;

	case CM_MEDIA_DIRECTOR:
		return IDS_COLUMN_NAME_DIRECTOR;
		break;

	case CM_MEDIA_GENRE:
		return IDS_COLUMN_NAME_GENRE;
		break;

	case CM_MEDIA_LANGUAGE:
		return IDS_COLUMN_NAME_LANGUAGE;
		break;

	case CM_MEDIA_BROADCASTDATE:
		return IDS_COLUMN_NAME_BROADCASTDATE;
		break;

	case CM_MEDIA_CHANNEL:
		return IDS_COLUMN_NAME_CHANNEL;
		break;

	case CM_MEDIA_STATIONNAME:
		return IDS_COLUMN_NAME_STATIONNAME;
		break;

	case CM_MEDIA_MOOD:
		return IDS_COLUMN_NAME_MOOD;
		break;

	case CM_MEDIA_PARENTALRATING:
		return IDS_COLUMN_NAME_PARENTALRATING;
		break;

	case CM_MEDIA_PARENTALRATINGREASON:
		return IDS_COLUMN_NAME_PARENTALRATINGREASON;
		break;

	case CM_MEDIA_PERIOD:
		return IDS_COLUMN_NAME_PERIOD;
		break;

	case CM_MEDIA_PRODUCER:
		return IDS_COLUMN_NAME_PRODUCER;
		break;

	case CM_MEDIA_PUBLISHER:
		return IDS_COLUMN_NAME_PUBLISHER;
		break;

	case CM_MEDIA_WRITER:
		return IDS_COLUMN_NAME_WRITER;
		break;

	case CM_MEDIA_YEAR:
		return IDS_COLUMN_NAME_YEAR;
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

int CShellBrowser::LookupColumnDescriptionStringIndex(int iColumnId)
{
	switch (iColumnId)
	{
	case CM_NAME:
		return IDS_COLUMN_DESCRIPTION_NAME;
		break;

	case CM_TYPE:
		return IDS_COLUMN_DESCRIPTION_TYPE;
		break;

	case CM_SIZE:
		return IDS_COLUMN_DESCRIPTION_SIZE;
		break;

	case CM_DATEMODIFIED:
		return IDS_COLUMN_DESCRIPTION_MODIFIED;
		break;

	case CM_ATTRIBUTES:
		return IDS_COLUMN_DESCRIPTION_ATTRIBUTES;
		break;

	case CM_REALSIZE:
		return IDS_COLUMN_DESCRIPTION_REALSIZE;
		break;

	case CM_SHORTNAME:
		return IDS_COLUMN_DESCRIPTION_SHORTNAME;
		break;

	case CM_OWNER:
		return IDS_COLUMN_DESCRIPTION_OWNER;
		break;

	case CM_PRODUCTNAME:
		return IDS_COLUMN_DESCRIPTION_PRODUCTNAME;
		break;

	case CM_COMPANY:
		return IDS_COLUMN_DESCRIPTION_COMPANY;
		break;

	case CM_DESCRIPTION:
		return IDS_COLUMN_DESCRIPTION_DESCRIPTION;
		break;

	case CM_FILEVERSION:
		return IDS_COLUMN_DESCRIPTION_FILEVERSION;
		break;

	case CM_PRODUCTVERSION:
		return IDS_COLUMN_DESCRIPTION_PRODUCTVERSION;
		break;

	case CM_SHORTCUTTO:
		return IDS_COLUMN_DESCRIPTION_SHORTCUTTO;
		break;

	case CM_HARDLINKS:
		return IDS_COLUMN_DESCRIPTION_HARDLINKS;
		break;

	case CM_EXTENSION:
		return IDS_COLUMN_DESCRIPTION_EXTENSION;
		break;

	case CM_CREATED:
		return IDS_COLUMN_DESCRIPTION_CREATED;
		break;

	case CM_ACCESSED:
		return IDS_COLUMN_DESCRIPTION_ACCESSED;
		break;

	case CM_TITLE:
		return IDS_COLUMN_DESCRIPTION_TITLE;
		break;

	case CM_SUBJECT:
		return IDS_COLUMN_DESCRIPTION_SUBJECT;
		break;

	case CM_AUTHORS:
		return IDS_COLUMN_DESCRIPTION_AUTHORS;
		break;

	case CM_KEYWORDS:
		return IDS_COLUMN_DESCRIPTION_KEYWORDS;
		break;

	case CM_COMMENT:
		return IDS_COLUMN_DESCRIPTION_COMMENT;
		break;

	case CM_CAMERAMODEL:
		return IDS_COLUMN_DESCRIPTION_CAMERAMODEL;
		break;

	case CM_DATETAKEN:
		return IDS_COLUMN_DESCRIPTION_DATETAKEN;
		break;

	case CM_WIDTH:
		return IDS_COLUMN_DESCRIPTION_WIDTH;
		break;

	case CM_HEIGHT:
		return IDS_COLUMN_DESCRIPTION_HEIGHT;
		break;

	case CM_VIRTUALCOMMENTS:
		return IDS_COLUMN_DESCRIPTION_COMMENT;
		break;

	case CM_TOTALSIZE:
		return IDS_COLUMN_DESCRIPTION_TOTALSIZE;
		break;

	case CM_FREESPACE:
		return IDS_COLUMN_DESCRIPTION_FREESPACE;
		break;

	case CM_FILESYSTEM:
		return IDS_COLUMN_DESCRIPTION_FILESYSTEM;
		break;

	case CM_NUMPRINTERDOCUMENTS:
		return IDS_COLUMN_DESCRIPTION_NUMPRINTERDOCUMENTS;
		break;

	case CM_PRINTERCOMMENTS:
		return IDS_COLUMN_DESCRIPTION_PRINTERCOMMENTS;
		break;

	case CM_PRINTERLOCATION:
		return IDS_COLUMN_DESCRIPTION_PRINTERLOCATION;
		break;

	case CM_NETWORKADAPTER_STATUS:
		return IDS_COLUMN_DESCRIPTION_NETWORKADAPTER_STATUS;
		break;

	case CM_MEDIA_BITRATE:
		return IDS_COLUMN_DESCRIPTION_BITRATE;
		break;

	default:
		assert(false);
		break;
	}

	return 0;
}

void CShellBrowser::ColumnClicked(int iClickedColumn)
{
	std::list<Column_t>::iterator itr;
	int iCurrentColumn = 0;
	SortMode sortMode = SortMode::Name;
	UINT iColumnId = 0;

	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		/* Only increment if this column is actually been shown. */
		if(itr->bChecked)
		{
			if(iCurrentColumn == iClickedColumn)
			{
				sortMode = DetermineColumnSortMode(itr->id);
				iColumnId = itr->id;
				break;
			}

			iCurrentColumn++;
		}
	}

	/* Same column was clicked. Toggle the
	ascending/descending sort state. Use unique
	column ID, not index, as columns may be
	inserted/deleted. */
	if(m_iPreviousSortedColumnId == iColumnId)
	{
		m_folderSettings.sortAscending = !m_folderSettings.sortAscending;
	}

	SortFolder(sortMode);
}

void CShellBrowser::ApplyHeaderSortArrow(void)
{
	HWND hHeader;
	HDITEM hdItem;
	std::list<Column_t>::iterator itr;
	BOOL previousColumnFound = FALSE;
	int iColumn = 0;
	int iPreviousSortedColumn = 0;
	int iColumnId = -1;

	hHeader = ListView_GetHeader(m_hListView);

	if (m_PreviousSortColumnExists)
	{
		/* Search through the currently active columns to find the column that previously
		had the up/down arrow. */
		for (itr = m_pActiveColumnList->begin(); itr != m_pActiveColumnList->end(); itr++)
		{
			/* Only increment if this column is actually been shown. */
			if (itr->bChecked)
			{
				if (m_iPreviousSortedColumnId == itr->id)
				{
					previousColumnFound = TRUE;
					break;
				}

				iPreviousSortedColumn++;
			}
		}

		if (previousColumnFound)
		{
			hdItem.mask = HDI_FORMAT;
			Header_GetItem(hHeader, iPreviousSortedColumn, &hdItem);

			if (hdItem.fmt & HDF_SORTUP)
			{
				hdItem.fmt &= ~HDF_SORTUP;
			}
			else if (hdItem.fmt & HDF_SORTDOWN)
			{
				hdItem.fmt &= ~HDF_SORTDOWN;
			}

			/* Remove the up/down arrow from the column by which
			results were previously sorted. */
			Header_SetItem(hHeader, iPreviousSortedColumn, &hdItem);
		}
	}

	/* Find the index of the column representing the current sort mode. */
	for(itr = m_pActiveColumnList->begin();itr != m_pActiveColumnList->end();itr++)
	{
		if(itr->bChecked)
		{
			if(DetermineColumnSortMode(itr->id) == m_folderSettings.sortMode)
			{
				iColumnId = itr->id;
				break;
			}

			iColumn++;
		}
	}

	hdItem.mask = HDI_FORMAT;
	Header_GetItem(hHeader,iColumn,&hdItem);

	if(!m_folderSettings.sortAscending)
		hdItem.fmt |= HDF_SORTDOWN;
	else
		hdItem.fmt |= HDF_SORTUP;

	/* Add the up/down arrow to the column by which
	items are now sorted. */
	Header_SetItem(hHeader,iColumn,&hdItem);

	m_iPreviousSortedColumnId = iColumnId;
	m_PreviousSortColumnExists = true;
}

size_t CShellBrowser::QueryNumActiveColumns(void) const
{
	return m_pActiveColumnList->size();
}

void CShellBrowser::ImportAllColumns(const ColumnExport_t *pce)
{
	m_ControlPanelColumnList = pce->ControlPanelColumnList;
	m_MyComputerColumnList = pce->MyComputerColumnList;
	m_MyNetworkPlacesColumnList = pce->MyNetworkPlacesColumnList;
	m_NetworkConnectionsColumnList = pce->NetworkConnectionsColumnList;
	m_PrintersColumnList = pce->PrintersColumnList;
	m_RealFolderColumnList = pce->RealFolderColumnList;
	m_RecycleBinColumnList = pce->RecycleBinColumnList;
}

void CShellBrowser::ExportAllColumns(ColumnExport_t *pce)
{
	SaveColumnWidths();

	pce->ControlPanelColumnList			= m_ControlPanelColumnList;
	pce->MyComputerColumnList			= m_MyComputerColumnList;
	pce->MyNetworkPlacesColumnList		= m_MyNetworkPlacesColumnList;
	pce->NetworkConnectionsColumnList	= m_NetworkConnectionsColumnList;
	pce->PrintersColumnList				= m_PrintersColumnList;
	pce->RealFolderColumnList			= m_RealFolderColumnList;
	pce->RecycleBinColumnList			= m_RecycleBinColumnList;
}

void CShellBrowser::SaveColumnWidths(void)
{
	std::list<Column_t> *pActiveColumnList = NULL;
	std::list<Column_t>::iterator itr;
	int iColumn = 0;

	if(CompareVirtualFolders(CSIDL_CONTROLS))
	{
		pActiveColumnList = &m_ControlPanelColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_DRIVES))
	{
		pActiveColumnList = &m_MyComputerColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_BITBUCKET))
	{
		pActiveColumnList = &m_RecycleBinColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_PRINTERS))
	{
		pActiveColumnList = &m_PrintersColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_CONNECTIONS))
	{
		pActiveColumnList = &m_NetworkConnectionsColumnList;
	}
	else if(CompareVirtualFolders(CSIDL_NETWORK))
	{
		pActiveColumnList = &m_MyNetworkPlacesColumnList;
	}
	else
	{
		pActiveColumnList = &m_RealFolderColumnList;
	}

	/* Only save column widths if the listview is currently in
	details view. If it's not currently in details view, then
	column widths have already been saved when the view changed. */
	if(m_folderSettings.viewMode == +ViewMode::Details)
	{
		for(itr = pActiveColumnList->begin();itr != pActiveColumnList->end();itr++)
		{
			if(itr->bChecked)
			{
				itr->iWidth = ListView_GetColumnWidth(m_hListView,iColumn);

				iColumn++;
			}
		}
	}
}