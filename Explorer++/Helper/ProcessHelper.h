// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

DWORD GetProcessImageName(DWORD dwProcessId, TCHAR *szImageName, DWORD nSize);
BOOL GetProcessOwner(DWORD dwProcessId, TCHAR *szOwner, size_t cchMax);
BOOL SetProcessTokenPrivilege(DWORD dwProcessId, const TCHAR *PrivilegeName, BOOL bEnablePrivilege);