// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "TimeHelper.h"


BOOL LocalSystemTimeToFileTime(const SYSTEMTIME *lpLocalTime, FILETIME *lpFileTime)
{
	SYSTEMTIME SystemTime;
	BOOL result = TzSpecificLocalTimeToSystemTime(NULL, lpLocalTime, &SystemTime);

	if(result)
	{
		result = SystemTimeToFileTime(&SystemTime, lpFileTime);
	}

	return result;
}

BOOL FileTimeToLocalSystemTime(const FILETIME *lpFileTime, SYSTEMTIME *lpLocalTime)
{
	SYSTEMTIME SystemTime;
	BOOL result = FileTimeToSystemTime(lpFileTime, &SystemTime);

	if(result)
	{
		result = SystemTimeToTzSpecificLocalTime(NULL, &SystemTime, lpLocalTime);
	}

	return result;
}

void MergeDateTime(SYSTEMTIME *pstOutput, const SYSTEMTIME *pstDate, const SYSTEMTIME *pstTime)
{
	/* Date fields. */
	pstOutput->wYear = pstDate->wYear;
	pstOutput->wMonth = pstDate->wMonth;
	pstOutput->wDayOfWeek = pstDate->wDayOfWeek;
	pstOutput->wDay = pstDate->wDay;

	/* Time fields. */
	pstOutput->wHour = pstTime->wHour;
	pstOutput->wMinute = pstTime->wMinute;
	pstOutput->wSecond = pstTime->wSecond;
	pstOutput->wMilliseconds = pstTime->wMilliseconds;
}