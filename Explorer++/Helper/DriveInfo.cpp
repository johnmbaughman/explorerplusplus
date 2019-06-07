// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Helper.h"
#include "FileOperations.h"
#include "Macros.h"


BOOL GetClusterSize(const TCHAR *Drive, DWORD *pdwClusterSize)
{
	DWORD dwSectorsPerCluster;
	DWORD dwBytesPerSector;
	BOOL bRet = GetDiskFreeSpace(Drive,&dwSectorsPerCluster,&dwBytesPerSector,NULL,NULL);

	if(!bRet)
	{
		return FALSE;
	}

	/* It's not expected that this
	will ever actually overflow.
	The cluster size should be
	_far_ below the maximum
	DWORD value. */
	HRESULT hr = DWordMult(dwBytesPerSector, dwSectorsPerCluster, pdwClusterSize);

	if(FAILED(hr))
	{
		return FALSE;
	}

	return TRUE;
}

TCHAR GetDriveLetterFromMask(ULONG unitmask)
{
	int BitNum = 0;

	while(!(unitmask & 0x1))
	{
		unitmask >>= 1;
		BitNum++;
	}

	return (TCHAR)BitNum + 'A';
}