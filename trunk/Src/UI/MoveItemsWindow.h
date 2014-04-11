#pragma once
#ifndef __IPG_UI_MOVE_ITEMS_WINDOW_H__
#define __IPG_UI_MOVE_ITEMS_WINDOW_H__

#include <UI/Window.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <CEGUIEvent.h>

namespace Prop
{
	class CPropInventory;
}

namespace CEGUI
{
	class EventArgs;
	class PushButton;
	class Spinner;
}

namespace UI
{
using namespace Prop;

class CMoveItemsWindow: public CWindow
{
private:

	WORD	ItemsTotalCount;
	WORD	ItemsContainerCount;
	bool	IgnoreSpinnerValueEvent;
	bool	DialogResult;
	CStrID	ItemID;

	bool OnOwnerHide(const CEGUI::EventArgs& e);
	bool OnHide(const CEGUI::EventArgs& e);
	bool OnCloseClick(const CEGUI::EventArgs& e);
	bool OnButtonOkClick(const CEGUI::EventArgs& e);
	bool OnContainerSpinnerValueChanged(const CEGUI::EventArgs& e);
	bool OnInventorySpinnerValueChanged(const CEGUI::EventArgs& e);

protected:
	
	CEGUI::Event::Connection pConnectionOnWindowParentHide;

	CEGUI::Spinner*		pContSpn;
	CEGUI::Spinner*		pInvSpn;
	CEGUI::PushButton*	pBtnOk;
	CEGUI::Window*		pOwnerWnd;

	// Contents of container
	CPropInventory*		pContInv;
	// Contents of player's inventory
	CPropInventory*		pInv;

	DECLARE_EVENT_HANDLER(ShowMoveItemsWindow, OnShow);

public:

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif