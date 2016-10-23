#include "IngameMenuPanel.h"

#include <Items/Prop/PropInventory.h>
#include <Game/Entity.h>
#include <Game/EntityManager.h>
#include <Game/GameLevelView.h>
#include <Events/EventServer.h>
#include <CEGUI/Event.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{

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
	if (!pView) OK;

	Game::CEntity* pEnt = NULL;
	const CArray<CStrID>& Sel = pView->GetSelection();
	for (UPTR i = 0; i < Sel.GetCount(); ++i)
	{
		CStrID EntityID = Sel[i];
		pEnt = EntityMgr->GetEntity(EntityID);
		if (pEnt && pEnt->HasProperty<Prop::CPropInventory>())
		{
			Data::PParams P = n_new(Data::CParams(1));
			P->Set(CStrID("EntityID"), EntityID);
			EventSrv->FireEvent(CStrID("ShowInventory"), P);
			OK;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}
