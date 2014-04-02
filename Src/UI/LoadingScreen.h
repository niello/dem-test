#pragma once
#ifndef __IPG_UI_MAIN_MENU_H__
#define __IPG_UI_MAIN_MENU_H__

#include <UI/Window.h>
#include <Events/EventsFwd.h>

// Loading screen. Reflects loading process and progress for user

namespace CEGUI
{
	class PushButton;
	class EventArgs;
}

namespace UI
{

class CLoadingScreen: public CWindow //???CWindow or CScreen derivative? CScreen owns CDict<CStrID, CWindow>
{
	__DeclareClassNoFactory;

public:

	CLoadingScreen();
	virtual ~CLoadingScreen();

	virtual void Init(CEGUI::Window* pWindow);
};

}

#endif
