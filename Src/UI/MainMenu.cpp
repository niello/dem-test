#include "MainMenu.h"

#include <App/IPGApplication.h>
#include <App/AppStates.h>

#include <CEGUI/widgets/PushButton.h>

namespace UI
{
__ImplementClassNoFactory(UI::CMainMenu, UI::CUIWindow);

CMainMenu::CMainMenu()
{
}
//---------------------------------------------------------------------

CMainMenu::~CMainMenu()
{
}
//---------------------------------------------------------------------

void CMainMenu::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);

	CString WndName(pWindow->getName().c_str());

	pNewGameBtn = (CEGUI::PushButton*)pWnd->getChild("BtnNewGame");
	pNewGameBtn->setDrawMode(DrawModeFlagWindowOpaque);
	pNewGameBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CMainMenu::OnNewGameBtnClick, this));

	CEGUI::PushButton* pBtn = (CEGUI::PushButton*)pWnd->getChild("BtnQuit");
	pBtn->setDrawMode(DrawModeFlagWindowOpaque);
	pBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CMainMenu::OnQuitBtnClick, this));
}
//---------------------------------------------------------------------

bool CMainMenu::OnNewGameBtnClick(const CEGUI::EventArgs& e)
{
	//ConnOnNewGameBtnClick->disconnect();
	//ConnOnNewGameBtnClick = NULL;

	Data::PParams P = n_new(Data::CParams);
	P->Set(CStrID("Request"), (int)App::Request_NewGame);
	IPGApp->FSM.RequestState(CStrID("Loading"), P);

	OK;
}
//---------------------------------------------------------------------

bool CMainMenu::OnQuitBtnClick(const CEGUI::EventArgs& e)
{
	IPGApp->FSM.RequestState(CStrID::Empty);
	OK;
}
//---------------------------------------------------------------------

}