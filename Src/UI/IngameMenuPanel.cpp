#include "IngameMenuPanel.h"

#include <Items/Prop/PropInventory.h>
#include <Game/Entity.h>
#include <Game/GameServer.h>
#include <Events/EventServer.h>
#include <CEGUI/Event.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{

CIngameMenuPanel::CIngameMenuPanel()
{
}
//---------------------------------------------------------------------

void CIngameMenuPanel::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);

	CString WndName(pWindow->getName().c_str());

	pInventoryBtn = (CEGUI::PushButton*)pWnd->getChild("InvBtn");
	pInventoryBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CIngameMenuPanel::OnInventoryBtnClick, this));
}
//---------------------------------------------------------------------

bool CIngameMenuPanel::OnInventoryBtnClick(const CEGUI::EventArgs& e)
{
	NOT_IMPLEMENTED;
	Game::CEntity* pEnt = NULL;
	//if (GameSrv->GetActiveLevel())
	//{
	//	const CArray<CStrID>& Sel = GameSrv->GetActiveLevel()->GetSelection();
	//	for (UPTR i = 0; i < Sel.GetCount(); ++i)
	//	{
	//		pEnt = EntityMgr->GetEntity(Sel[i]);
	//		if (pEnt && pEnt->HasProperty<Prop::CPropInventory>()) break;
	//		else pEnt = NULL;
	//	}
	//}

	if (pEnt)
	{
		Data::PParams P = n_new(Data::CParams);
		P->Set(CStrID("EntityID"), pEnt->GetUID());
		EventSrv->FireEvent(CStrID("ShowInventory"), P);
	}

	OK;
}
//---------------------------------------------------------------------

}
