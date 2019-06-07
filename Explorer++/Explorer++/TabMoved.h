// Copyright (C) Explorer++ Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "Event.h"
#include "TabContainer.h"

namespace Plugins
{
	class TabMoved : public Event
	{
	public:

		TabMoved(CTabContainer *tabContainer);
		virtual ~TabMoved();

	protected:

		virtual boost::signals2::connection connectObserver(sol::protected_function observer, sol::this_state state);

	private:

		CTabContainer *m_tabContainer;
	};
}