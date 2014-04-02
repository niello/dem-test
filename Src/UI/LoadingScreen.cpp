#include "LoadingScreen.h"

namespace UI
{
__ImplementClassNoFactory(UI::CLoadingScreen, UI::CWindow);

using namespace Events;

CLoadingScreen::CLoadingScreen()
{
}
//---------------------------------------------------------------------

CLoadingScreen::~CLoadingScreen()
{
}
//---------------------------------------------------------------------

void CLoadingScreen::Init(CEGUI::Window* pWindow)
{
	CWindow::Init(pWindow);

	//CString WndName = pWindow->getName().c_str();
}
//---------------------------------------------------------------------

}