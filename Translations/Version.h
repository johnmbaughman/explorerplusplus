// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#define MAJOR_VERSION	1
#define MINOR_VERSION	3
#define MICRO_VERSION	4
#define BUILD_VERSION	0

#define QUOTE_(x)		#x
#define QUOTE(x)		QUOTE_(x)

#define VERSION_STRING	QUOTE(MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION.BUILD_VERSION)
#define VERSION_STRING_W	_T(QUOTE(MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION.BUILD_VERSION))
