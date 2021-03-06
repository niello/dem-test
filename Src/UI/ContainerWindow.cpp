#include "ContainerWindow.h"

#include <Events/EventServer.h>
#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Items/Prop/PropInventory.h>
#include <Items/Item.h>
#include <Items/ItemStack.h>
#include <Data/StringUtils.h>

#include <CEGUI/Event.h>
#include <CEGUI/widgets/FrameWindow.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/Listbox.h>
#include <CEGUI/widgets/ListboxTextItem.h>

namespace UI
{

void CContainerWindow::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);
		
	CString WndName(pWindow->getName().c_str());
	
	pGiveBtn = (CEGUI::PushButton*)pWnd->getChild("GiveBtn");
	pGiveBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CContainerWindow::OnGiveBtnClick, this));

	pTakeBtn = (CEGUI::PushButton*)pWnd->getChild("TakeBtn");
	pTakeBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CContainerWindow::OnTakeBtnClick, this));

	pInvList = (CEGUI::Listbox*)pWnd->getChild("InvList");

	pContList = (CEGUI::Listbox*)pWnd->getChild("ContList");

	pInvWVInfo = pWnd->getChild("InvWVInfo");
	pContWVInfo = pWnd->getChild("ContWVInfo");

	pWnd->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,
		CEGUI::Event::Subscriber(&CContainerWindow::OnCloseClick, this));

	SUBSCRIBE_PEVENT(ShowContainerWindow, CContainerWindow, OnShow);
	SUBSCRIBE_PEVENT(HideContainerWindow, CContainerWindow, OnHide);
}
//---------------------------------------------------------------------

bool CContainerWindow::OnShow(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	const Data::CParam& Prm = P->Get(CStrID("SO"));
	CStrID EntID = Prm.IsA<CStrID>() ? Prm.GetValue<CStrID>() : CStrID(Prm.GetValue<CString>().CStr());
	Game::PEntity pContEnt = GameSrv->GetEntityMgr()->GetEntity(EntID);
	n_assert2(pContEnt.IsValidPtr(), "Show container window: container not found.");

	const Data::CParam& Prm2 = P->Get(CStrID("Actor"));
	EntID = Prm2.IsA<CStrID>() ? Prm2.GetValue<CStrID>() : CStrID(Prm2.GetValue<CString>().CStr());
	Game::PEntity pActor = GameSrv->GetEntityMgr()->GetEntity(EntID);
	n_assert2(pActor, "Show container window: actor not found.");

	pContainerInv = pContEnt->GetProperty<Prop::CPropInventory>();
	pPlrInv = pActor->GetProperty<Prop::CPropInventory>();

	ReloadLists();
	Show();
	OK;
}
//---------------------------------------------------------------------

bool CContainerWindow::OnHide(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Hide();
	OK;
}
//---------------------------------------------------------------------

bool CContainerWindow::OnCloseClick(const CEGUI::EventArgs& e)
{
	EventSrv->FireEvent(CStrID("OnContainerWindowClosed"));
	Hide();
	OK;
}
//---------------------------------------------------------------------

bool CContainerWindow::OnGiveBtnClick(const CEGUI::EventArgs& e)
{
	MoveSelectedItem(pContList, pContainerInv, pPlrInv, false);
	OK;
}
//---------------------------------------------------------------------

bool CContainerWindow::OnTakeBtnClick(const CEGUI::EventArgs& e)
{
	MoveSelectedItem(pInvList, pPlrInv, pContainerInv, true);
	OK;
}
//---------------------------------------------------------------------

void CContainerWindow::ReloadLists()
{
	ClearLists();

	if (!pContainerInv || !pPlrInv) return;

	FillList(pContainerInv, pContList, pContWVInfo, false);
	FillList(pPlrInv, pInvList, pInvWVInfo, true);
}
//---------------------------------------------------------------------

void CContainerWindow::ClearLists()
{
	pInvList->resetList();
	pContList->resetList();
}
//---------------------------------------------------------------------

void CContainerWindow::FillList(Prop::CPropInventory* pInventory, CEGUI::Listbox* pListBox,
								CEGUI::Window* pWVInfo, bool IgnoreEquippedItems)
{
	const CArray<Items::CItemStack>& Items = pInventory->GetItems();
	
	float Volume = 0.f;
	for (UPTR i = 0; i < Items.GetCount(); ++i)
	{
		Items::CItemStack& Stack = Items[i];
		if (IgnoreEquippedItems && !Stack.GetNotEquippedCount()) continue;
		Volume += Stack.GetTpl()->Volume * Stack.GetCount(); //???????

		U16 Count = IgnoreEquippedItems ? Stack.GetNotEquippedCount() : Stack.GetCount();

		CString Name(Stack.GetTpl()->UIName.CStr());
		if (Name.IsEmpty()) Name = Stack.GetItemID().CStr();
		if (Count > 1) Name += " (" + StringUtils::FromInt(Count) + ")";
		CEGUI::ListboxTextItem* NewItem =
			n_new(CEGUI::ListboxTextItem((CEGUI::utf8*)Name.CStr(), 0, Stack.GetItem()));
		n_assert(NewItem);
		NewItem->setTextColours(CEGUI::Colour(0xffffffff));
		NewItem->setSelectionBrushImage("TaharezLook/MultiListSelectionBrush");
		NewItem->setSelectionColours(CEGUI::Colour(0xff606099));

		pListBox->addItem(NewItem);
	}

	CString WVStr;
	WVStr.Format("Weight:%5.2f/%5.2f, Volume:%5.2f/%5.2f", pInventory->CurrWeight, pInventory->MaxWeight, Volume, pInventory->MaxVolume);
	pWVInfo->setText(WVStr.CStr());
}
//---------------------------------------------------------------------

bool CContainerWindow::MoveSelectedItem(CEGUI::Listbox* pFromListBox,
										Prop::CPropInventory* pFromInventory,
										Prop::CPropInventory* pToInventory,
										bool FromInventoryToContainer)
{
	CEGUI::ListboxItem* pMovedListItem = pFromListBox->getFirstSelectedItem();
	if (!pMovedListItem) FAIL;

	Items::CItem* pMovedItem = (Items::CItem*)pMovedListItem->getUserData();
	Items::CItemStack* pMovedStack = pFromInventory->FindItemStack(pMovedItem);
	if (!pMovedStack) FAIL;

	if (pMovedStack->GetNotEquippedCount() > 1)
	{
		SUBSCRIBE_PEVENT(MoveItemsWindowClosed, CContainerWindow, OnMoveItemsWindowClosed);

		Data::PParams P = n_new(Data::CParams(4));
		if (FromInventoryToContainer)
		{
			P->Set(CStrID("ContainerID"), pToInventory->GetEntity()->GetUID());
			P->Set(CStrID("InventoryID"), pFromInventory->GetEntity()->GetUID());
		}
		else
		{
			P->Set(CStrID("ContainerID"), pFromInventory->GetEntity()->GetUID());
			P->Set(CStrID("InventoryID"), pToInventory->GetEntity()->GetUID());
		}
		P->Set(CStrID("ItemID"), pMovedItem->GetID());
		P->Set(CStrID("OwnerWndName"), CString(GetWnd()->getName().c_str()));

		EventSrv->FireEvent(CStrID("ShowMoveItemsWindow"),P);
		OK;
	}

	if (!pToInventory->AddItem(pMovedItem)) FAIL;

	n_assert(pFromInventory->RemoveItem(pMovedItem) == 1);

	ReloadLists();
	OK;
}
//---------------------------------------------------------------------

bool CContainerWindow::OnMoveItemsWindowClosed(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	UNSUBSCRIBE_EVENT(MoveItemsWindowClosed);

	Data::PParams P = ((const Events::CEvent&)Event).Params;
	if (P->Get<int>(CStrID("DialogResult"))) ReloadLists();

	OK;
}
//---------------------------------------------------------------------

}
