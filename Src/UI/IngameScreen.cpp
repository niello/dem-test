#include "IngameScreen.h"

#include "ActionListPopup.h"
#include "DialogueWindow.h"
#include "IngameMenuPanel.h"
#include "Inventory.h"
#include "ContainerWindow.h"
#include "MoveItemsWindow.h"
#include <Quests/Quest.h> //!!!only for status!
#include <UI/CEGUI/FmtLbTextItem.h>
#include <Events/EventServer.h>

#include <CEGUI/WindowManager.h>
#include <CEGUI/Font.h>
#include <CEGUI/widgets/Listbox.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{
__ImplementClassNoFactory(UI::CIngameScreen, UI::CUIWindow);

CIngameScreen::CIngameScreen()
{
}
//---------------------------------------------------------------------

CIngameScreen::~CIngameScreen()
{
}
//---------------------------------------------------------------------

void CIngameScreen::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);

	CString WndName(pWindow->getName().c_str());

	CEGUI::Window* pAPWnd =
		CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/PopupMenu", (WndName + "/ActionListPopup").CStr());
	pWnd->addChild(pAPWnd);
	ActionPopup = n_new(CActionListPopup);
	ActionPopup->Init(pAPWnd);
	ActionPopup->Hide();

	IngameMenuPanel = n_new(CIngameMenuPanel);
	IngameMenuPanel->Init(pWnd->getChild("IngameMenuPanel"));

	Inventory = n_new(CInventory);
	Inventory->Init(pWnd->getChild("Inventory"));

	ContainerWindow = n_new(CContainerWindow);
	ContainerWindow->Init(pWnd->getChild("ContainerWindow"));
	ContainerWindow->Hide();

	MoveItemsWindow = n_new(CMoveItemsWindow);
	MoveItemsWindow->Init(pWnd->getChild("MovingItemsWindow")); //!!!rename!
	MoveItemsWindow->Hide();

	DlgWindow = n_new(CDialogueWindow);
	DlgWindow->Init(pWnd->getChild("DialogueWindow"));
	DlgWindow->Hide(); //???here or inside Init()?
	
	IAOTip = CreateTipWindow(1);

	//!!!tmp!
	CEGUI::Window* pConsoleWnd = pWnd->getChild("Console");
	Console = (CEGUI::Listbox*)pConsoleWnd->getChild("TextArea");
	n_assert(Console);

	CEGUI::PushButton* pBtn = (CEGUI::PushButton*)pWnd->getChild("BtnDbgExit");
	pBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CIngameScreen::OnDbgExitBtnClick, this));

	//!!!NEED TO UNSUBSCRIBE IN Term()!
	SUBSCRIBE_PEVENT(ShowIAOTip, CIngameScreen, ShowIAOTip);
	SUBSCRIBE_PEVENT(HideIAOTip, CIngameScreen, HideIAOTip);
	SUBSCRIBE_PEVENT(ShowPhrase, CIngameScreen, ShowPhrase);
	SUBSCRIBE_PEVENT(HidePhrase, CIngameScreen, HidePhrase);
	SUBSCRIBE_PEVENT(OnQuestStatusChanged, CIngameScreen, OnQuestStatusChanged);
	SUBSCRIBE_PEVENT(OnObjectDescRequested, CIngameScreen, OnObjectDescRequested);
	SUBSCRIBE_PEVENT(OnIAOActionStart, CIngameScreen, OnIAOActionStart);
	SUBSCRIBE_PEVENT(OnIAOActionAbort, CIngameScreen, OnIAOActionAbort);
}
//---------------------------------------------------------------------

Ptr<CTipWindow> CIngameScreen::CreateTipWindow(int TipID)
{
	n_assert2(TipID >= 0 && TipID < 1000, "TipID is out of range [0; 999].");

	char TipName[8];
	sprintf_s(TipName, sizeof(TipName) / sizeof(char), "Tip_%d", TipID);

	CEGUI::Window* pTipWnd = CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/StaticText", TipName);
	pTipWnd->setProperty("HorzFormatting", "HorzCentred");
	pTipWnd->setProperty("FrameEnabled", "false");
	pTipWnd->setProperty("BackgroundEnabled", "false");
	pTipWnd->setMousePassThroughEnabled(true);
	pTipWnd->setFont("DejaVuSans-8");
	pTipWnd->setSize(CEGUI::USize(CEGUI::UDim(0.f, 20.f),
		CEGUI::UDim(0.f, pWnd->getFont()->getFontHeight() + 10.f)));
	pWnd->addChild(pTipWnd);
	Ptr<CTipWindow> Tip = n_new(CTipWindow);
	Tip->Init(pTipWnd);
	Tip->Hide();

	return Tip;
}
//---------------------------------------------------------------------

Ptr<CTipWindow> CIngameScreen::GetOrCreatePhraseTip(CStrID EntityID)
{
	const int TipIndexOffset = 100;

	Ptr<CTipWindow> TipWnd = NULL;
	for (CArray<Ptr<CTipWindow>>::CIterator it = PhraseTips.Begin(); it != PhraseTips.End(); ++it)
	{
		if ((*it)->GetEntityID() == EntityID)
		{
			TipWnd = *it;
			break;
		}
		if ((*it)->GetEntityID() == CStrID::Empty)
			TipWnd = *it; //This window is unbound, so we don't need to create a new window.
	}
	if (TipWnd.IsNullPtr())
	{
		TipWnd = CreateTipWindow(TipIndexOffset + PhraseTips.GetCount());
		PhraseTips.Add(TipWnd);
	}
	return TipWnd;
}
//---------------------------------------------------------------------

bool CIngameScreen::OnDbgExitBtnClick(const CEGUI::EventArgs& e)
{
	EventSrv->FireEvent(CStrID("CloseApplication"));
	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::ShowIAOTip(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	return ShowTip(P->Get<CStrID>(CStrID("EntityID")), IAOTip, P->Get<CString>(CStrID("Text")), TipAlignTop);
}
//---------------------------------------------------------------------

bool CIngameScreen::HideIAOTip(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	IAOTip->Hide();
	////???keep open until ActionListPopup is present? (remember bool IsTipVisible and OnPopupClose apply)?

	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::ShowPhrase(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	// The static text widget types allows the frame and background to be disabled via simple
	// properties, though might be a little heavy-weight for simple labels (c) CEGUI forums
	CStrID EntityID = P->Get<CStrID>(CStrID("EntityID"));
	Ptr<CTipWindow> PhraseTip = GetOrCreatePhraseTip(EntityID);
	PhraseTip->GetWnd()->setProperty("FrameEnabled", "false");
	PhraseTip->GetWnd()->setProperty("BackgroundEnabled", "false");
	
	return ShowTip(EntityID, PhraseTip, P->Get<CString>(CStrID("Text")), TipAlignTop);
}
//---------------------------------------------------------------------

bool CIngameScreen::HidePhrase(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	CStrID EntityID = P->Get<CStrID>(CStrID("EntityID"));
	Ptr<CTipWindow> PhraseTip = GetPhraseTip(EntityID);
	if (PhraseTip.IsValidPtr()) PhraseTip->Hide();

	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::ShowTip(CStrID EntityID, CTipWindow* pTipWnd, const CString& Text, ETipAlignment Alignment)
{
	const CEGUI::Font* f = pTipWnd->GetWnd()->getFont();
	pTipWnd->GetWnd()->setSize(CEGUI::USize(CEGUI::UDim(0.f, f->getTextExtent((CEGUI::utf8*)Text.CStr()) + 20.f),
											CEGUI::UDim(0.f, f->getFontHeight() + 14.f)));
	pTipWnd->GetWnd()->setText((CEGUI::utf8*)Text.CStr());
	pTipWnd->BindToEntity(EntityID, Alignment);
	if (!pTipWnd->IsVisible()) pTipWnd->Show();
	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::OnQuestStatusChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	Story::CQuest::EStatus Status = (Story::CQuest::EStatus)P->Get<int>(CStrID("Status"));

	CString Text((P->Get<bool>(CStrID("IsTask"))) ? "Task \"" : "Quest \"");
	Text += P->Get<CString>(CStrID("Name"));

	switch (Status)
	{
		case Story::CQuest::Opened:
			{
				Text += "\" opened:\n";
				Text += P->Get<CString>(CStrID("Desc"));
				break;
			}
		case Story::CQuest::Done: Text += "\" is done."; break;
		case Story::CQuest::Failed: Text += "\" failed."; break;
		default: Sys::Error("CIngameScreen console: Wrong task status in OnQuestStatusChanged event!");
	}

	CEGUI::FormattedListboxTextItem* NewItem =
		n_new(CEGUI::FormattedListboxTextItem((CEGUI::utf8*)Text.CStr(), CEGUI::HTF_WORDWRAP_LEFT_ALIGNED));//!!!, 0, 0, true);
	NewItem->setTextColours(CEGUI::Colour(0xffb0b0b0));
	Console->addItem(NewItem);
	Console->ensureItemIsVisible(Console->getItemCount() - 1);

	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::OnObjectDescRequested(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const CString& UIDesc = ((const Events::CEvent&)Event).Params->Get<CString>(CStrID("UIDesc"));
	CEGUI::FormattedListboxTextItem* NewItem =
		n_new(CEGUI::FormattedListboxTextItem((CEGUI::utf8*)UIDesc.CStr(), CEGUI::HTF_WORDWRAP_LEFT_ALIGNED));//!!!, 0, 0, true);
	NewItem->setTextColours(CEGUI::Colour(0xffd0d0d0));
	Console->addItem(NewItem);
	Console->ensureItemIsVisible(Console->getItemCount() - 1);
	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::OnIAOActionStart(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	if (P->Get<CStrID>(CStrID("Action")) == "OpenContainer")
	{
		//???call method instead?
		//EventSrv->FireEvent(CStrID("ShowContainerWindow"), P);
	}
	OK;
}
//---------------------------------------------------------------------

bool CIngameScreen::OnIAOActionAbort(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	if (P->Get<CStrID>(CStrID("Action")) == "OpenContainer")
	{
		//???call method instead?
		EventSrv->FireEvent(CStrID("HideContainerWindow"), P);
	}
	OK;
}
//---------------------------------------------------------------------

}