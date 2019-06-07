// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <Windows.h>

BOOL	GetClusterSize(const TCHAR *Drive, DWORD *pdwClusterSize);
TCHAR	GetDriveLetterFromMask(ULONG unitmask);