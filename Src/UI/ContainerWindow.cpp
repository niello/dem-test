#include "ContainerWindow.h"

#include <Events/EventServer.h>
#include <Game/EntityManager.h>
#include <Items/Prop/PropInventory.h>
#include <Items/Item.h>
#include <Items/ItemStack.h>

#include <CEGUIEvent.h>
#include <elements/CEGUIFrameWindow.h>
#include <elements/CEGUIPushButton.h>
#include <elements/CEGUIListbox.h>
#include <elements/CEGUIListboxTextItem.h>

namespace UI
{
using namespace Game;
using namespace Prop;

void CContainerWindow::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);
		
	CString WndName = pWindow->getName().c_str();
	
	pGiveBtn = (CEGUI::PushButton*)pWnd->getChild((WndName + "/GiveBtn").CStr());
	pGiveBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CContainerWindow::OnGiveBtnClick, this));

	pTakeBtn = (CEGUI::PushButton*)pWnd->getChild((WndName + "/TakeBtn").CStr());
	pTakeBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CContainerWindow::OnTakeBtnClick, this));

	pInvList = (CEGUI::Listbox*)pWnd->getChild((WndName + "/InvList").CStr());

	pContList = (CEGUI::Listbox*)pWnd->getChild((WndName + "/ContList").CStr());

	pInvWVInfo = pWnd->getChild((WndName + "/InvWVInfo").CStr());
	pContWVInfo = pWnd->getChild((WndName + "/ContWVInfo").CStr());

	pWnd->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,
		CEGUI::Event::Subscriber(&CContainerWindow::OnCloseClick, this));

	SUBSCRIBE_PEVENT(ShowContainerWindow, CContainerWindow, OnShow);
	SUBSCRIBE_PEVENT(HideContainerWindow, CContainerWindow, OnHide);
}
//---------------------------------------------------------------------

bool CContainerWindow::OnShow(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	const Data::CParam& Prm = P->Get(CStrID("SO"));
	CStrID EntID = Prm.IsA<CStrID>() ? Prm.GetValue<CStrID>() : CStrID(Prm.GetValue<CString>().CStr());
	PEntity pContEnt = EntityMgr->GetEntity(EntID);
	n_assert2(pContEnt.IsValid(), "Show container window: container not found.");

	const Data::CParam& Prm2 = P->Get(CStrID("Actor"));
	EntID = Prm2.IsA<CStrID>() ? Prm2.GetValue<CStrID>() : CStrID(Prm2.GetValue<CString>().CStr());
	PEntity pActor = EntityMgr->GetEntity(EntID);
	n_assert2(pActor, "Show container window: actor not found.");

	pContainerInv = pContEnt->GetProperty<CPropInventory>();
	pPlrInv = pActor->GetProperty<CPropInventory>();

	ReloadLists();
	Show();
	OK;
}
//---------------------------------------------------------------------

bool CContainerWindow::OnHide(const Events::CEventBase& Event)
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

void CContainerWindow::FillList(CPropInventory* pInventory, CEGUI::Listbox* pListBox,
								CEGUI::Window* pWVInfo, bool IgnoreEquippedItems)
{
	const CArray<CItemStack>& Items = pInventory->GetItems();
	
	float Volume = 0.f;
	for (int i = 0; i < Items.GetCount(); i++)
	{
		CItemStack& Stack = Items[i];
		if (IgnoreEquippedItems && !Stack.GetNotEquippedCount()) continue;
		Volume += Stack.GetTpl()->Volume * Stack.GetCount(); //???????

		WORD Count = IgnoreEquippedItems ? Stack.GetNotEquippedCount() : Stack.GetCount();

		CString Name = Stack.GetTpl()->UIName.CStr();
		if (Name.IsEmpty()) Name = Stack.GetItemID().CStr();
		if (Count > 1) Name += " (" + CString::FromInt(Count) + ")";
		CEGUI::ListboxTextItem* NewItem =
			n_new(CEGUI::ListboxTextItem((CEGUI::utf8*)Name.CStr(), 0, Stack.GetItem()));
		n_assert(NewItem);
		NewItem->setTextColours(CEGUI::colour(0xffffffff));
		NewItem->setSelectionBrushImage("TaharezLook", "MultiListSelectionBrush");
		NewItem->setSelectionColours(CEGUI::colour(0xff606099));

		pListBox->addItem(NewItem);
	}

	CString WVStr;
	WVStr.Format("Weight:%5.2f/%5.2f, Volume:%5.2f/%5.2f", pInventory->CurrWeight, pInventory->MaxWeight, Volume, pInventory->MaxVolume);
	pWVInfo->setText(WVStr.CStr());
}
//---------------------------------------------------------------------

bool CContainerWindow::MoveSelectedItem(CEGUI::Listbox* pFromListBox,
										CPropInventory* pFromInventory,
										CPropInventory* pToInventory,
										bool FromInventoryToContainer)
{
	CEGUI::ListboxItem* pMovedListItem = pFromListBox->getFirstSelectedItem();
	if (!pMovedListItem) FAIL;

	CItem* pMovedItem = (CItem*)pMovedListItem->getUserData();
	CItemStack* pMovedStack = pFromInventory->FindItemStack(pMovedItem);
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

bool CContainerWindow::OnMoveItemsWindowClosed(const Events::CEventBase& Event)
{
	UNSUBSCRIBE_EVENT(MoveItemsWindowClosed);

	Data::PParams P = ((const Events::CEvent&)Event).Params;
	if (P->Get<int>(CStrID("DialogResult"))) ReloadLists();

	OK;
}
//---------------------------------------------------------------------

}
