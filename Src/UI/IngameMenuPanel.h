#pragma once
#ifndef __IPG_UI_INGAME_MENU_PANEL_H__
#define __IPG_UI_INGAME_MENU_PANEL_H__

#include <UI/UIWindow.h>

// Ingame panel that provides access to character-related screens like inventory etc and
// game-related functions like main menu screen or game pause or smth.

namespace Game
{
	class CGameLevelView;
}

namespace CEGUI
{
	class PushButton;
	class EventArgs;
}

namespace UI
{

class CIngameMenuPanel: public CUIWindow
{
protected:

	CEGUI::PushButton*		pInventoryBtn;

	bool OnInventoryBtnClick(const CEGUI::EventArgs& e);

public:

	Game::CGameLevelView*	pView;

	CIngameMenuPanel(): pView(NULL) {}

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif