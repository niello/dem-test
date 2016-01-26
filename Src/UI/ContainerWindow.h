#pragma once
#ifndef __IPG_UI_CONTAINER_WINDOW_H__
#define __IPG_UI_CONTAINER_WINDOW_H__

#include <UI/UIWindow.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>

namespace Prop
{
	class CPropInventory;
}

namespace CEGUI
{
	class EventArgs;
	class PushButton;
	class Listbox;
}

namespace UI
{

class CContainerWindow: public CUIWindow
{
private:

	static void FillList(Prop::CPropInventory* Inventory, CEGUI::Listbox* ListBox, CEGUI::Window* pWVInfo, bool IgnoreEquippedItems);

	void ClearLists();
	void ReloadLists();
	bool MoveSelectedItem(CEGUI::Listbox* pFromListBox, Prop::CPropInventory* pFromInventory, Prop::CPropInventory* pToInventory, bool FromInventoryToContainer);

protected:

	CEGUI::PushButton*		pGiveBtn;
	CEGUI::PushButton*		pTakeBtn;
	CEGUI::Listbox*			pInvList;
	CEGUI::Listbox*			pContList;
	CEGUI::Window*			pContWVInfo;
	CEGUI::Window*			pInvWVInfo;

	Prop::CPropInventory*	pContainerInv;
	Prop::CPropInventory*	pPlrInv;

	bool OnCloseClick(const CEGUI::EventArgs& e);
	bool OnGiveBtnClick(const CEGUI::EventArgs& e);
	bool OnTakeBtnClick(const CEGUI::EventArgs& e);
	
	DECLARE_EVENT_HANDLER(ShowContainerWindow, OnShow);
	DECLARE_EVENT_HANDLER(HideContainerWindow, OnHide);
	DECLARE_EVENT_HANDLER(MoveItemsWindowClosed, OnMoveItemsWindowClosed);

public:

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif
