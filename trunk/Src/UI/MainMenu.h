#pragma once
#ifndef __IPG_UI_MAIN_MENU_H__
#define __IPG_UI_MAIN_MENU_H__

#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>

// Main menu allows user to start new game or load saved or smth

namespace CEGUI
{
	class PushButton;
	class EventArgs;
}

namespace UI
{

class CMainMenu: public CUIWindow //???CUIWindow or CScreen derivative? CScreen owns CDict<CStrID, CUIWindow>
{
	__DeclareClassNoFactory;

protected:

	//!!!list of saves, list of levels!

	CEGUI::PushButton*			pNewGameBtn;

	//CEGUI::Event::Connection	ConnOnNewGameBtnClick; //ScopedConnection

	bool OnNewGameBtnClick(const CEGUI::EventArgs& e);
	bool OnQuitBtnClick(const CEGUI::EventArgs& e);

public:

	CMainMenu();
	virtual ~CMainMenu();

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif
