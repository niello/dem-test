#include "TipWindow.h"

#include <Game/GameServer.h>
#include <Events/EventServer.h>
#include <mathlib/rectangle.h>

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
		UpdateBinding();
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
			UpdateBinding();
			SUBSCRIBE_PEVENT(OnUIUpdate, CTipWindow, OnUIUpdate);
		}
	}
	else
	{
		UNSUBSCRIBE_EVENT(OnUIUpdate);
		//EntityID = CStrID::Empty;
	}
	CWindow::SetVisible(Visible);
}
//---------------------------------------------------------------------

void CTipWindow::UpdateBinding()
{
	n_assert(EntityID.IsValid() && pWnd);

	// Request active level instead of entitie's level, because UI works for active level
	Game::CGameLevel* pLevel = GameSrv->GetActiveLevel();
	Game::CEntity* pEntity = EntityMgr->GetEntity(EntityID, true);
	if (!pLevel || !pEntity || pEntity->GetLevel() != pLevel)
	{
		Hide();
		return;
	}

	rectangle ScreenRect;
	pLevel->GetEntityScreenRect(ScreenRect, *pEntity, &WorldOffset);
		
	vector2 WndSize = GetSizeRel(),
			WndPos = ScreenOffset;

	if (Alignment & TipAlignTop)
		WndPos.y += ScreenRect.v0.y - WndSize.y;
	else if (Alignment & TipAlignBottom)
		WndPos.y += ScreenRect.v1.y;
	else
		WndPos.y += (ScreenRect.v0.y + ScreenRect.v1.y - WndSize.y) * 0.5f;

	if (Alignment & TipAlignLeft)
		WndPos.x += ScreenRect.v0.x - WndSize.x;
	else if (Alignment & TipAlignRight)
		WndPos.x += ScreenRect.v1.x;
	else
		WndPos.x += (ScreenRect.v0.x + ScreenRect.v1.x - WndSize.x) * 0.5f;
	
	SetPositionRel(WndPos);
}
//---------------------------------------------------------------------

bool CTipWindow::OnUIUpdate(const Events::CEventBase& Event)
{
	UpdateBinding();
	OK;
}
//---------------------------------------------------------------------

}