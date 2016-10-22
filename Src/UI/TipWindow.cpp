#include "TipWindow.h"

#include <Game/GameServer.h>
#include <Events/EventServer.h>
#include <Data/Regions.h>

namespace UI
{

void CTipWindow::BindToEntity(CStrID _EntityID, ETipAlignment _Alignment, vector2 _ScreenOffset, vector3 _WorldOffset)
{
	EntityID = _EntityID;
	Alignment = _Alignment;
	ScreenOffset = _ScreenOffset;
	WorldOffset = _WorldOffset;

	//!!!UNSUBSCRIBE IN Term()!
	if (IsVisible() && EntityID.IsValid())
	{
		//UpdateBinding();
		if (!IS_SUBSCRIBED(OnUIUpdate))
			SUBSCRIBE_PEVENT(OnUIUpdate, CTipWindow, OnUIUpdate);
	}
}
//---------------------------------------------------------------------

void CTipWindow::SetVisible(bool Visible)
{
	if (Visible)
	{
		if (EntityID.IsValid())
		{
			//UpdateBinding();
			SUBSCRIBE_PEVENT(OnUIUpdate, CTipWindow, OnUIUpdate);
		}
	}
	else
	{
		UNSUBSCRIBE_EVENT(OnUIUpdate);
		//EntityID = CStrID::Empty;
	}
	CUIWindow::SetVisible(Visible);
}
//---------------------------------------------------------------------

void CTipWindow::UpdateBinding()
{
	NOT_IMPLEMENTED;

	n_assert(EntityID.IsValid() && pWnd);

	// Request active level instead of entitie's level, because UI works for active level
	Game::CGameLevel* pLevel = NULL;//GameSrv->GetActiveLevel();
	Game::CEntity* pEntity = EntityMgr->GetEntity(EntityID, true);
	if (!pLevel || !pEntity || pEntity->GetLevel() != pLevel)
	{
		Hide();
		return;
	}

	Data::CRect ScreenRect;
	//pLevel->GetEntityScreenRect(ScreenRect, *pEntity, &WorldOffset);
		
	vector2 WndSize = GetSizeRel(),
			WndPos = ScreenOffset;

	if (Alignment & TipAlign_Top)
		WndPos.y += (float)ScreenRect.Y - WndSize.y;
	else if (Alignment & TipAlign_Bottom)
		WndPos.y += (float)ScreenRect.Bottom();
	else
		WndPos.y += ((float)ScreenRect.Y + (float)ScreenRect.Bottom() - WndSize.y) * 0.5f;

	if (Alignment & TipAlign_Left)
		WndPos.x += (float)ScreenRect.X - WndSize.x;
	else if (Alignment & TipAlign_Right)
		WndPos.x += (float)ScreenRect.Right();
	else
		WndPos.x += ((float)ScreenRect.X + (float)ScreenRect.Right() - WndSize.x) * 0.5f;
	
	SetPositionRel(WndPos);
}
//---------------------------------------------------------------------

bool CTipWindow::OnUIUpdate(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//UpdateBinding();
	OK;
}
//---------------------------------------------------------------------

}