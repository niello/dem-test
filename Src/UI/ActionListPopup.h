#pragma once
#ifndef __IPG_UI_ACT_LIST_POPUP_H__
#define __IPG_UI_ACT_LIST_POPUP_H__

#include <UI/UIWindow.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>

// Popup menu appears on right click and offers a list of actions available to actor

namespace Game
{
	class CEntity;
}

namespace Prop
{
	class CPropUIControl;
}

namespace UI
{

class CActionListPopup: public CUIWindow
{
protected:

	//???description (static tooltip-like hint with action info)

	//???or smart ptrs?
	Game::CEntity*			pActorEnt;
	Prop::CPropUIControl*	pCtl;

	bool OnBtnClicked(const CEGUI::EventArgs& e);
	bool OnClickOutsideRect(const CEGUI::EventArgs& e); //auto-close popup
	
	DECLARE_EVENT_HANDLER(ShowActionListPopup, OnShow);
	DECLARE_EVENT_HANDLER(HideActionListPopup, OnHide);

	void Clear();

public:

	CActionListPopup(): pActorEnt(NULL), pCtl(NULL) {}
	//virtual ~CActionListPopup();

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif