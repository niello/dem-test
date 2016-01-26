#include "LoadingScreen.h"

namespace UI
{
__ImplementClassNoFactory(UI::CLoadingScreen, UI::CUIWindow);

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
	CUIWindow::Init(pWindow);

	//CString WndName = pWindow->getName().c_str();
}
//---------------------------------------------------------------------

}