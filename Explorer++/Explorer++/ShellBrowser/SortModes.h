// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

enum SortMode
{
	FSM_NAME = 1,
	FSM_DATEMODIFIED = 2,
	FSM_SIZE = 3,
	FSM_TYPE = 4,
	FSM_TOTALSIZE = 5,
	FSM_FREESPACE = 6,
	FSM_COMMENTS = 7,
	FSM_DATEDELETED = 8,
	FSM_ORIGINALLOCATION = 9,
	FSM_ATTRIBUTES = 10,
	FSM_REALSIZE = 11,
	FSM_SHORTNAME = 12,
	FSM_OWNER = 13,

	FSM_PRODUCTNAME = 14,
	FSM_COMPANY = 15,
	FSM_DESCRIPTION = 16,
	FSM_FILEVERSION = 17,
	FSM_PRODUCTVERSION = 18,

	FSM_SHORTCUTTO = 19,
	FSM_HARDLINKS = 20,
	FSM_EXTENSION = 21,
	FSM_CREATED = 22,
	FSM_ACCESSED = 23,

	FSM_TITLE = 24,
	FSM_SUBJECT = 25,
	FSM_AUTHORS = 26,
	FSM_KEYWORDS = 27,

	FSM_CAMERAMODEL = 29,
	FSM_DATETAKEN = 30,
	FSM_WIDTH = 31,
	FSM_HEIGHT = 32,

	FSM_VIRTUALCOMMENTS = 33,

	FSM_FILESYSTEM = 34,

	FSM_NUMPRINTERDOCUMENTS = 36,
	FSM_PRINTERSTATUS = 37,
	FSM_PRINTERCOMMENTS = 38,
	FSM_PRINTERLOCATION = 39,

	FSM_NETWORKADAPTER_STATUS = 40,

	FSM_MEDIA_BITRATE = 41,
	FSM_MEDIA_COPYRIGHT = 42,
	FSM_MEDIA_DURATION = 43,
	FSM_MEDIA_PROTECTED = 44,
	FSM_MEDIA_RATING = 45,
	FSM_MEDIA_ALBUMARTIST = 46,
	FSM_MEDIA_ALBUM = 47,
	FSM_MEDIA_BEATSPERMINUTE = 48,
	FSM_MEDIA_COMPOSER = 49,
	FSM_MEDIA_CONDUCTOR = 50,
	FSM_MEDIA_DIRECTOR = 51,
	FSM_MEDIA_GENRE = 52,
	FSM_MEDIA_LANGUAGE = 53,
	FSM_MEDIA_BROADCASTDATE = 54,
	FSM_MEDIA_CHANNEL = 55,
	FSM_MEDIA_STATIONNAME = 56,
	FSM_MEDIA_MOOD = 57,
	FSM_MEDIA_PARENTALRATING = 58,
	FSM_MEDIA_PARENTALRATINGREASON = 59,
	FSM_MEDIA_PERIOD = 60,
	FSM_MEDIA_PRODUCER = 61,
	FSM_MEDIA_PUBLISHER = 62,
	FSM_MEDIA_WRITER = 63,
	FSM_MEDIA_YEAR = 64
};

static const SortMode RealFolderSortModes[] =
{ FSM_NAME,FSM_SIZE,FSM_TYPE,FSM_DATEMODIFIED,FSM_ATTRIBUTES,
FSM_REALSIZE,FSM_SHORTNAME,FSM_OWNER,FSM_PRODUCTNAME,
FSM_COMPANY,FSM_DESCRIPTION,FSM_FILEVERSION,FSM_PRODUCTVERSION,
FSM_SHORTCUTTO,FSM_HARDLINKS,FSM_EXTENSION,FSM_CREATED,
FSM_ACCESSED,FSM_TITLE,FSM_SUBJECT,FSM_AUTHORS,FSM_KEYWORDS,
FSM_COMMENTS,FSM_CAMERAMODEL,FSM_DATETAKEN,FSM_WIDTH,FSM_HEIGHT,
FSM_MEDIA_BITRATE,FSM_MEDIA_COPYRIGHT,FSM_MEDIA_DURATION,
FSM_MEDIA_PROTECTED,FSM_MEDIA_RATING,FSM_MEDIA_ALBUMARTIST,
FSM_MEDIA_ALBUM,FSM_MEDIA_BEATSPERMINUTE,FSM_MEDIA_COMPOSER,
FSM_MEDIA_CONDUCTOR,FSM_MEDIA_DIRECTOR,FSM_MEDIA_GENRE,
FSM_MEDIA_LANGUAGE,FSM_MEDIA_BROADCASTDATE,FSM_MEDIA_CHANNEL,
FSM_MEDIA_STATIONNAME,FSM_MEDIA_MOOD,FSM_MEDIA_PARENTALRATING,
FSM_MEDIA_PARENTALRATINGREASON,FSM_MEDIA_PERIOD,FSM_MEDIA_PRODUCER,
FSM_MEDIA_PUBLISHER,FSM_MEDIA_WRITER,FSM_MEDIA_YEAR };

static const SortMode ControlPanelSortModes[] =
{ FSM_NAME,FSM_VIRTUALCOMMENTS };

static const SortMode MyComputerSortModes[] =
{ FSM_NAME,FSM_TYPE,FSM_TOTALSIZE,FSM_FREESPACE,
FSM_VIRTUALCOMMENTS,FSM_FILESYSTEM };

static const SortMode RecycleBinSortModes[] =
{ FSM_NAME,FSM_ORIGINALLOCATION,FSM_DATEDELETED,
FSM_SIZE,FSM_TYPE,FSM_DATEMODIFIED };

static const SortMode PrintersSortModes[] =
{ FSM_NAME,FSM_NUMPRINTERDOCUMENTS,FSM_PRINTERSTATUS,
FSM_PRINTERCOMMENTS,FSM_PRINTERLOCATION };

static const SortMode NetworkConnectionsSortModes[] =
{ FSM_NAME,FSM_TYPE,FSM_NETWORKADAPTER_STATUS,
FSM_OWNER };