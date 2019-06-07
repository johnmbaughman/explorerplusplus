// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Explorer++.h"
#include "Config.h"
#include "IDropFilesCallback.h"
#include "iServiceProvider.h"
#include "ListViewEdit.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MassRenameDialog.h"
#include "MenuRanges.h"
#include "SetFileAttributesDialog.h"
#include "ShellBrowser/Columns.h"
#include "ShellBrowser/ViewModes.h"
#include "../Helper/ContextMenuManager.h"
#include "../Helper/Controls.h"
#include "../Helper/DropHandler.h"
#include "../Helper/FileActionHandler.h"
#include "../Helper/FileContextMenuManager.h"
#include "../Helper/Helper.h"
#include "../Helper/iDataObject.h"
#include "../Helper/iDropSource.h"
#include "../Helper/ListViewHelper.h"
#include "../Helper/Macros.h"
#include "../Helper/MenuHelper.h"
#include "../Helper/ShellHelper.h"

const DWORD MAIN_LISTVIEW_STYLES = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
WS_CLIPCHILDREN | LVS_ICON | LVS_EDITLABELS | LVS_SHOWSELALWAYS |
LVS_SHAREIMAGELISTS | LVS_AUTOARRANGE | WS_TABSTOP | LVS_ALIGNTOP;

static unsigned int g_RealFolderHeaderList[] =
{CM_NAME, CM_TYPE, CM_SIZE, CM_DATEMODIFIED,
CM_AUTHORS, CM_TITLE};

static unsigned int g_ControlPanelHeaderList[] =
{CM_NAME, CM_VIRTUALCOMMENTS};

static unsigned int g_MyComputerHeaderList[] =
{CM_NAME, CM_TYPE, CM_TOTALSIZE,
CM_FREESPACE, CM_VIRTUALCOMMENTS,
CM_FILESYSTEM};

static unsigned int g_NetworkConnectionsHeaderList[] =
{CM_NAME, CM_TYPE, CM_NETWORKADAPTER_STATUS,
CM_OWNER};

static unsigned int g_NetworkHeaderList[] =
{CM_NAME, CM_VIRTUALCOMMENTS};

static unsigned int g_PrintersHeaderList[] =
{CM_NAME, CM_NUMPRINTERDOCUMENTS, CM_PRINTERSTATUS,
CM_PRINTERCOMMENTS, CM_PRINTERLOCATION};

static unsigned int g_RecycleBinHeaderList[] =
{CM_NAME, CM_ORIGINALLOCATION, CM_DATEDELETED,
CM_SIZE, CM_TYPE, CM_DATEMODIFIED};

LRESULT CALLBACK ListViewProcStub(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData);

HWND Explorerplusplus::CreateMainListView(HWND hParent)
{
	HWND hListView = CreateListView(hParent, MAIN_LISTVIEW_STYLES);

	if(hListView == NULL)
	{
		return NULL;
	}

	IImageList *pImageList = NULL;
	HRESULT hr = SHGetImageList(SHIL_SMALL, IID_PPV_ARGS(&pImageList));

	if(SUCCEEDED(hr))
	{
		ListView_SetImageList(hListView, reinterpret_cast<HIMAGELIST>(pImageList), LVSIL_SMALL);
		pImageList->Release();
	}

	DWORD dwExtendedStyle = ListView_GetExtendedListViewStyle(hListView);

	if(m_config->useFullRowSelect)
	{
		dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	}

	if(m_config->checkBoxSelection)
	{
		dwExtendedStyle |= LVS_EX_CHECKBOXES;
	}

	ListView_SetExtendedListViewStyle(hListView,dwExtendedStyle);

	/* TODO: This subclass needs to be removed. */
	SetWindowSubclass(hListView, ListViewProcStub, 0, reinterpret_cast<DWORD_PTR>(this));

	SetWindowTheme(hListView,L"Explorer",NULL);

	SetListViewInitialPosition(hListView);

	return hListView;
}

LRESULT CALLBACK ListViewProcStub(HWND hwnd,UINT uMsg,
	WPARAM wParam,LPARAM lParam,UINT_PTR uIdSubclass,DWORD_PTR dwRefData)
{
	UNREFERENCED_PARAMETER(uIdSubclass);

	Explorerplusplus *pexpp = reinterpret_cast<Explorerplusplus *>(dwRefData);

	return pexpp->ListViewSubclassProc(hwnd,uMsg,wParam,lParam);
}

LRESULT CALLBACK Explorerplusplus::ListViewSubclassProc(HWND ListView,
UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_MENUSELECT:
			SendMessage(m_hContainer,WM_MENUSELECT,wParam,lParam);
			break;

		case WM_SETFOCUS:
			m_hLastActiveWindow = ListView;
			m_mainToolbar->UpdateToolbarButtonStates();
			break;

		case WM_LBUTTONDOWN:
			OnListViewLButtonDown(wParam,lParam);
			break;

		case WM_LBUTTONDBLCLK:
			{
				LV_HITTESTINFO	ht;
				DWORD			dwPos;
				POINT			MousePos;

				dwPos = GetMessagePos();
				MousePos.x = GET_X_LPARAM(dwPos);
				MousePos.y = GET_Y_LPARAM(dwPos);
				ScreenToClient(m_hActiveListView,&MousePos);

				ht.pt = MousePos;
				ListView_HitTest(ListView,&ht);

				/* NM_DBLCLK for the listview is sent both on double clicks
				(by default), as well as in the situation when LVS_EX_ONECLICKACTIVATE
				is active (in which case it is sent on a single mouse click).
				Therefore, because we only want to navigate up one folder on
				a DOUBLE click, we'll handle the event here. */
				if(ht.flags == LVHT_NOWHERE)
				{
					/* The user has double clicked in the whitespace
					area for this tab, so go up one folder... */
					OnNavigateUp();
					return 0;
				}
			}
			break;

		case WM_RBUTTONDOWN:
			if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
				&& !(wParam & MK_MBUTTON))
			{
				LV_HITTESTINFO lvhti;

				lvhti.pt.x	= LOWORD(lParam);
				lvhti.pt.y	= HIWORD(lParam);

				/* Test to see if the mouse click was
				on an item or not. */
				ListView_HitTest(m_hActiveListView,&lvhti);

				HDC hdc;
				TCHAR szText[MAX_PATH];
				RECT rc;
				SIZE sz;

				UINT uViewMode = m_pActiveShellBrowser->GetViewMode();

				if(uViewMode == ViewMode::List)
				{
					if(!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
					{
						ListView_GetItemRect(m_hActiveListView,lvhti.iItem,&rc,LVIR_LABEL);
						ListView_GetItemText(m_hActiveListView,lvhti.iItem,0,szText,SIZEOF_ARRAY(szText));

						hdc = GetDC(m_hActiveListView);
						GetTextExtentPoint32(hdc,szText,lstrlen(szText),&sz);
						ReleaseDC(m_hActiveListView,hdc);

						rc.right = rc.left + sz.cx;

						if(!PtInRect(&rc,lvhti.pt))
						{
							m_bBlockNext = TRUE;
						}
					}
				}

				if(!(lvhti.flags & LVHT_NOWHERE))
				{
					m_bDragAllowed = TRUE;
				}
			}
			break;

		case WM_RBUTTONUP:
			m_bDragCancelled = FALSE;
			m_bDragAllowed = FALSE;

			m_bBlockNext = FALSE;
			break;

		case WM_MBUTTONDOWN:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnListViewMButtonDown(&pt);
			}
			break;

		case WM_MBUTTONUP:
			{
				POINT pt;
				POINTSTOPOINT(pt, MAKEPOINTS(lParam));
				OnListViewMButtonUp(&pt);
			}
			break;

		/* If no item is currently been dragged, and the last drag
		has not just finished (i.e. item was dragged, but was cancelled
		with escape, but mouse button is still down), and when the right
		mouse button was clicked, it was over an item, start dragging. */
		case WM_MOUSEMOVE:
			{
				m_bBlockNext = FALSE;
				if(!m_bDragging && !m_bDragCancelled && m_bDragAllowed)
				{
					if((wParam & MK_RBUTTON) && !(wParam & MK_LBUTTON)
						&& !(wParam & MK_MBUTTON))
					{
						NMLISTVIEW nmlv;
						POINT pt;
						DWORD dwPos;
						HRESULT hr;

						dwPos = GetMessagePos();
						pt.x = GET_X_LPARAM(dwPos);
						pt.y = GET_Y_LPARAM(dwPos);
						MapWindowPoints(HWND_DESKTOP,m_hActiveListView,&pt,1);

						LV_HITTESTINFO lvhti;

						lvhti.pt = pt;

						/* Test to see if the mouse click was
						on an item or not. */
						ListView_HitTest(m_hActiveListView,&lvhti);

						if(!(lvhti.flags & LVHT_NOWHERE) && ListView_GetSelectedCount(m_hActiveListView) > 0)
						{
							nmlv.iItem = 0;
							nmlv.ptAction = pt;

							hr = OnListViewBeginDrag((LPARAM)&nmlv,DRAG_TYPE_RIGHTCLICK);

							if(hr == DRAGDROP_S_CANCEL)
							{
								m_bDragCancelled = TRUE;
							}
						}
					}
				}
			}
			break;

		case WM_MOUSEWHEEL:
			if(OnMouseWheel(MOUSEWHEEL_SOURCE_LISTVIEW,wParam,lParam))
			{
				return 0;
			}
			break;

		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->code)
			{
			case HDN_BEGINDRAG:
				return FALSE;
				break;

			case HDN_ENDDRAG:
				{
					/* When the drag ends, the dragged item
					is shifted into position, and all later
					items are shifted down one. Therefore,
					take the item out of its position in the
					list, and move it into its new position. */
					NMHEADER *pnmHeader = NULL;
					std::list<Column_t> ActiveColumnList;
					std::list<Column_t>::iterator itr;
					Column_t Column;
					int i = 0;

					pnmHeader = (NMHEADER *)lParam;

					m_pActiveShellBrowser->ExportCurrentColumns(&ActiveColumnList);

					i = 0;
					itr = ActiveColumnList.begin();
					while(i < (pnmHeader->iItem + 1) && itr != ActiveColumnList.end())
					{
						if(itr->bChecked)
						{
							i++;
						}

						itr++;
					}

					if(itr != ActiveColumnList.begin())
						itr--;

					Column = *itr;
					ActiveColumnList.erase(itr);

					i = 0;
					itr = ActiveColumnList.begin();
					while(i < (pnmHeader->pitem->iOrder + 1) && itr != ActiveColumnList.end())
					{
						if(itr->bChecked)
						{
							i++;
						}

						itr++;
					}

					if(itr != ActiveColumnList.begin())
						itr--;

					ActiveColumnList.insert(itr,Column);

					m_pActiveShellBrowser->ImportColumns(&ActiveColumnList);

					Tab &tab = m_tabContainer->GetSelectedTab();
					RefreshTab(tab);

					return TRUE;
				}
				break;
			}
			break;
	}

	return DefSubclassProc(ListView,msg,wParam,lParam);
}

void Explorerplusplus::OnListViewLButtonDown(WPARAM wParam,LPARAM lParam)
{
	LV_HITTESTINFO HitTestInfo;

	HitTestInfo.pt.x	= LOWORD(lParam);
	HitTestInfo.pt.y	= HIWORD(lParam);

	/* Test to see if the mouse click was
	on an item or not. */
	ListView_HitTest(m_hActiveListView,&HitTestInfo);

	/* If the mouse click was not on an item,
	then assume we are counting down the number
	of items selected. */
	if(HitTestInfo.flags == LVHT_NOWHERE)
	{
		m_bSelectionFromNowhere = TRUE;

		if(!(wParam & MK_CONTROL) && m_nSelected > 1)
			m_bCountingDown = TRUE;
	}
	else
	{
		m_bSelectionFromNowhere = FALSE;
	}
}

void Explorerplusplus::OnListViewMButtonDown(POINT *pt)
{
	LV_HITTESTINFO ht;
	ht.pt = *pt;
	ListView_HitTest(m_hActiveListView,&ht);

	if(ht.flags != LVHT_NOWHERE && ht.iItem != -1)
	{
		m_ListViewMButtonItem = ht.iItem;

		ListView_SetItemState(m_hActiveListView,ht.iItem,LVIS_FOCUSED,LVIS_FOCUSED);
	}
	else
	{
		m_ListViewMButtonItem = -1;
	}
}

void Explorerplusplus::OnListViewMButtonUp(POINT *pt)
{
	LV_HITTESTINFO	ht;
	ht.pt = *pt;
	ListView_HitTest(m_hActiveListView,&ht);

	if(ht.flags != LVHT_NOWHERE)
	{
		/* Only open an item if it was the one
		on which the middle mouse button was
		initially clicked on. */
		if(ht.iItem == m_ListViewMButtonItem)
		{
			IShellFolder *pShellFolder		= NULL;
			LPITEMIDLIST pidl				= NULL;
			LPITEMIDLIST ridl				= NULL;
			SFGAOF uAttributes				= SFGAO_FOLDER | SFGAO_STREAM;
			TCHAR szParsingPath[MAX_PATH];
			STRRET str;
			HRESULT hr;

			pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
			hr = BindToIdl(pidl, IID_PPV_ARGS(&pShellFolder));

			if(SUCCEEDED(hr))
			{
				ridl = m_pActiveShellBrowser->QueryItemRelativeIdl(ht.iItem);

				hr = pShellFolder->GetAttributesOf(1,(LPCITEMIDLIST *)&ridl,&uAttributes);

				if(SUCCEEDED(hr))
				{
					/* Is this a folder? */
					if((uAttributes & SFGAO_FOLDER) &&
						!(uAttributes & SFGAO_STREAM))
					{
						hr = pShellFolder->GetDisplayNameOf(ridl,SHGDN_FORPARSING,&str);

						if(SUCCEEDED(hr))
						{
							hr = StrRetToBuf(&str, ridl, szParsingPath, SIZEOF_ARRAY(szParsingPath));

							if(SUCCEEDED(hr))
							{
								CreateNewTab(szParsingPath);
							}
						}
					}
				}

				CoTaskMemFree(ridl);
				pShellFolder->Release();
			}

			CoTaskMemFree(pidl);
		}
	}
}

LRESULT Explorerplusplus::OnListViewKeyDown(LPARAM lParam)
{
	LV_KEYDOWN	*lv_key = NULL;

	lv_key = (LV_KEYDOWN *)lParam;

	switch(lv_key->wVKey)
	{
		case VK_RETURN:
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				/* Key press: Ctrl+Enter
				Action: Open item in background tab. */
				OpenAllSelectedItems(TRUE);
			}
			else
			{
				OpenAllSelectedItems(FALSE);
			}
			break;

		case VK_DELETE:
			if(IsKeyDown(VK_SHIFT))
			{
				OnListViewFileDelete(true);
			}
			else
			{
				OnListViewFileDelete(false);
			}
			break;

		case VK_BACK:
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				LPITEMIDLIST pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

				TCHAR szRoot[MAX_PATH];
				HRESULT hr = GetDisplayName(pidl,szRoot,SIZEOF_ARRAY(szRoot),SHGDN_FORPARSING);

				if(SUCCEEDED(hr))
				{
					BOOL bRes = PathStripToRoot(szRoot);

					if(bRes)
					{
						/* Go to the root of this directory. */
						BrowseFolderInCurrentTab(szRoot, SBSP_ABSOLUTE);
					}
				}

				CoTaskMemFree(pidl);
			}
			else
			{
				OnNavigateUp();
			}
			break;

		case 'A':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				m_bCountingUp = TRUE;
				NListView::ListView_SelectAllItems(m_hActiveListView,TRUE);
				SetFocus(m_hActiveListView);
			}
			break;

		case 'C':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				OnListViewCopy(TRUE);
			}
			break;

		case 'I':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				m_bInverted = TRUE;
				m_nSelectedOnInvert = m_nSelected;
				NListView::ListView_InvertSelection(m_hActiveListView);
				SetFocus(m_hActiveListView);
			}
			break;

		case 'V':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				OnListViewPaste();
			}
			break;

		case 'X':
			if(IsKeyDown(VK_CONTROL) &&
				!IsKeyDown(VK_SHIFT) &&
				!IsKeyDown(VK_MENU))
			{
				OnListViewCopy(FALSE);
			}
			break;
	}

	return 0;
}

void Explorerplusplus::OnListViewItemChanged(LPARAM lParam)
{
	NMLISTVIEW	*ItemChanged = NULL;
	int			iObjectIndex;
	BOOL		Selected;

	ItemChanged = (NM_LISTVIEW FAR *)lParam;

	iObjectIndex = DetermineListViewObjectIndex(ItemChanged->hdr.hwndFrom);

	if(iObjectIndex == -1)
		return;

	Tab &tab = m_tabContainer->GetTab(iObjectIndex);

	if(tab.GetShellBrowser()->QueryDragging())
		return;

	HWND listView = tab.listView;

	if(ItemChanged->uChanged == LVIF_STATE &&
		((LVIS_STATEIMAGEMASK & ItemChanged->uNewState) >> 12) != 0 &&
		((LVIS_STATEIMAGEMASK & ItemChanged->uOldState) >> 12) != 0)
	{
		if(ListView_GetCheckState(listView,ItemChanged->iItem))
		{
			NListView::ListView_SelectItem(listView,ItemChanged->iItem,TRUE);
		}
		else
		{
			NListView::ListView_SelectItem(listView,ItemChanged->iItem,FALSE);
		}

		return;
	}

	if((ItemChanged->uNewState & LVIS_SELECTED) &&
	(ItemChanged->uOldState & LVIS_SELECTED))
		return;

	/* Only proceed if an item was selected or deselected. */
	if(ItemChanged->uNewState & LVIS_SELECTED)
		Selected = TRUE;
	else if(ItemChanged->uOldState & LVIS_SELECTED)
		Selected  = FALSE;
	else
		return;

	if(Selected)
	{
		if(ListView_GetCheckState(listView,ItemChanged->iItem) == 0)
			ListView_SetCheckState(listView,ItemChanged->iItem,TRUE);
	}
	else
	{
		if(ListView_GetCheckState(listView,ItemChanged->iItem) != 0)
			ListView_SetCheckState(listView,ItemChanged->iItem,FALSE);
	}

	/* The selection for this tab has changed, so invalidate any
	folder size calculations that are occurring for this tab
	(applies only to folder sizes that will be shown in the display
	window). */
	std::list<DWFolderSize_t>::iterator itr;

	for(itr = m_DWFolderSizes.begin();itr != m_DWFolderSizes.end();itr++)
	{
		if(itr->iTabId == iObjectIndex)
		{
			itr->bValid = FALSE;
		}
	}

	/* Only update internal selection info
	if the listview that sent the change
	notification is active. */
	if(m_tabContainer->IsTabSelected(tab))
	{
		if(Selected)
		{
			m_nSelected++;

			if(m_nSelected == ListView_GetItemCount(m_hActiveListView))
				m_bCountingUp = FALSE;
		}
		else
		{
			m_nSelected--;

			if(m_nSelected <= 1)
				m_bCountingDown = FALSE;
		}
	}

	tab.GetShellBrowser()->UpdateFileSelectionInfo(
	(int)ItemChanged->lParam,Selected);

	if((ListView_GetItemCount(m_hActiveListView) - m_nSelected) == m_nSelectedOnInvert)
		m_bInverted = FALSE;

	if(m_bCountingUp || m_bCountingDown || m_bInverted)
		return;

	UpdateDisplayWindow();
	UpdateStatusBarText();
	m_mainToolbar->UpdateToolbarButtonStates();
}

int Explorerplusplus::DetermineListViewObjectIndex(HWND hListView)
{
	for (auto &item : m_tabContainer->GetAllTabs())
	{
		if (item.second.listView == hListView)
		{
			return item.first;
		}
	}

	return -1;
}

BOOL Explorerplusplus::OnListViewBeginLabelEdit(LPARAM lParam)
{
	if(!CanRename())
	{
		return TRUE;
	}

	/* Subclass the edit window. The only reason this is
	done is so that when the listview item is put into
	edit mode, any extension the file has will not be
	selected along with the rest of the text. Although
	selection works directly from here in Windows Vista,
	it does not work in Windows XP. */
	HWND hEdit = ListView_GetEditControl(m_hActiveListView);

	if(hEdit == NULL)
	{
		return TRUE;
	}

	CListViewEdit::CreateNew(hEdit,reinterpret_cast<NMLVDISPINFO *>(lParam)->item.iItem,this);

	m_bListViewRenaming = TRUE;

	return FALSE;
}

BOOL Explorerplusplus::OnListViewEndLabelEdit(LPARAM lParam)
{
	NMLVDISPINFO	*pdi = NULL;
	LVITEM			*pItem = NULL;
	TCHAR			NewFileName[MAX_PATH + 1];
	TCHAR			OldFileName[MAX_PATH + 1];
	TCHAR			CurrentDirectory[MAX_PATH];
	TCHAR			OldName[MAX_PATH];
	TCHAR			szTemp[128];
	TCHAR			szError[256];
	TCHAR			szTitle[256];
	DWORD			dwAttributes;
	int				ret;

	pdi = (NMLVDISPINFO *) lParam;
	pItem = &pdi->item;

	m_bListViewRenaming = FALSE;

	/* Did the user cancel the editing? */
	if(pItem->pszText == NULL)
		return FALSE;

	/* Is the new filename empty? */
	if(lstrcmp(pItem->pszText,EMPTY_STRING) == 0)
		return FALSE;

	/*
	Deny file names ending with a dot, as they are just
	synonyms for the same file without any dot(s).
	For example:
	C:\Hello.txt
	C:\Hello.txt....
	refer to exactly the same file.
	
	Taken from the web site referenced below:
	"Do not end a file or directory name with a trailing
	space or a period. Although the underlying file system
	may support such names, the operating system does not.
	However, it is acceptable to start a name with a period."	
	*/
	if(pItem->pszText[lstrlen(pItem->pszText) - 1] == '.')
		return FALSE;

	/*
	The following characters are NOT allowed
	within a file name:
	\/:*?"<>|

	See: http://msdn.microsoft.com/en-us/library/aa365247.aspx
	*/
	if(StrChr(pItem->pszText,'\\') != NULL ||
		StrChr(pItem->pszText,'/') != NULL ||
		StrChr(pItem->pszText,':') != NULL ||
		StrChr(pItem->pszText,'*') != NULL ||
		StrChr(pItem->pszText,'?') != NULL ||
		StrChr(pItem->pszText,'"') != NULL ||
		StrChr(pItem->pszText,'<') != NULL ||
		StrChr(pItem->pszText,'>') != NULL ||
		StrChr(pItem->pszText,'|') != NULL)
	{
		LoadString(m_hLanguageModule,IDS_ERR_FILENAMEINVALID,
			szError,SIZEOF_ARRAY(szError));
		LoadString(m_hLanguageModule,IDS_ERR_FILENAMEINVALID_MSGTITLE,
			szTitle,SIZEOF_ARRAY(szTitle));

		MessageBox(m_hContainer,szError,szTitle,MB_ICONERROR);

		return 0;
	}

	m_pActiveShellBrowser->QueryCurrentDirectory(SIZEOF_ARRAY(CurrentDirectory), CurrentDirectory);
	StringCchCopy(NewFileName,SIZEOF_ARRAY(NewFileName),CurrentDirectory);
	StringCchCopy(OldFileName,SIZEOF_ARRAY(OldFileName),CurrentDirectory);

	m_pActiveShellBrowser->QueryDisplayName(pItem->iItem,SIZEOF_ARRAY(OldName),OldName);
	PathAppend(OldFileName,OldName);

	BOOL bRes = PathAppend(NewFileName,pItem->pszText);

	if(!bRes)
	{
		return 0;
	}

	dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(pItem->iItem);

	if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
	{
		BOOL bExtensionHidden = FALSE;

		bExtensionHidden = (!m_config->globalFolderSettings.showExtensions) ||
			(m_config->globalFolderSettings.hideLinkExtension && lstrcmpi(PathFindExtension(OldName),_T(".lnk")) == 0);

		/* If file extensions are turned off, the new filename
		will be incorrect (i.e. it will be missing the extension).
		Therefore, append the extension manually if it is turned
		off. */
		if(bExtensionHidden)
		{
			TCHAR	*szExt = NULL;

			szExt = PathFindExtension(OldName);

			if(*szExt == '.')
				StringCchCat(NewFileName,SIZEOF_ARRAY(NewFileName),szExt);
		}
	}

	if (lstrcmp(OldFileName, NewFileName) == 0)
		return FALSE;

	CFileActionHandler::RenamedItem_t RenamedItem;
	RenamedItem.strOldFilename = OldFileName;
	RenamedItem.strNewFilename = NewFileName;

	TrimStringRight(RenamedItem.strNewFilename,_T(" "));

	std::list<CFileActionHandler::RenamedItem_t> RenamedItemList;
	RenamedItemList.push_back(RenamedItem);
	ret = m_FileActionHandler.RenameFiles(RenamedItemList);

	/* If the file was not renamed, show an error message. */
	if(!ret)
	{
		LoadString(m_hLanguageModule,IDS_FILERENAMEERROR,szTemp,
		SIZEOF_ARRAY(szTemp));

		MessageBox(m_hContainer,szTemp,NExplorerplusplus::APP_NAME,
			MB_ICONWARNING|MB_OK);
	}

	return ret;
}

void Explorerplusplus::OnListViewRClick(POINT *pCursorPos)
{
	/* It may be possible for the active tab/folder
	to change while the menu is been shown (e.g. if
	not handled correctly, the back/forward buttons
	on a mouse will change the directory without
	destroying the popup menu).
	If this happens, the shell browser used will not
	stay constant throughout this function, and will
	cause various problems (e.g. changing from a
	computer where new items can be created to one
	where they cannot while the popup menu is still
	active will cause the 'new' entry to stay on the
	menu incorrectly).
	Due to the possibility of unforseen problems,
	this function should NOT access the current
	shell browser once the menu has been destroyed. */

	SetForegroundWindow(m_hContainer);

	if(IsKeyDown(VK_SHIFT) &&
		!IsKeyDown(VK_CONTROL) &&
		!IsKeyDown(VK_MENU))
	{
		LVHITTESTINFO lvhti;

		lvhti.pt = *pCursorPos;
		ScreenToClient(m_hActiveListView,&lvhti.pt);
		ListView_HitTest(m_hActiveListView,&lvhti);

		if(!(lvhti.flags & LVHT_NOWHERE) && lvhti.iItem != -1)
		{
			if(ListView_GetItemState(m_hActiveListView,lvhti.iItem,LVIS_SELECTED) !=
				LVIS_SELECTED)
			{
				NListView::ListView_SelectAllItems(m_hActiveListView,FALSE);
				NListView::ListView_SelectItem(m_hActiveListView,lvhti.iItem,TRUE);
			}
		}
	}

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		OnListViewBackgroundRClick(pCursorPos);
	}
	else
	{
		OnListViewItemRClick(pCursorPos);
	}
}

void Explorerplusplus::OnListViewBackgroundRClick(POINT *pCursorPos)
{
	HMENU hMenu = InitializeRightClickMenu();
	LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	LPITEMIDLIST pidlParent = ILClone(pidlDirectory);
	ILRemoveLastID(pidlParent);

	LPCITEMIDLIST pidlChildFolder = ILFindLastID(pidlDirectory);

	IShellFolder *pShellFolder = NULL;
	HRESULT hr = BindToIdl(pidlParent, IID_PPV_ARGS(&pShellFolder));

	if(SUCCEEDED(hr))
	{
		IDataObject *pDataObject = NULL;
		hr = GetUIObjectOf(pShellFolder, NULL, 1, &pidlChildFolder, IID_PPV_ARGS(&pDataObject));

		if(SUCCEEDED(hr))
		{
			CServiceProvider ServiceProvider(this);
			CContextMenuManager cmm(CContextMenuManager::CONTEXT_MENU_TYPE_BACKGROUND, pidlDirectory,
				pDataObject, &ServiceProvider, BLACKLISTED_BACKGROUND_MENU_CLSID_ENTRIES);

			cmm.ShowMenu(m_hContainer,hMenu,IDM_FILE_COPYFOLDERPATH,MIN_SHELL_MENU_ID,
				MAX_SHELL_MENU_ID,*pCursorPos,*m_pStatusBar);

			pDataObject->Release();
		}

		pShellFolder->Release();
	}

	CoTaskMemFree(pidlParent);
	CoTaskMemFree(pidlDirectory);
	DestroyMenu(hMenu);
}

HMENU Explorerplusplus::InitializeRightClickMenu(void)
{
	HMENU hMenu = GetSubMenu(LoadMenu(m_hLanguageModule,
		MAKEINTRESOURCE(IDR_MAINMENU_RCLICK)),0);

	MENUITEMINFO mii;

	for(auto ViewMode : m_viewModes)
	{
		TCHAR szTemp[64];
		LoadString(m_hLanguageModule,GetViewModeMenuStringId(ViewMode),
			szTemp,SIZEOF_ARRAY(szTemp));

		mii.cbSize		= sizeof(mii);
		mii.fMask		= MIIM_ID|MIIM_STRING;
		mii.wID			= GetViewModeMenuId(ViewMode);
		mii.dwTypeData	= szTemp;
		InsertMenuItem(hMenu,IDM_RCLICK_VIEW_PLACEHOLDER,FALSE,&mii);
	}

	DeleteMenu(hMenu,IDM_RCLICK_VIEW_PLACEHOLDER,MF_BYCOMMAND);

	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_SUBMENU;
	mii.hSubMenu	= m_hArrangeSubMenuRClick;
	SetMenuItemInfo(hMenu,IDM_POPUP_SORTBY,FALSE,&mii);

	mii.cbSize		= sizeof(mii);
	mii.fMask		= MIIM_SUBMENU;
	mii.hSubMenu	= m_hGroupBySubMenuRClick;
	SetMenuItemInfo(hMenu,IDM_POPUP_GROUPBY,FALSE,&mii);

	UINT uViewMode = m_pActiveShellBrowser->GetViewMode();

	if(uViewMode == ViewMode::List)
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,FALSE);
	}
	else
	{
		lEnableMenuItem(hMenu,IDM_POPUP_GROUPBY,TRUE);
	}

	return hMenu;
}

void Explorerplusplus::OnListViewItemRClick(POINT *pCursorPos)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected > 0)
	{
		std::list<LPITEMIDLIST> pidlList;
		int iItem = -1;

		while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
		{
			pidlList.push_back(m_pActiveShellBrowser->QueryItemRelativeIdl(iItem));
		}

		LPITEMIDLIST pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

		CFileContextMenuManager fcmm(m_hActiveListView,pidlDirectory,
			pidlList);

		FileContextMenuInfo_t fcmi;
		fcmi.uFrom = FROM_LISTVIEW;

		CStatusBar StatusBar(m_hStatusBar);

		fcmm.ShowMenu(this,MIN_SHELL_MENU_ID,MAX_SHELL_MENU_ID,pCursorPos,&StatusBar,
			reinterpret_cast<DWORD_PTR>(&fcmi),TRUE,IsKeyDown(VK_SHIFT));

		CoTaskMemFree(pidlDirectory);

		for(auto pidl : pidlList)
		{
			CoTaskMemFree(pidl);
		}
	}
}

void Explorerplusplus::OnListViewHeaderRClick(POINT *pCursorPos)
{
	HMENU						hHeaderPopupMenu;
	HMENU						hMenu;
	MENUITEMINFO				mii;
	std::list<Column_t>			m_pActiveColumnList;
	std::list<Column_t>::iterator	itr;
	TCHAR						szColumnText[256];
	unsigned int				*pHeaderList = NULL;
	int							nItems = 0;
	int							iItem = 0;
	int							i = 0;

	hHeaderPopupMenu = LoadMenu(m_hLanguageModule,MAKEINTRESOURCE(IDR_HEADER_MENU));

	hMenu = GetSubMenu(hHeaderPopupMenu,0);

	m_pActiveShellBrowser->ExportCurrentColumns(&m_pActiveColumnList);

	nItems = GetColumnHeaderMenuList(&pHeaderList);

	for(i = 0;i < nItems;i++)
	{
		for(itr = m_pActiveColumnList.begin();itr != m_pActiveColumnList.end();itr++)
		{
			if(itr->id == pHeaderList[i])
			{
				LoadString(m_hLanguageModule,CShellBrowser::LookupColumnNameStringIndex(itr->id),
					szColumnText,SIZEOF_ARRAY(szColumnText));

				if(itr->bChecked)
					mii.fState	= MFS_CHECKED;
				else
					mii.fState	= MFS_ENABLED;

				/* Build a list of the current columns, and add them
				to the menu. */
				mii.cbSize			= sizeof(mii);
				mii.fMask			= MIIM_STRING|MIIM_STATE|MIIM_ID;
				mii.dwTypeData		= szColumnText;
				mii.wID				= MENU_HEADER_STARTID + iItem;
				InsertMenuItem(hMenu,iItem,TRUE,&mii);

				iItem++;
				break;
			}
		}
	}

	TrackPopupMenu(hMenu,TPM_LEFTALIGN|TPM_RIGHTBUTTON|TPM_VERTICAL,
		pCursorPos->x,pCursorPos->y,0,m_hContainer,NULL);

	DestroyMenu(hHeaderPopupMenu);
}

int Explorerplusplus::GetColumnHeaderMenuList(unsigned int **pHeaderList)
{
	int nItems;

	if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_DRIVES))
	{
		*pHeaderList = g_MyComputerHeaderList;
		nItems = sizeof(g_MyComputerHeaderList) / sizeof(g_MyComputerHeaderList[0]);
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_CONTROLS))
	{
		*pHeaderList = g_ControlPanelHeaderList;
		nItems = sizeof(g_ControlPanelHeaderList) / sizeof(g_ControlPanelHeaderList[0]);
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_BITBUCKET))
	{
		*pHeaderList = g_RecycleBinHeaderList;
		nItems = sizeof(g_RecycleBinHeaderList) / sizeof(g_RecycleBinHeaderList[0]);
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_CONNECTIONS))
	{
		*pHeaderList = g_NetworkConnectionsHeaderList;
		nItems = sizeof(g_NetworkConnectionsHeaderList) / sizeof(g_NetworkConnectionsHeaderList[0]);
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_NETWORK))
	{
		*pHeaderList = g_NetworkHeaderList;
		nItems = sizeof(g_NetworkHeaderList) / sizeof(g_NetworkHeaderList[0]);
	}
	else if(CompareVirtualFolders(m_CurrentDirectory, CSIDL_PRINTERS))
	{
		*pHeaderList = g_PrintersHeaderList;
		nItems = sizeof(g_PrintersHeaderList) / sizeof(g_PrintersHeaderList[0]);
	}
	else
	{
		*pHeaderList = g_RealFolderHeaderList;
		nItems = sizeof(g_RealFolderHeaderList) / sizeof(g_RealFolderHeaderList[0]);
	}

	return nItems;
}

HRESULT Explorerplusplus::OnListViewBeginDrag(LPARAM lParam,DragTypes_t DragType)
{
	IDropSource			*pDropSource = NULL;
	IDragSourceHelper	*pDragSourceHelper = NULL;
	NMLISTVIEW			*pnmlv = NULL;
	POINT				pt = {0,0};
	HRESULT				hr;
	int					iDragStartObjectIndex;

	pnmlv = reinterpret_cast<NMLISTVIEW *>(lParam);

	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return E_FAIL;
	}

	LPITEMIDLIST pidlDirectory = NULL;
	std::list<LPITEMIDLIST> ItemList;
	std::list<std::wstring> FilenameList;
	int iItem = -1;

	/* Store the pidl of the current folder, as well as the relative
	pidl's of the dragged items. */
	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		ItemList.push_back(m_pActiveShellBrowser->QueryItemRelativeIdl(iItem));

		TCHAR szFullFilename[MAX_PATH];

		m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		std::wstring stringFilename(szFullFilename);

		FilenameList.push_back(stringFilename);
	}

	hr = CoCreateInstance(CLSID_DragDropHelper,NULL,CLSCTX_ALL,
		IID_PPV_ARGS(&pDragSourceHelper));

	if(SUCCEEDED(hr))
	{
		hr = CreateDropSource(&pDropSource,DragType);

		if(SUCCEEDED(hr))
		{
			FORMATETC ftc[2];
			STGMEDIUM stg[2];

			/* We'll export two formats:
			CF_HDROP
			CFSTR_SHELLIDLIST */
			BuildHDropList(&ftc[0],&stg[0],FilenameList);
			BuildShellIDList(&ftc[1],&stg[1],pidlDirectory,ItemList);

			IDataObject *pDataObject = NULL;
			IDataObjectAsyncCapability *pAsyncCapability = NULL;
			
			hr = CreateDataObject(ftc,stg,&pDataObject,2);
			pDataObject->QueryInterface(IID_PPV_ARGS(&pAsyncCapability));

			assert(pAsyncCapability != NULL);

			/* Docs mention setting the argument to VARIANT_TRUE/VARIANT_FALSE.
			But the argument is a BOOL, so we'll go with regular TRUE/FALSE. */
			pAsyncCapability->SetAsyncMode(TRUE);

			hr = pDragSourceHelper->InitializeFromWindow(m_hActiveListView,&pt,pDataObject);

			m_pActiveShellBrowser->DragStarted(pnmlv->iItem,&pnmlv->ptAction);
			m_bDragging = TRUE;

			/* Need to remember which tab started the drag (as
			it may be different from the tab in which the drag
			finishes). */
			iDragStartObjectIndex = m_selectedTabId;

			DWORD dwEffect;

			hr = DoDragDrop(pDataObject,pDropSource,DROPEFFECT_COPY|DROPEFFECT_MOVE|
				DROPEFFECT_LINK,&dwEffect);

			m_bDragging = FALSE;

			/* The object that starts any drag may NOT be the
			object that stops it (i.e. when a file is dragged
			between tabs). Therefore, need to tell the object
			that STARTED dragging that dragging has stopped. */
			m_tabContainer->GetTab(iDragStartObjectIndex).GetShellBrowser()->DragStopped();

			BOOL bInAsyncOp;

			hr = pAsyncCapability->InOperation(&bInAsyncOp);

			pAsyncCapability->Release();
			pDataObject->Release();
			pDropSource->Release();
		}

		pDragSourceHelper->Release();
	}

	for(auto pidl : ItemList)
	{
		CoTaskMemFree(pidl);
	}

	CoTaskMemFree(pidlDirectory);

	return hr;
}

void Explorerplusplus::OnListViewFileDelete(bool permanent)
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected == 0)
	{
		return;
	}

	// PIDLPointer is analogous to a unique_ptr. This vector exists only
	// so that the underlying PIDLs will be freed on scope exit (i.e.
	// when this function returns).
	std::vector<PIDLPointer> pidlPtrs;

	std::vector<LPCITEMIDLIST> pidls;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		PIDLPointer pidlPtr(m_pActiveShellBrowser->QueryItemCompleteIdl(iItem));

		if (!pidlPtr)
		{
			continue;
		}

		pidls.push_back(pidlPtr.get());
		pidlPtrs.push_back(std::move(pidlPtr));
	}

	if (pidls.empty())
	{
		return;
	}

	m_FileActionHandler.DeleteFiles(m_hContainer,pidls,permanent,false);
}

void Explorerplusplus::OnListViewDoubleClick(NMHDR *nmhdr)
{
	if(nmhdr->hwndFrom == m_hActiveListView)
	{
		LV_HITTESTINFO	ht;
		DWORD			dwPos;
		POINT			MousePos;

		dwPos = GetMessagePos();
		MousePos.x = GET_X_LPARAM(dwPos);
		MousePos.y = GET_Y_LPARAM(dwPos);
		ScreenToClient(m_hActiveListView,&MousePos);

		ht.pt = MousePos;
		ListView_HitTest(m_hActiveListView,&ht);

		if(ht.flags != LVHT_NOWHERE && ht.iItem != -1)
		{
			if(IsKeyDown(VK_MENU))
			{
				LPITEMIDLIST pidlDirectory = NULL;
				LPITEMIDLIST pidl = NULL;

				pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
				pidl = m_pActiveShellBrowser->QueryItemRelativeIdl(ht.iItem);

				ShowMultipleFileProperties(pidlDirectory, (LPCITEMIDLIST *)&pidl, m_hContainer, 1);

				CoTaskMemFree(pidl);
				CoTaskMemFree(pidlDirectory);
			}
			else if(IsKeyDown(VK_CONTROL))
			{
				/* Open the item in a new tab. */
				OpenListViewItem(ht.iItem,TRUE,FALSE);
			}
			else if(IsKeyDown(VK_SHIFT))
			{
				/* Open the item in a new window. */
				OpenListViewItem(ht.iItem,FALSE,TRUE);
			}
			else
			{
				OpenListViewItem(ht.iItem,FALSE,FALSE);
			}
		}
	}
}

void Explorerplusplus::OnListViewFileRename()
{
	int nSelected = ListView_GetSelectedCount(m_hActiveListView);

	/* If there is only item selected, start editing
	it in-place. If multiple items are selected,
	show the mass rename dialog. */
	if(nSelected == 1)
	{
		OnListViewFileRenameSingle();
	}
	else if(nSelected > 1)
	{
		OnListViewFileRenameMultiple();
	}
}

void Explorerplusplus::OnListViewFileRenameSingle()
{
	int iSelected = ListView_GetNextItem(m_hActiveListView,
		-1, LVNI_SELECTED | LVNI_FOCUSED);

	if (iSelected == -1)
	{
		return;
	}

	BOOL canRename = TestListViewItemAttributes(iSelected, SFGAO_CANRENAME);

	if (!canRename)
	{
		return;
	}

	ListView_EditLabel(m_hActiveListView, iSelected);
}

void Explorerplusplus::OnListViewFileRenameMultiple()
{
	std::list<std::wstring>	FullFilenameList;
	TCHAR szFullFilename[MAX_PATH];
	int iIndex = -1;

	while ((iIndex = ListView_GetNextItem(m_hActiveListView,
		iIndex, LVNI_SELECTED)) != -1)
	{
		BOOL canRename = TestListViewItemAttributes(iIndex, SFGAO_CANRENAME);

		if (!canRename)
		{
			continue;
		}

		m_pActiveShellBrowser->QueryFullItemName(iIndex, szFullFilename, SIZEOF_ARRAY(szFullFilename));
		FullFilenameList.push_back(szFullFilename);
	}

	if (FullFilenameList.empty())
	{
		return;
	}

	CMassRenameDialog CMassRenameDialog(m_hLanguageModule, IDD_MASSRENAME,
		m_hContainer, FullFilenameList, &m_FileActionHandler);
	CMassRenameDialog.ShowModalDialog();
}

void Explorerplusplus::OnListViewShowFileProperties(void) const
{
	LPITEMIDLIST	*ppidl = NULL;
	LPITEMIDLIST	pidlDirectory = NULL;
	int				iItem;
	int				nSelected;
	int				i = 0;

	nSelected = ListView_GetSelectedCount(m_hActiveListView);

	if(nSelected == 0)
	{
		ppidl = NULL;
	}
	else
	{
		ppidl = (LPITEMIDLIST *)malloc(nSelected * sizeof(LPCITEMIDLIST));

		iItem = -1;

		while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
		{
			ppidl[i] = m_pActiveShellBrowser->QueryItemRelativeIdl(iItem);

			i++;
		}
	}

	pidlDirectory = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
	ShowMultipleFileProperties(pidlDirectory, (LPCITEMIDLIST *)ppidl, m_hContainer, nSelected);
	CoTaskMemFree(pidlDirectory);

	for(i = 0;i < nSelected;i++)
	{
		CoTaskMemFree(ppidl[i]);
	}

	free(ppidl);
}

void Explorerplusplus::OnListViewCopyItemPath(void) const
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strItemPaths;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		strItemPaths += szFullFilename + std::wstring(_T("\r\n"));
	}

	strItemPaths = strItemPaths.substr(0,strItemPaths.size() - 2);

	CopyTextToClipboard(strItemPaths);
}

void Explorerplusplus::OnListViewCopyUniversalPaths(void) const
{
	if(ListView_GetSelectedCount(m_hActiveListView) == 0)
	{
		return;
	}

	std::wstring strUniversalPaths;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFilename[MAX_PATH];
		m_pActiveShellBrowser->QueryFullItemName(iItem,szFullFilename,SIZEOF_ARRAY(szFullFilename));

		TCHAR szBuffer[1024];

		DWORD dwBufferSize = SIZEOF_ARRAY(szBuffer);
		UNIVERSAL_NAME_INFO *puni = reinterpret_cast<UNIVERSAL_NAME_INFO *>(&szBuffer);
		DWORD dwRet = WNetGetUniversalName(szFullFilename,UNIVERSAL_NAME_INFO_LEVEL,
			reinterpret_cast<LPVOID>(puni),&dwBufferSize);

		if(dwRet == NO_ERROR)
		{
			strUniversalPaths += puni->lpUniversalName + std::wstring(_T("\r\n"));
		}
		else
		{
			strUniversalPaths += szFullFilename + std::wstring(_T("\r\n"));
		}
	}

	strUniversalPaths = strUniversalPaths.substr(0,strUniversalPaths.size() - 2);

	CopyTextToClipboard(strUniversalPaths);
}

HRESULT Explorerplusplus::OnListViewCopy(BOOL bCopy)
{
	IDataObject		*pClipboardDataObject = NULL;
	int				iItem = -1;
	HRESULT			hr;

	if(!CanCopy())
		return E_FAIL;

	SetCursor(LoadCursor(NULL,IDC_WAIT));

	std::list<std::wstring> FileNameList;

	BuildListViewFileSelectionList(m_hActiveListView,&FileNameList);

	if(bCopy)
	{
		hr = CopyFiles(FileNameList,&pClipboardDataObject);

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
		}
	}
	else
	{
		hr = CutFiles(FileNameList,&pClipboardDataObject);

		if(SUCCEEDED(hr))
		{
			m_pClipboardDataObject = pClipboardDataObject;
			m_iCutTabInternal = m_selectedTabId;

			TCHAR szFilename[MAX_PATH];

			/* 'Ghost' each of the cut items. */
			while((iItem = ListView_GetNextItem(m_hActiveListView,
				iItem,LVNI_SELECTED)) != -1)
			{
				m_pActiveShellBrowser->QueryDisplayName(iItem,SIZEOF_ARRAY(szFilename),
					szFilename);
				m_CutFileNameList.push_back(szFilename);

				m_pActiveShellBrowser->GhostItem(iItem);
			}
		}
	}

	SetCursor(LoadCursor(NULL,IDC_ARROW));

	return hr;
}

void Explorerplusplus::OnListViewSetFileAttributes(void) const
{
	if(ListView_GetSelectedCount(m_hActiveListView) > 0)
	{
		int iSel = -1;

		std::list<NSetFileAttributesDialogExternal::SetFileAttributesInfo_t> sfaiList;

		while((iSel = ListView_GetNextItem(m_hActiveListView,
			iSel,LVNI_SELECTED)) != -1)
		{
			NSetFileAttributesDialogExternal::SetFileAttributesInfo_t sfai;

			m_pActiveShellBrowser->QueryFullItemName(iSel,sfai.szFullFileName,SIZEOF_ARRAY(sfai.szFullFileName));

			WIN32_FIND_DATA wfd = m_pActiveShellBrowser->QueryFileFindData(iSel);
			sfai.wfd = wfd;

			sfaiList.push_back(sfai);
		}

		CSetFileAttributesDialog SetFileAttributesDialog(m_hLanguageModule,
			IDD_SETFILEATTRIBUTES,m_hContainer,sfaiList);

		SetFileAttributesDialog.ShowModalDialog();
	}
}

void Explorerplusplus::OnListViewPaste(void)
{
	IDataObject *pClipboardObject = NULL;
	HRESULT hr;

	hr = OleGetClipboard(&pClipboardObject);

	if(hr == S_OK)
	{
		TCHAR szDestination[MAX_PATH + 1];

		/* DO NOT use the internal current directory string.
		Files are copied asynchronously, so a change of directory
		will cause the destination directory to change in the
		middle of the copy operation. */
		StringCchCopy(szDestination,SIZEOF_ARRAY(szDestination),
			m_CurrentDirectory);

		/* Also, the string must be double NULL terminated. */
		szDestination[lstrlen(szDestination) + 1] = '\0';

		CDropHandler *pDropHandler = CDropHandler::CreateNew();
		CDropFilesCallback *DropFilesCallback = new CDropFilesCallback(this);
		pDropHandler->CopyClipboardData(pClipboardObject,m_hContainer,szDestination,
			DropFilesCallback,!m_config->overwriteExistingFilesConfirmation);
		pDropHandler->Release();

		pClipboardObject->Release();
	}
}

void Explorerplusplus::BuildListViewFileSelectionList(HWND hListView,
	std::list<std::wstring> *pFileSelectionList)
{
	if(pFileSelectionList == NULL)
	{
		return;
	}

	std::list<std::wstring> FileSelectionList;
	int iItem = -1;

	while((iItem = ListView_GetNextItem(hListView,
		iItem,LVNI_SELECTED)) != -1)
	{
		TCHAR szFullFileName[MAX_PATH];

		m_pActiveShellBrowser->QueryFullItemName(iItem,
			szFullFileName,SIZEOF_ARRAY(szFullFileName));

		std::wstring stringFileName(szFullFileName);
		FileSelectionList.push_back(stringFileName);
	}

	pFileSelectionList->assign(FileSelectionList.begin(),
		FileSelectionList.end());
}

int Explorerplusplus::HighlightSimilarFiles(HWND ListView) const
{
	TCHAR	FullFileName[MAX_PATH];
	TCHAR	TestFile[MAX_PATH];
	HRESULT	hr;
	BOOL	bSimilarTypes;
	int		iSelected;
	int		nItems;
	int		nSimilar = 0;
	int		i = 0;

	iSelected = ListView_GetNextItem(ListView,
	-1,LVNI_SELECTED);

	if(iSelected == -1)
		return -1;

	hr = m_pActiveShellBrowser->QueryFullItemName(iSelected,TestFile,SIZEOF_ARRAY(TestFile));

	if(SUCCEEDED(hr))
	{
		nItems = ListView_GetItemCount(ListView);

		for(i = 0;i < nItems;i++)
		{
			m_pActiveShellBrowser->QueryFullItemName(i,FullFileName,SIZEOF_ARRAY(FullFileName));

			bSimilarTypes = CompareFileTypes(FullFileName,TestFile);

			if(bSimilarTypes)
			{
				NListView::ListView_SelectItem(ListView,i,TRUE);
				nSimilar++;
			}
			else
			{
				NListView::ListView_SelectItem(ListView,i,FALSE);
			}
		}
	}

	return nSimilar;
}

void Explorerplusplus::OpenAllSelectedItems(BOOL bOpenInNewTab)
{
	BOOL	m_bSeenDirectory = FALSE;
	DWORD	dwAttributes;
	int		iItem = -1;
	int		iFolderItem = -1;

	while((iItem = ListView_GetNextItem(m_hActiveListView,iItem,LVIS_SELECTED)) != -1)
	{
		dwAttributes = m_pActiveShellBrowser->QueryFileAttributes(iItem);

		if((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
		{
			m_bSeenDirectory = TRUE;
			iFolderItem = iItem;
		}
		else
		{
			OpenListViewItem(iItem,FALSE,FALSE);
		}
	}

	if(m_bSeenDirectory)
		OpenListViewItem(iFolderItem,bOpenInNewTab,FALSE);
}

void Explorerplusplus::OpenListViewItem(int iItem, BOOL bOpenInNewTab, BOOL bOpenInNewWindow)
{
	LPITEMIDLIST	pidlComplete = NULL;
	LPITEMIDLIST	pidl = NULL;
	LPITEMIDLIST	ridl = NULL;

	pidl = m_pActiveShellBrowser->QueryCurrentDirectoryIdl();
	ridl = m_pActiveShellBrowser->QueryItemRelativeIdl(iItem);

	if(ridl != NULL)
	{
		pidlComplete = ILCombine(pidl, ridl);

		OpenItem(pidlComplete, bOpenInNewTab, bOpenInNewWindow);

		CoTaskMemFree(pidlComplete);
		CoTaskMemFree(ridl);
	}

	CoTaskMemFree(pidl);
}