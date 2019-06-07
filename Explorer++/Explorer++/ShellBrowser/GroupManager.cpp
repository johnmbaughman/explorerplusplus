// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "IShellView.h"
#include "Config.h"
#include "iShellBrowser_internal.h"
#include "MainResource.h"
#include "SortModes.h"
#include "../Helper/Helper.h"
#include "../Helper/Macros.h"
#include "../Helper/ShellHelper.h"
#include "../Helper/TimeHelper.h"
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <iphlpapi.h>
#include <propkey.h>
#include <cassert>
#include <list>

namespace
{
	static const UINT KBYTE = 1024;
	static const UINT MBYTE = 1024 * 1024;
	static const UINT GBYTE = 1024 * 1024 *1024;
}

#define GROUP_BY_DATECREATED	0
#define GROUP_BY_DATEMODIFIED	1
#define GROUP_BY_DATEACCESSED	2

#define GROUP_BY_SIZE			0
#define GROUP_BY_TOTALSIZE		1

#define GROUP_OTHER				27

BOOL CShellBrowser::GetShowInGroups(void) const
{
	return m_folderSettings.showInGroups;
}

/* Simply sets the grouping flag, without actually moving
items into groups. */
void CShellBrowser::SetShowInGroupsFlag(BOOL bShowInGroups)
{
	m_folderSettings.showInGroups = bShowInGroups;
}

void CShellBrowser::SetShowInGroups(BOOL bShowInGroups)
{
	m_folderSettings.showInGroups = bShowInGroups;

	if(!m_folderSettings.showInGroups)
	{
		ListView_EnableGroupView(m_hListView,FALSE);
		SortFolder(m_folderSettings.sortMode);
		return;
	}
	else
	{
		MoveItemsIntoGroups();
	}
}

INT CALLBACK CShellBrowser::GroupNameComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData)
{
	CShellBrowser *shellBrowser = reinterpret_cast<CShellBrowser *>(pvData);
	return shellBrowser->GroupNameComparison(Group1_ID, Group2_ID);
}

INT CALLBACK CShellBrowser::GroupNameComparison(INT Group1_ID, INT Group2_ID)
{
	TCHAR *pszGroupHeader1 = NULL;
	TCHAR *pszGroupHeader2 = NULL;
	int iReturnValue;

	pszGroupHeader1 = RetrieveGroupHeader(Group1_ID);
	pszGroupHeader2 = RetrieveGroupHeader(Group2_ID);

	if (lstrcmpi(pszGroupHeader1, _T("Other")) == 0 &&
		lstrcmpi(pszGroupHeader2, _T("Other")) != 0)
	{
		iReturnValue = 1;
	}
	else if (lstrcmpi(pszGroupHeader1, _T("Other")) != 0 &&
		lstrcmpi(pszGroupHeader2, _T("Other")) == 0)
	{
		iReturnValue = -1;
	}
	else if (lstrcmpi(pszGroupHeader1, _T("Other")) == 0 &&
		lstrcmpi(pszGroupHeader2, _T("Other")) == 0)
	{
		iReturnValue = 0;
	}
	else
	{
		iReturnValue = StrCmpLogicalW(pszGroupHeader1, pszGroupHeader2);
	}

	if (!m_folderSettings.sortAscending)
	{
		iReturnValue = -iReturnValue;
	}

	return iReturnValue;
}

INT CALLBACK CShellBrowser::GroupFreeSpaceComparisonStub(INT Group1_ID, INT Group2_ID, void *pvData)
{
	CShellBrowser *shellBrowser = reinterpret_cast<CShellBrowser *>(pvData);
	return shellBrowser->GroupFreeSpaceComparison(Group1_ID, Group2_ID);
}

INT CALLBACK CShellBrowser::GroupFreeSpaceComparison(INT Group1_ID, INT Group2_ID)
{
	TCHAR *pszGroupHeader1 = NULL;
	TCHAR *pszGroupHeader2 = NULL;
	int iReturnValue;

	pszGroupHeader1 = RetrieveGroupHeader(Group1_ID);
	pszGroupHeader2 = RetrieveGroupHeader(Group2_ID);

	if (lstrcmpi(pszGroupHeader1, _T("Unspecified")) == 0 &&
		lstrcmpi(pszGroupHeader2, _T("Unspecified")) != 0)
	{
		iReturnValue = 1;
	}
	else if (lstrcmpi(pszGroupHeader1, _T("Unspecified")) != 0 &&
		lstrcmpi(pszGroupHeader2, _T("Unspecified")) == 0)
	{
		iReturnValue = -1;
	}
	else if (lstrcmpi(pszGroupHeader1, _T("Unspecified")) == 0 &&
		lstrcmpi(pszGroupHeader2, _T("Unspecified")) == 0)
	{
		iReturnValue = 0;
	}
	else
	{
		iReturnValue = StrCmpLogicalW(pszGroupHeader1, pszGroupHeader2);
	}

	if (!m_folderSettings.sortAscending)
	{
		iReturnValue = -iReturnValue;
	}

	return iReturnValue;
}

TCHAR *CShellBrowser::RetrieveGroupHeader(int iGroupId)
{
	std::list<TypeGroup_t>::iterator	itr;

	for(itr = m_GroupList.begin();itr != m_GroupList.end();itr++)
	{
		if(itr->iGroupId == iGroupId)
		{
			return itr->szHeader;
		}
	}

	return NULL;
}

/*
 * Determines the id of the group the specified
 * item belongs to.
 */
int CShellBrowser::DetermineItemGroup(int iItemInternal)
{
	PFNLVGROUPCOMPARE	pfnGroupCompare = NULL;
	TCHAR				szGroupHeader[512];
	int					iGroupId;

	BasicItemInfo_t basicItemInfo = getBasicItemInfo(iItemInternal);

	switch(m_folderSettings.sortMode)
	{
		case SortMode::Name:
			DetermineItemNameGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Type:
			DetermineItemTypeGroupVirtual(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Size:
			DetermineItemSizeGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::DateModified:
			DetermineItemDateGroup(iItemInternal,GROUP_BY_DATEMODIFIED,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::TotalSize:
			DetermineItemTotalSizeGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::FreeSpace:
			DetermineItemFreeSpaceGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupFreeSpaceComparisonStub;
			break;

		case SortMode::DateDeleted:
			break;

		case SortMode::OriginalLocation:
			DetermineItemSummaryGroup(basicItemInfo, &SCID_ORIGINAL_LOCATION, szGroupHeader, SIZEOF_ARRAY(szGroupHeader), m_config->globalFolderSettings);
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Attributes:
			DetermineItemAttributeGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::ShortName:
			DetermineItemNameGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Owner:
			DetermineItemOwnerGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::ProductName:
			DetermineItemVersionGroup(iItemInternal,_T("ProductName"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Company:
			DetermineItemVersionGroup(iItemInternal,_T("CompanyName"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Description:
			DetermineItemVersionGroup(iItemInternal,_T("FileDescription"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::FileVersion:
			DetermineItemVersionGroup(iItemInternal,_T("FileVersion"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::ProductVersion:
			DetermineItemVersionGroup(iItemInternal,_T("ProductVersion"),szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::ShortcutTo:
			break;

		case SortMode::HardLinks:
			break;

		case SortMode::Extension:
			DetermineItemExtensionGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Created:
			DetermineItemDateGroup(iItemInternal,GROUP_BY_DATECREATED,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Accessed:
			DetermineItemDateGroup(iItemInternal,GROUP_BY_DATEACCESSED,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Title:
			DetermineItemSummaryGroup(basicItemInfo,&PKEY_Title,szGroupHeader,SIZEOF_ARRAY(szGroupHeader), m_config->globalFolderSettings);
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Subject:
			DetermineItemSummaryGroup(basicItemInfo,&PKEY_Subject,szGroupHeader,SIZEOF_ARRAY(szGroupHeader), m_config->globalFolderSettings);
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Authors:
			DetermineItemSummaryGroup(basicItemInfo,&PKEY_Author,szGroupHeader,SIZEOF_ARRAY(szGroupHeader), m_config->globalFolderSettings);
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Keywords:
			DetermineItemSummaryGroup(basicItemInfo,&PKEY_Keywords,szGroupHeader,SIZEOF_ARRAY(szGroupHeader), m_config->globalFolderSettings);
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Comments:
			DetermineItemSummaryGroup(basicItemInfo,&PKEY_Comment,szGroupHeader,SIZEOF_ARRAY(szGroupHeader), m_config->globalFolderSettings);
			pfnGroupCompare = GroupNameComparisonStub;
			break;


		case SortMode::CameraModel:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagEquipModel,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::DateTaken:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagDateTime,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Width:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagImageWidth,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::Height:
			DetermineItemCameraPropertyGroup(iItemInternal,PropertyTagImageHeight,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;


		case SortMode::VirtualComments:
			break;

		case SortMode::FileSystem:
			DetermineItemFileSystemGroup(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		case SortMode::NumPrinterDocuments:
			break;

		case SortMode::PrinterStatus:
			break;

		case SortMode::PrinterComments:
			break;

		case SortMode::PrinterLocation:
			break;

		case SortMode::NetworkAdapterStatus:
			DetermineItemNetworkStatus(iItemInternal,szGroupHeader,SIZEOF_ARRAY(szGroupHeader));
			pfnGroupCompare = GroupNameComparisonStub;
			break;

		default:
			assert(false);
			break;
	}

	iGroupId = CheckGroup(szGroupHeader,pfnGroupCompare);

	return iGroupId;
}

/*
 * Checks if a group with specified id is already
 * in the listview. If not, the group is inserted
 * into its sorted position with the specified
 * header text.
 */
int CShellBrowser::CheckGroup(const TCHAR *szGroupHeader,
PFNLVGROUPCOMPARE pfnGroupCompare)
{
	LVINSERTGROUPSORTED			lvigs;
	std::list<TypeGroup_t>::iterator	itr;
	TypeGroup_t					TypeGroup;
	WCHAR						wszHeader[512];
	BOOL						bFoundGroup = FALSE;
	int							iGroupId = -1;

	/* Check to see if the group has already been inserted... */
	for(itr = m_GroupList.begin();itr != m_GroupList.end();itr++)
	{
		if(lstrcmpi(szGroupHeader,itr->szHeader) == 0)
		{
			LVGROUP lvGroup;

			bFoundGroup = TRUE;
			iGroupId = itr->iGroupId;
			itr->nItems++;

			StringCchPrintf(wszHeader, SIZEOF_ARRAY(wszHeader),
				_T("%s (%d)"), szGroupHeader, itr->nItems);

			lvGroup.cbSize = sizeof(LVGROUP);
			lvGroup.mask = LVGF_HEADER;
			lvGroup.pszHeader = wszHeader;

			ListView_SetGroupInfo(m_hListView, iGroupId, &lvGroup);

			break;
		}
	}

	if(!bFoundGroup)
	{
		iGroupId = m_iGroupId++;

		StringCchCopy(TypeGroup.szHeader,SIZEOF_ARRAY(TypeGroup.szHeader),
			szGroupHeader);
		TypeGroup.iGroupId = iGroupId;
		TypeGroup.nItems = 1;
		m_GroupList.push_back(TypeGroup);

		StringCchPrintf(wszHeader, SIZEOF_ARRAY(wszHeader),
			_T("%s (%d)"), szGroupHeader, TypeGroup.nItems);

		/* The group is not in the listview, so insert it in. */
		lvigs.lvGroup.cbSize	= sizeof(LVGROUP);
		lvigs.lvGroup.mask		= LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
		lvigs.lvGroup.state		= LVGS_COLLAPSIBLE;
		lvigs.lvGroup.pszHeader	= wszHeader;
		lvigs.lvGroup.iGroupId	= iGroupId;
		lvigs.lvGroup.stateMask	= 0;
		lvigs.pfnGroupCompare	= pfnGroupCompare;
		lvigs.pvData			= reinterpret_cast<void *>(this);

		ListView_InsertGroupSorted(m_hListView,&lvigs);
	}

	return iGroupId;
}

/*
 * Determines the id of the group to which the specified
 * item belongs, based on the item's name.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
/* TODO: These groups have changed as of Windows Visa.*/
void CShellBrowser::DetermineItemNameGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR ch;
	std::list<TypeGroup_t>::iterator itr;

	/* Take the first character of the item's name,
	and use it to determine which group it belongs to. */
	ch = m_itemInfoMap.at(iItemInternal).szDisplayName[0];

	if(iswalpha(ch))
	{
		StringCchPrintf(szGroupHeader,cchMax,
			_T("%c"),towupper(ch));
	}
	else
	{
		StringCchCopy(szGroupHeader,cchMax,
			_T("Other"));
	}
}

/*
 * Determines the id of the group to which the specified
 * item belongs, based on the item's size.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
void CShellBrowser::DetermineItemSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR *SizeGroups[] = {_T("Folders"),_T("Tiny"),_T("Small"),_T("Medium"),_T("Large"),_T("Huge")};
	int SizeGroupLimits[] = {0,0,32 * KBYTE,100 * KBYTE,MBYTE,10 * MBYTE};
	int nGroups = 6;
	int iSize;
	int i;

	if((m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	== FILE_ATTRIBUTE_DIRECTORY)
	{
		/* This item is a folder. */
		iSize = 0;
	}
	else
	{
		i = nGroups - 1;

		double FileSize = m_itemInfoMap.at(iItemInternal).wfd.nFileSizeLow +
		(m_itemInfoMap.at(iItemInternal).wfd.nFileSizeHigh * pow(2.0,32.0));

		/* Check which of the size groups this item belongs to. */
		while(FileSize < SizeGroupLimits[i]
		&& i > 0)
			i--;

		iSize = i;
	}

	StringCchCopy(szGroupHeader,cchMax,SizeGroups[iSize]);
}

/*
 * Determines the id of the group to which the specified
 * drive/folder item belongs, based on the item's total size.
 * Also returns the text header for the group when szGroupHeader
 * is non-NULL.
 */
/* TODO: These groups have changed as of Windows Vista. */
void CShellBrowser::DetermineItemTotalSizeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlDirectory	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	TCHAR *SizeGroups[] = {_T("Unspecified"),_T("Small"),_T("Medium"),_T("Huge"),_T("Gigantic")};
	TCHAR szItem[MAX_PATH];
	STRRET str;
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;
	ULARGE_INTEGER TotalSizeGroupLimits[6];
	int nGroups = 5;
	int iSize = 0;
	int i;

	TotalSizeGroupLimits[0].QuadPart	= 0;
	TotalSizeGroupLimits[1].QuadPart	= 0;
	TotalSizeGroupLimits[2].QuadPart	= GBYTE;
	TotalSizeGroupLimits[3].QuadPart	= 20 * TotalSizeGroupLimits[2].QuadPart;
	TotalSizeGroupLimits[4].QuadPart	= 100 * TotalSizeGroupLimits[2].QuadPart;

	GetIdlFromParsingName(m_CurDir,&pidlDirectory);

	SHBindToParent(m_itemInfoMap.at(iItemInternal).pidlComplete.get(), IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *) &pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

	bRoot = PathIsRoot(szItem);

	if(bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem,NULL,&nTotalBytes,&nFreeBytes);

		CoTaskMemFree(pidlDirectory);
		pShellFolder->Release();

		i = nGroups - 1;

		while(nTotalBytes.QuadPart < TotalSizeGroupLimits[i].QuadPart && i > 0)
			i--;

		iSize = i;
	}

	if(!bRoot || !bRes)
	{
		iSize = 0;
	}

	StringCchCopy(szGroupHeader,cchMax,SizeGroups[iSize]);
}

void CShellBrowser::DetermineItemTypeGroupVirtual(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	SHFILEINFO shfi;
	std::list<TypeGroup_t>::iterator itr;

	SHGetFileInfo((LPTSTR)m_itemInfoMap.at(iItemInternal).pidlComplete.get(),
		0,&shfi,sizeof(shfi),SHGFI_PIDL|SHGFI_TYPENAME);

	StringCchCopy(szGroupHeader,cchMax,shfi.szTypeName);
}

void CShellBrowser::DetermineItemDateGroup(int iItemInternal,int iDateType,TCHAR *szGroupHeader,int cchMax) const
{
	using namespace boost::gregorian;
	using namespace boost::posix_time;

	SYSTEMTIME stFileTime;
	BOOL ret = FALSE;

	switch(iDateType)
	{
	case GROUP_BY_DATEMODIFIED:
		ret = FileTimeToLocalSystemTime(&m_itemInfoMap.at(iItemInternal).wfd.ftLastWriteTime, &stFileTime);
		break;

	case GROUP_BY_DATECREATED:
		ret = FileTimeToLocalSystemTime(&m_itemInfoMap.at(iItemInternal).wfd.ftCreationTime, &stFileTime);
		break;

	case GROUP_BY_DATEACCESSED:
		ret = FileTimeToLocalSystemTime(&m_itemInfoMap.at(iItemInternal).wfd.ftLastAccessTime, &stFileTime);
		break;

	default:
		assert(false);
		return;
	}

	if (!ret)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED, szGroupHeader, cchMax);
		return;
	}

	FILETIME localFileTime;
	ret = SystemTimeToFileTime(&stFileTime, &localFileTime);

	if (!ret)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED, szGroupHeader, cchMax);
		return;
	}

	ptime filePosixTime = from_ftime<ptime>(localFileTime);
	date fileDate = filePosixTime.date();

	date today = day_clock::local_day();

	if (fileDate > today)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_FUTURE, szGroupHeader, cchMax);
		return;
	}

	if (fileDate == today)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_TODAY, szGroupHeader, cchMax);
		return;
	}

	date yesterday = today - days(1);

	if (fileDate == yesterday)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_YESTERDAY, szGroupHeader, cchMax);
		return;
	}

	// Note that this assumes that Sunday is the first day of the week.
	unsigned short currentWeekday = today.day_of_week().as_number();
	date startOfWeek = today - days(currentWeekday);

	if (fileDate >= startOfWeek)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_THIS_WEEK, szGroupHeader, cchMax);
		return;
	}

	date startOfLastWeek = startOfWeek - weeks(1);

	if (fileDate >= startOfLastWeek)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LAST_WEEK, szGroupHeader, cchMax);
		return;
	}

	date startOfMonth = date(today.year(), today.month(), 1);

	if (fileDate >= startOfMonth)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_THIS_MONTH, szGroupHeader, cchMax);
		return;
	}

	date startOfLastMonth = startOfMonth - months(1);

	if (fileDate >= startOfLastMonth)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LAST_MONTH, szGroupHeader, cchMax);
		return;
	}

	date startOfYear = date(today.year(), 1, 1);

	if (fileDate >= startOfYear)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_THIS_YEAR, szGroupHeader, cchMax);
		return;
	}

	date startOfLastYear = startOfYear - years(1);

	if (fileDate >= startOfLastYear)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LAST_YEAR, szGroupHeader, cchMax);
		return;
	}

	LoadString(m_hResourceModule, IDS_GROUPBY_DATE_LONG_AGO, szGroupHeader, cchMax);
}

void CShellBrowser::DetermineItemSummaryGroup(const BasicItemInfo_t &itemInfo, const SHCOLUMNID *pscid,
	TCHAR *szGroupHeader, size_t cchMax, const GlobalFolderSettings &globalFolderSettings) const
{
	TCHAR szDetail[512];
	HRESULT hr = GetItemDetails(itemInfo, pscid, szDetail, SIZEOF_ARRAY(szDetail), globalFolderSettings);

	if(SUCCEEDED(hr) && lstrlen(szDetail) > 0)
	{
		StringCchCopy(szGroupHeader, cchMax, szDetail);
	}
	else
	{
		StringCchCopy(szGroupHeader, cchMax, _T("Unspecified"));
	}
}

/* TODO: Need to sort based on percentage free. */
void CShellBrowser::DetermineItemFreeSpaceGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	std::list<TypeGroup_t>::iterator itr;
	LPITEMIDLIST pidlDirectory	= NULL;
	TCHAR szFreeSpace[MAX_PATH];
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	STRRET str;
	TCHAR szItem[MAX_PATH];
	ULARGE_INTEGER nTotalBytes;
	ULARGE_INTEGER nFreeBytes;
	BOOL bRoot;
	BOOL bRes = FALSE;

	GetIdlFromParsingName(m_CurDir,&pidlDirectory);
	SHBindToParent(m_itemInfoMap.at(iItemInternal).pidlComplete.get(),
		IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

	CoTaskMemFree(pidlDirectory);
	pShellFolder->Release();

	bRoot = PathIsRoot(szItem);

	if(bRoot)
	{
		bRes = GetDiskFreeSpaceEx(szItem,NULL,&nTotalBytes,&nFreeBytes);

		LARGE_INTEGER lDiv1;
		LARGE_INTEGER lDiv2;

		lDiv1.QuadPart = 100;
		lDiv2.QuadPart = 10;

		/* Divide by 10 to remove the one's digit, then multiply
		by 10 so that only the ten's digit rmains. */
		StringCchPrintf(szFreeSpace,SIZEOF_ARRAY(szFreeSpace),
			_T("%I64d%% free"),(((nFreeBytes.QuadPart * lDiv1.QuadPart) / nTotalBytes.QuadPart) / lDiv2.QuadPart) * lDiv2.QuadPart);
	}
	
	if(!bRoot || !bRes)
	{
		StringCchCopy(szFreeSpace,SIZEOF_ARRAY(szFreeSpace),_T("Unspecified"));
	}

	StringCchCopy(szGroupHeader,cchMax,szFreeSpace);
}

void CShellBrowser::DetermineItemAttributeGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR FullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szAttributes[32];

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_itemInfoMap.at(iItemInternal).wfd.cFileName);

	BuildFileAttributeString(FullFileName,szAttributes,
		SIZEOF_ARRAY(szAttributes));

	StringCchCopy(szGroupHeader,cchMax,szAttributes);
}

void CShellBrowser::DetermineItemOwnerGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR FullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szOwner[512];

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_itemInfoMap.at(iItemInternal).wfd.cFileName);

	BOOL ret = GetFileOwner(FullFileName,szOwner,SIZEOF_ARRAY(szOwner));

	if(!ret)
	{
		StringCchCopy(szOwner,SIZEOF_ARRAY(szOwner),EMPTY_STRING);
	}

	StringCchCopy(szGroupHeader,cchMax,szOwner);
}

void CShellBrowser::DetermineItemVersionGroup(int iItemInternal,TCHAR *szVersionType,TCHAR *szGroupHeader,int cchMax) const
{
	BOOL bGroupFound = FALSE;
	TCHAR FullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szVersion[512];
	BOOL bVersionInfoObtained;

	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_itemInfoMap.at(iItemInternal).wfd.cFileName);

	bVersionInfoObtained = GetVersionInfoString(FullFileName,
		szVersionType,szVersion,SIZEOF_ARRAY(szVersion));

	bGroupFound = FALSE;

	if(!bVersionInfoObtained)
		StringCchCopy(szVersion,SIZEOF_ARRAY(szVersion),_T("Unspecified"));

	StringCchCopy(szGroupHeader,cchMax,szVersion);
}

void CShellBrowser::DetermineItemCameraPropertyGroup(int iItemInternal,PROPID PropertyId,TCHAR *szGroupHeader,int cchMax) const
{
	TCHAR szFullFileName[MAX_PATH];
	std::list<TypeGroup_t>::iterator itr;
	TCHAR szProperty[512];
	BOOL bRes;

	StringCchCopy(szFullFileName,SIZEOF_ARRAY(szFullFileName),m_CurDir);
	PathAppend(szFullFileName,m_itemInfoMap.at(iItemInternal).wfd.cFileName);

	bRes = ReadImageProperty(szFullFileName,PropertyId,szProperty,
		SIZEOF_ARRAY(szProperty));

	if(!bRes)
		StringCchCopy(szProperty,SIZEOF_ARRAY(szProperty),_T("Other"));

	StringCchCopy(szGroupHeader,cchMax,szProperty);
}

void CShellBrowser::DetermineItemExtensionGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	if ((m_itemInfoMap.at(iItemInternal).wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_EXTENSION_FOLDER, szGroupHeader, cchMax);
		return;
	}

	TCHAR FullFileName[MAX_PATH];
	StringCchCopy(FullFileName,SIZEOF_ARRAY(FullFileName),m_CurDir);
	PathAppend(FullFileName,m_itemInfoMap.at(iItemInternal).wfd.cFileName);

	TCHAR *pExt = PathFindExtension(FullFileName);

	if(*pExt == '\0')
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_EXTENSION_NONE, szGroupHeader, cchMax);
	}
	else
	{
		StringCchCopy(szGroupHeader,cchMax,pExt);
	}
}

void CShellBrowser::DetermineItemFileSystemGroup(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	IShellFolder *pShellFolder	= NULL;
	LPITEMIDLIST pidlRelative	= NULL;
	TCHAR szFileSystemName[MAX_PATH];
	TCHAR szItem[MAX_PATH];
	STRRET str;
	BOOL bRoot;
	BOOL bRes;

	SHBindToParent(m_itemInfoMap.at(iItemInternal).pidlComplete.get(),
		IID_PPV_ARGS(&pShellFolder), (LPCITEMIDLIST *)&pidlRelative);

	pShellFolder->GetDisplayNameOf(pidlRelative,SHGDN_FORPARSING,&str);
	StrRetToBuf(&str,pidlRelative,szItem,SIZEOF_ARRAY(szItem));

	bRoot = PathIsRoot(szItem);

	if(bRoot)
	{
		bRes = GetVolumeInformation(szItem,NULL,0,NULL,NULL,NULL,szFileSystemName,
			SIZEOF_ARRAY(szFileSystemName));

		if(!bRes || *szFileSystemName == '\0')
		{
			LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED, szFileSystemName, SIZEOF_ARRAY(szFileSystemName));
		}
	}
	else
	{
		LoadString(m_hResourceModule, IDS_GROUPBY_UNSPECIFIED, szFileSystemName, SIZEOF_ARRAY(szFileSystemName));
	}

	StringCchCopy(szGroupHeader,cchMax,szFileSystemName);

	pShellFolder->Release();
}

/* TODO: Fix. Need to check for each adapter. */
void CShellBrowser::DetermineItemNetworkStatus(int iItemInternal,TCHAR *szGroupHeader,int cchMax) const
{
	/* When this function is
	properly implemented, this
	can be removed. */
	UNREFERENCED_PARAMETER(iItemInternal);

	std::list<TypeGroup_t>::iterator itr;

	TCHAR szStatus[32] = EMPTY_STRING;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses = NULL;
	UINT uStatusID = 0;
	ULONG ulOutBufLen = 0;

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,NULL,&ulOutBufLen);

	pAdapterAddresses = (IP_ADAPTER_ADDRESSES *)malloc(ulOutBufLen);

	GetAdaptersAddresses(AF_UNSPEC,0,NULL,pAdapterAddresses,&ulOutBufLen);

	/* TODO: These strings need to be setup correctly. */
	/*switch(pAdapterAddresses->OperStatus)
	{
		case IfOperStatusUp:
			uStatusID = IDS_NETWORKADAPTER_CONNECTED;
			break;

		case IfOperStatusDown:
			uStatusID = IDS_NETWORKADAPTER_DISCONNECTED;
			break;

		case IfOperStatusTesting:
			uStatusID = IDS_NETWORKADAPTER_TESTING;
			break;

		case IfOperStatusUnknown:
			uStatusID = IDS_NETWORKADAPTER_UNKNOWN;
			break;

		case IfOperStatusDormant:
			uStatusID = IDS_NETWORKADAPTER_DORMANT;
			break;

		case IfOperStatusNotPresent:
			uStatusID = IDS_NETWORKADAPTER_NOTPRESENT;
			break;

		case IfOperStatusLowerLayerDown:
			uStatusID = IDS_NETWORKADAPTER_LOWLAYER;
			break;
	}*/

	LoadString(m_hResourceModule,uStatusID,
		szStatus,SIZEOF_ARRAY(szStatus));

	StringCchCopy(szGroupHeader,cchMax,szStatus);
}

void CShellBrowser::InsertItemIntoGroup(int iItem,int iGroupId)
{
	LVITEM Item;

	/* Move the item into the group. */
	Item.mask		= LVIF_GROUPID;
	Item.iItem		= iItem;
	Item.iSubItem	= 0;
	Item.iGroupId	= iGroupId;
	ListView_SetItem(m_hListView,&Item);
}

void CShellBrowser::MoveItemsIntoGroups(void)
{
	LVITEM Item;
	int nItems;
	int iGroupId;
	int i = 0;

	ListView_RemoveAllGroups(m_hListView);
	ListView_EnableGroupView(m_hListView,TRUE);

	nItems = ListView_GetItemCount(m_hListView);

	SendMessage(m_hListView,WM_SETREDRAW,(WPARAM)FALSE,(LPARAM)NULL);

	m_GroupList.clear();
	m_iGroupId = 0;

	for(i = 0;i < nItems ;i++)
	{
		Item.mask		= LVIF_PARAM;
		Item.iItem		= i;
		Item.iSubItem	= 0;
		ListView_GetItem(m_hListView,&Item);

		iGroupId = DetermineItemGroup((int)Item.lParam);

		InsertItemIntoGroup(i,iGroupId);
	}

	SendMessage(m_hListView,WM_SETREDRAW,(WPARAM)TRUE,(LPARAM)NULL);
}