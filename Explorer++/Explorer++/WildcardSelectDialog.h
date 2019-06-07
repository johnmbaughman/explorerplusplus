// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "CoreInterface.h"
#include "../Helper/BaseDialog.h"
#include "../Helper/DialogSettings.h"
#include "../Helper/ResizableDialog.h"
#include <MsXml2.h>
#include <objbase.h>
#include <list>
#include <string>

class CWildcardSelectDialog;

class CWildcardSelectDialogPersistentSettings : public CDialogSettings
{
public:

	~CWildcardSelectDialogPersistentSettings();

	static			CWildcardSelectDialogPersistentSettings &GetInstance();

private:

	friend			CWildcardSelectDialog;

	static const	TCHAR SETTINGS_KEY[];

	static const	TCHAR SETTING_PATTERN_LIST[];
	static const	TCHAR SETTING_CURRENT_TEXT[];

	CWildcardSelectDialogPersistentSettings();

	CWildcardSelectDialogPersistentSettings(const CWildcardSelectDialogPersistentSettings &);
	CWildcardSelectDialogPersistentSettings & operator=(const CWildcardSelectDialogPersistentSettings &);

	void			SaveExtraRegistrySettings(HKEY hKey);
	void			LoadExtraRegistrySettings(HKEY hKey);

	void			SaveExtraXMLSettings(IXMLDOMDocument *pXMLDom, IXMLDOMElement *pParentNode);
	void			LoadExtraXMLSettings(BSTR bstrName, BSTR bstrValue);

	TCHAR			m_szPattern[256];
	std::list<std::wstring>	m_PatternList;
};

class CWildcardSelectDialog : public CBaseDialog
{
public:

	CWildcardSelectDialog(HINSTANCE hInstance,int iResource,HWND hParent,BOOL bSelect,IExplorerplusplus *pexpp);
	~CWildcardSelectDialog();

protected:

	INT_PTR	OnInitDialog();
	INT_PTR	OnCommand(WPARAM wParam,LPARAM lParam);
	INT_PTR	OnClose();
	INT_PTR	OnDestroy();

private:

	void				GetResizableControlInformation(CBaseDialog::DialogSizeConstraint &dsc, std::list<CResizableDialog::Control_t> &ControlList);
	void				SaveState();

	void				OnOk();
	void				OnCancel();
	void				SelectItems(TCHAR *szPattern);

	IExplorerplusplus	*m_pexpp;
	BOOL				m_bSelect;

	HICON				m_hDialogIcon;

	CWildcardSelectDialogPersistentSettings	*m_pwsdps;
};