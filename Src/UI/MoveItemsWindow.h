#pragma once
#ifndef __IPG_UI_MOVE_ITEMS_WINDOW_H__
#define __IPG_UI_MOVE_ITEMS_WINDOW_H__

#include <UI/UIWindow.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <CEGUI/Event.h>

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

class CMoveItemsWindow: public CUIWindow
{
private:

	U16		ItemsTotalCount;
	U16		ItemsContainerCount;
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

	CEGUI::Spinner*			pContSpn;
	CEGUI::Spinner*			pInvSpn;
	CEGUI::PushButton*		pBtnOk;
	CEGUI::Window*			pOwnerWnd;

	// Contents of container
	Prop::CPropInventory*	pContInv;
	// Contents of player's inventory
	Prop::CPropInventory*	pInv;

	DECLARE_EVENT_HANDLER(ShowMoveItemsWindow, OnShow);

public:

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif