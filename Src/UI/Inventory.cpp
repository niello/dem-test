#include "Inventory.h"

#include <Items/ItemStack.h>
#include <UI/UIServer.h>
#include <Events/EventServer.h>
#include <Game/EntityManager.h>
#include <Items/Prop/PropEquipment.h>
#include <Data/StringUtils.h>

#include <CEGUI/Event.h>
#include <CEGUI/widgets/FrameWindow.h>
#include <CEGUI/widgets/Listbox.h>
#include <CEGUI/widgets/ListboxTextItem.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{

CInventory::CInventory()
{
}
//---------------------------------------------------------------------

void CInventory::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);
	
	pWnd->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,
		CEGUI::Event::Subscriber(&CInventory::OnCloseClick, this));

	CString WndName(pWindow->getName().c_str());

	pInvList = (CEGUI::Listbox*)pWnd->getChild("InvList");
	pInvList->setShowVertScrollbar(true);
	pInvList->setMultiselectEnabled(false);

	pEquipList = (CEGUI::Listbox*)pWnd->getChild("EquipList");
	pEquipList->setShowVertScrollbar(true);
	pEquipList->setMultiselectEnabled(false);

	pEquipBtn = (CEGUI::PushButton*)pWnd->getChild("EquipBtn");
	pEquipBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CInventory::OnEquipBtnClick, this));

	pUnequipBtn = (CEGUI::PushButton*)pWnd->getChild("UnequipBtn");
	pUnequipBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CInventory::OnUnequipBtnClick, this));

	pWVInfo = pWnd->getChild("WVInfo");

	//!!!not here - bug below!
	//ConnKeyUp = pWnd->subscribeEvent(CEGUI::Window::EventKeyUp,
	//	CEGUI::Event::Subscriber(&CDialogueWindow::OnKeyUp, this));
	//ConnTextAreaMM = pTextArea->subscribeEvent(CEGUI::Listbox::EventMouseMove,
	//	CEGUI::Event::Subscriber(&CDialogueWindow::OnTextAreaMouseMove, this));
	//ConnAnswerClicked = pTextArea->subscribeEvent(CEGUI::Listbox::EventMouseClick,
	//	CEGUI::Event::Subscriber(&CDialogueWindow::OnAnswerClicked, this));

	//!!!need to unsubscribe somewhere!
	SUBSCRIBE_PEVENT(ShowInventory, CInventory, OnShow);
	SUBSCRIBE_PEVENT(OnItemAdded, CInventory, OnInvContentsChanged);
	SUBSCRIBE_PEVENT(OnItemRemoved, CInventory, OnInvContentsChanged);
}
//---------------------------------------------------------------------

void CInventory::Update()
{
	pInvList->resetList();
	const CArray<Items::CItemStack>& InvItems = pEquip->GetItems();
	for (UPTR i = 0; i < InvItems.GetCount(); ++i)
	{
		Items::CItemStack& Stack = InvItems[i];
		
		if (!Stack.GetNotEquippedCount()) continue;

		CString Name(Stack.GetTpl()->UIName.CStr());
		if (Name.IsEmpty()) Name = Stack.GetItemID().CStr();
		if (Stack.GetNotEquippedCount() > 1)
			Name += " (" + StringUtils::FromInt(Stack.GetNotEquippedCount()) + ")";
		CEGUI::ListboxTextItem* NewItem =
			n_new(CEGUI::ListboxTextItem((CEGUI::utf8*)Name.CStr(), 0, &Stack));
		n_assert(NewItem);
		NewItem->setTextColours(CEGUI::Colour(0xffffffff));
		NewItem->setSelectionBrushImage("TaharezLook/MultiListSelectionBrush");
		NewItem->setSelectionColours(CEGUI::Colour(0xff606099));
		pInvList->addItem(NewItem);
	}

	pEquipList->resetList();
	for (UPTR i = 0; i < pEquip->Slots.GetCount(); ++i)
	{
		CString Text(pEquip->Slots.KeyAt(i).CStr());
		Text += ": ";
		Items::CItemStack* pStack = pEquip->Slots.ValueAt(i).pStack;
		if (pStack)
		{
			const char* Name = pStack->GetTpl()->UIName.CStr();
			Text += (Name) ? Name : pStack->GetItemID().CStr();
		}
		CEGUI::ListboxTextItem* NewItem =
			n_new(CEGUI::ListboxTextItem((CEGUI::utf8*)Text.CStr(), 0, (void*)((int)pEquip->Slots.KeyAt(i))));
		n_assert(NewItem);
		NewItem->setTextColours(CEGUI::Colour(0xffffffff));
		NewItem->setSelectionBrushImage("TaharezLook/MultiListSelectionBrush");
		NewItem->setSelectionColours(CEGUI::Colour(0xff606099));
		pEquipList->addItem(NewItem);
	}

	CString WV;
	WV.Format("Weight:%5.2f/%5.2f, Volume:%5.2f/%5.2f",
		pEquip->CurrWeight, pEquip->MaxWeight, pEquip->CurrVolume, pEquip->MaxVolume);
	pWVInfo->setText(WV.CStr());
}
//---------------------------------------------------------------------

bool CInventory::OnCloseClick(const CEGUI::EventArgs& e)
{
	Hide();
	OK;
}
//---------------------------------------------------------------------

bool CInventory::OnEquipBtnClick(const CEGUI::EventArgs& e)
{
	CEGUI::ListboxItem* pInvItem = pInvList->getFirstSelectedItem();
	CEGUI::ListboxItem* pEquipItem = pEquipList->getFirstSelectedItem();
	if (pInvItem && pEquipItem && pEquip->Equip(CStrID(pEquipItem->getUserData()), (Items::CItemStack*)pInvItem->getUserData()))
		Update();
	OK;
}
//---------------------------------------------------------------------

bool CInventory::OnUnequipBtnClick(const CEGUI::EventArgs& e)
{
	CEGUI::ListboxItem* pEquipItem = pEquipList->getFirstSelectedItem();
	if (pEquipItem)
	{
		pEquip->Unequip(CStrID(pEquipItem->getUserData()));
		Update(); //???bool Unequip for performance reasons?
	}
	OK;
}
//---------------------------------------------------------------------

bool CInventory::OnShow(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Events::CEvent& e = (const Events::CEvent&)Event;

	Game::PEntity pOwnerEnt = EntityMgr->GetEntity(e.Params->Get<CStrID>(CStrID("EntityID")));
	
	if (pOwnerEnt.IsNullPtr())
	{
		Hide();
		OK;
	}
	
	pWnd->setText((CString("Inventory - ") + pOwnerEnt->GetUID().CStr()).CStr());

	//!!!???param bool Show or always show by this event & hide by close key now?

	pEquip = pOwnerEnt->GetProperty<Prop::CPropEquipment>();
	n_assert(pEquip);

	Update();
	Show();

	OK;
}
//---------------------------------------------------------------------

//???!!!subscribe OnShow, unsubscribe OnHide?
bool CInventory::OnInvContentsChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (IsVisible() && pEquip &&
		((const Events::CEvent&)Event).Params->Get<CString>(CStrID("Entity")) == pEquip->GetEntity()->GetUID().CStr())
		Update();
	OK;
}
//---------------------------------------------------------------------

}
