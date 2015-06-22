#include "AppStateMenu.h"

#include <App/IPGApplication.h>
#include <System/OSWindow.h>
#include <Debug/DebugServer.h>
#include <UI/UIServer.h>
#include <UI/MainMenu.h>
#include <Time/TimeServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>
#include <Input/InputServer.h>
#include <Core/CoreServer.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateMenu, App::CStateHandler);

void CAppStateMenu::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	TimeSrv->Trigger();

	//???load once?
	if (UI::CUIServer::HasInstance())
	{
		Ptr<UI::CMainMenu> MainMenu = n_new(UI::CMainMenu);
		MainMenu->Load("MainMenu.layout");
		UISrv->RegisterScreen(CStrID("MainMenu"), MainMenu);
		UISrv->SetRootScreen(MainMenu);
		UISrv->ShowGUI();
	}
}
//---------------------------------------------------------------------

void CAppStateMenu::OnStateLeave(CStrID NextState)
{
	if (UI::CUIServer::HasInstance()) UISrv->HideGUI();
}
//---------------------------------------------------------------------

CStrID CAppStateMenu::OnFrame()
{
	if (!IPGApp->MainWindow->IsOpen()) return CStrID::Empty;

	TimeSrv->Trigger();
	EventSrv->ProcessPendingEvents();
	InputSrv->Trigger();
	IPGApp->MainWindow->ProcessMessages();
	DbgSrv->Trigger();
	if (UI::CUIServer::HasInstance()) UISrv->Trigger((float)TimeSrv->GetFrameTime());

	//AudioSrv->Trigger();
	VideoSrv->Trigger();

	//if (RenderSrv->BeginFrame())
	//{
	//	RenderSrv->Clear(Render::Clear_All, 0xff000000, 1.f, 0); 
	//	UISrv->Render();
	//	RenderSrv->EndFrame();
	//	RenderSrv->Present(); //!!!must be called as late as possible after EndFrame!
	//}

	CoreSrv->Trigger();

	return GetID();
}
//---------------------------------------------------------------------

}