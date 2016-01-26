#include "ActionListPopup.h"

#include <UI/PropUIControl.h>
#include <UI/UIServer.h>
#include <Events/EventServer.h>

#include <CEGUI/WindowManager.h>
#include <CEGUI/widgets/MenuItem.h>
#include <CEGUI/widgets/PopupMenu.h>

namespace UI
{

void CActionListPopup::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);

	pWnd->setFont("DejaVuSans-8");

	//!!!need to unsubscribe somewhere!
	SUBSCRIBE_PEVENT(ShowActionListPopup, CActionListPopup, OnShow);
	SUBSCRIBE_PEVENT(HideActionListPopup, CActionListPopup, OnHide);

	//!!!subscribe on click outside rect only on Show()!
	pWnd->getParent()->subscribeEvent(CEGUI::Window::EventMouseClick,
				CEGUI::Event::Subscriber(&CActionListPopup::OnClickOutsideRect, this));
}
//---------------------------------------------------------------------

void CActionListPopup::Clear()
{
	n_assert(pWnd);
	pActorEnt = NULL;
	pCtl = NULL;
	((CEGUI::PopupMenu*)pWnd)->resetList();
}
//---------------------------------------------------------------------

bool CActionListPopup::OnShow(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	Clear(); //???here or when hidden? need to process re-request when visible

	pActorEnt = (Game::CEntity*)P->Get<PVOID>(CStrID("ActorEntityPtr"));
	pCtl = (Prop::CPropUIControl*)P->Get<PVOID>(CStrID("CtlPtr"));

	if (!pCtl) OK; // Later this may be a case for formation & movement menu

	const CArray<Prop::CPropUIControl::CAction>& Actions = pCtl->GetActions();

	// Actions are sorted by Enabled flag and then by priority
	for (CArray<Prop::CPropUIControl::CAction>::CIterator It = Actions.Begin(); It != Actions.End(); ++It)
		if (It->Visible)
		{
			CEGUI::Window* pItem = CEGUI::WindowManager::getSingleton().createWindow("TaharezLook/MenuItem");
			pItem->setText((CEGUI::utf8*)It->GetUIName());
			pItem->setEnabled(It->Enabled);
			if (It->Enabled)
			{
				pItem->setUserData((void*)It->ID.CStr());
				pItem->subscribeEvent(CEGUI::MenuItem::EventClicked,
					CEGUI::Event::Subscriber(&CActionListPopup::OnBtnClicked, this));
			}
			pWnd->addChild(pItem);
		}

	if (((CEGUI::PopupMenu*)pWnd)->getItemCount())
	{
		CEGUI::Vector2f Pos = pWnd->getGUIContext().getMouseCursor().getPosition();
		SetPosition(CEGUI::UVector2(CEGUI::UDim(0.f, Pos.d_x), CEGUI::UDim(0.f, Pos.d_y)));
		Show();
	}
	else Hide();

	OK;
}
//---------------------------------------------------------------------

bool CActionListPopup::OnHide(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (pCtl == ((const Events::CEvent&)Event).Params->Get<PVOID>(CStrID("CtlPtr"))) Hide();
	OK;
}
//---------------------------------------------------------------------

bool CActionListPopup::OnBtnClicked(const CEGUI::EventArgs& e)
{
	n_assert(pCtl);
	pCtl->ExecuteAction(pActorEnt, CStrID((char*)((const CEGUI::WindowEventArgs&)e).window->getUserData()));
	Hide();
	Clear(); //???clear here, or clear OnHide()???
	OK;
}
//---------------------------------------------------------------------

bool CActionListPopup::OnClickOutsideRect(const CEGUI::EventArgs& e)
{
	//!!!check is outside rect! subscribe & process event correctly!
	//!!!NOW DIRTY HACK!
	CEGUI::Vector2f p = pWnd->getGUIContext().getMouseCursor().getPosition();
	p.d_x += 2.f;
	p.d_y += 2.f;
	if (!pWnd->isHit(p, true))
	{
		Hide();
		Clear(); //???clear here???
	}
	OK;
}
//---------------------------------------------------------------------

}
