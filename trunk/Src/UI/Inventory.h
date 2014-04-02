#pragma once
#ifndef __IPG_UI_INVENTORY_H__
#define __IPG_UI_INVENTORY_H__

#include <UI/Window.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>

// Inventory & equipment [screen] window. Allows user to manipulate character items

namespace Prop
{
	class CPropEquipment;
}

namespace CEGUI
{
	class PushButton;
	class Listbox;
	class EventArgs;
}

namespace UI
{
using namespace Prop;

class CInventory: public CWindow
{
protected:

	CEGUI::Listbox*		pInvList;
	CEGUI::Listbox*		pEquipList;
	CEGUI::PushButton*	pEquipBtn;
	CEGUI::PushButton*	pUnequipBtn;
	CEGUI::Window*		pWVInfo;
	//CEGUI::PushButton*	pCloseBtn;

	CPropEquipment*		pEquip;

	void Update();

	bool OnCloseClick(const CEGUI::EventArgs& e);
	bool OnEquipBtnClick(const CEGUI::EventArgs& e);
	bool OnUnequipBtnClick(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(ShowInventory, OnShow);
	DECLARE_2_EVENTS_HANDLER(OnItemAdded, OnItemRemoved, OnInvContentsChanged);

public:

	CInventory();
	//virtual ~CInventory();

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif