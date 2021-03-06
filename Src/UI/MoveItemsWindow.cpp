#include "MoveItemsWindow.h"

#include <Events/EventServer.h>
#include <Game/GameServer.h>
#include <Game/Entity.h>
#include <Items/Prop/PropInventory.h>
#include <Items/Item.h>
#include <UI/UIServer.h>

#include <CEGUI/Event.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/FrameWindow.h>
#include <CEGUI/widgets/PushButton.h>
#include <CEGUI/widgets/Spinner.h>

namespace UI
{

void CMoveItemsWindow::Init(CEGUI::Window* pWindow)
{
	pContInv = pInv = NULL;
	pOwnerWnd = NULL;
	pConnectionOnWindowParentHide = NULL;
	IgnoreSpinnerValueEvent = false;
	DialogResult = false;

	CUIWindow::Init(pWindow);

	CString WndName(pWindow->getName().c_str());

	pContSpn = (CEGUI::Spinner*)pWnd->getChild("ContainerSpinner");
	pInvSpn = (CEGUI::Spinner*)pWnd->getChild("InventorySpinner");
	pBtnOk = (CEGUI::PushButton*)pWnd->getChild("OkButton");

	pContSpn->setMinimumValue(0);
	pInvSpn->setMinimumValue(0);

	pContSpn->subscribeEvent(CEGUI::Spinner::EventValueChanged,
		CEGUI::Event::Subscriber(&CMoveItemsWindow::OnContainerSpinnerValueChanged, this));
	pInvSpn->subscribeEvent(CEGUI::Spinner::EventValueChanged,
		CEGUI::Event::Subscriber(&CMoveItemsWindow::OnInventorySpinnerValueChanged, this));

	pWnd->subscribeEvent(CEGUI::FrameWindow::EventHidden,
		CEGUI::Event::Subscriber(&CMoveItemsWindow::OnHide, this));
	pWnd->subscribeEvent(CEGUI::FrameWindow::EventCloseClicked,
		CEGUI::Event::Subscriber(&CMoveItemsWindow::OnCloseClick, this));

	pBtnOk->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CMoveItemsWindow::OnButtonOkClick, this));

	SUBSCRIBE_PEVENT(ShowMoveItemsWindow, CMoveItemsWindow, OnShow);
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnShow(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	DialogResult = false;

	Data::PParams P = ((const Events::CEvent&)Event).Params;

	Game::PEntity pContEnt = GameSrv->GetEntityMgr()->GetEntity(P->Get<CStrID>(CStrID("ContainerID")));
	n_assert2(pContEnt.IsValidPtr(), "Show container window: container not found.");

	Game::PEntity pActor = GameSrv->GetEntityMgr()->GetEntity(P->Get<CStrID>(CStrID("InventoryID")));
	n_assert2(pActor, "Show container window: actor not found.");
	
	ItemID = P->Get<CStrID>(CStrID("ItemID"));

	const CString& WindowOwnerID = P->Get<CString>(CStrID("OwnerWndName"), CString::Empty);

	if (WindowOwnerID.IsValid())
	{
		//!!!TEST IT!
		n_assert(false);
		pOwnerWnd = CEGUI::System::getSingleton().getDefaultGUIContext().getRootWindow()->getChild(CEGUI::String(WindowOwnerID.CStr()));
		n_assert(pOwnerWnd);
		pOwnerWnd->setEnabled(false);
		pConnectionOnWindowParentHide = pOwnerWnd->subscribeEvent(CEGUI::FrameWindow::EventHidden,
			CEGUI::Event::Subscriber(&CMoveItemsWindow::OnOwnerHide, this));
	}

	pInv = pActor->GetProperty<Prop::CPropInventory>(),
	pContInv = pContEnt->GetProperty<Prop::CPropInventory>();

	U16 ItemsInventoryCount = 0;
	ItemsContainerCount = 0;

	Items::CItemStack* pItemStack = pInv->FindItemStack(ItemID);
	if (pItemStack) ItemsInventoryCount = pItemStack->GetNotEquippedCount();

	pItemStack = pContInv->FindItemStack(ItemID);
	if (pItemStack) ItemsContainerCount = pItemStack->GetNotEquippedCount();

	ItemsTotalCount = ItemsContainerCount + ItemsInventoryCount;

	pContSpn->setMaximumValue(ItemsTotalCount);
	pInvSpn->setMaximumValue(ItemsTotalCount);
	
	pContSpn->setCurrentValue(ItemsContainerCount);
	pInvSpn->setCurrentValue(ItemsInventoryCount);

	Show();
	GetWnd()->activate();

	OK;
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnHide(const CEGUI::EventArgs& Event)
{
	if (pOwnerWnd)
	{
		UISrv->DelayedDisconnect(pConnectionOnWindowParentHide);
		pOwnerWnd->setEnabled(true);
		pConnectionOnWindowParentHide = NULL;
		pOwnerWnd = NULL;
	}

	Data::PParams P = n_new(Data::CParams(1));
	P->Set(CStrID("DialogResult"), (int)DialogResult);
	EventSrv->FireEvent(CStrID("MoveItemsWindowClosed"), P);

	OK;
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnOwnerHide(const CEGUI::EventArgs& e)
{
	Hide();
	OK;
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnCloseClick( const CEGUI::EventArgs& e )
{
	Hide();
	OK;
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnButtonOkClick(const CEGUI::EventArgs& e)
{
	n_assert(pContInv);
	n_assert(pInv);

	U16 ContainerCount = (U16)pContSpn->getCurrentValue();

	if (ContainerCount != ItemsContainerCount)
	{
		short ContainerDelta = ContainerCount - ItemsContainerCount;
		if (ContainerDelta > 0)
		{
			if (pContInv->AddItem(ItemID, ContainerDelta))
			{
				pInv->RemoveItem(ItemID, ContainerDelta);
				DialogResult = true;
			}
		}
		else if (ContainerDelta < 0)
		{
			if (pInv->AddItem(ItemID, -ContainerDelta))
			{
				pContInv->RemoveItem(ItemID, -ContainerDelta);
				DialogResult = true;
			}
		}
	}
	
	pContInv = NULL;
	pInv = NULL;
	Hide();
	OK;
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnContainerSpinnerValueChanged(const CEGUI::EventArgs& e)
{
	if (IgnoreSpinnerValueEvent) OK;

	U16 Value = (U16)pContSpn->getCurrentValue();
	IgnoreSpinnerValueEvent = true;
	pInvSpn->setCurrentValue(ItemsTotalCount - Value);
	IgnoreSpinnerValueEvent = false;
	
	OK;
}
//---------------------------------------------------------------------

bool CMoveItemsWindow::OnInventorySpinnerValueChanged(const CEGUI::EventArgs& e)
{
	if (IgnoreSpinnerValueEvent) OK;

	U16 Value = (U16)pInvSpn->getCurrentValue();
	IgnoreSpinnerValueEvent = true;
	pContSpn->setCurrentValue(ItemsTotalCount - Value);
	IgnoreSpinnerValueEvent = false;
	
	OK;
}
//---------------------------------------------------------------------

}
