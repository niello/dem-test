#include "AppStateMenu.h"

#include <App/IPGApplication.h>
#include <System/OSWindow.h>
#include <Debug/DebugServer.h>
#include <UI/UIContext.h>
#include <UI/MainMenu.h>
#include <Frame/RenderPath.h>
#include <Frame/RenderPathLoader.h>
#include <Render/GPUDriver.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Time/TimeServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>
#include <Input/InputServer.h>
#include <IO/PathUtils.h>
#include <Core/CoreServer.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateMenu, App::CStateHandler);

void CAppStateMenu::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	TimeSrv->Trigger();

	const char* pRenderPathURI = "Shaders:D3D11Forward.hrd";
	Resources::PResource RRP = ResourceMgr->RegisterResource(pRenderPathURI);
	if (!RRP->IsLoaded())
	{
		Resources::PResourceLoader Loader = RRP->GetLoader();
		if (Loader.IsNullPtr())
			Loader = ResourceMgr->CreateDefaultLoaderFor<Frame::CRenderPath>(PathUtils::GetExtension(pRenderPathURI));
		ResourceMgr->LoadResourceSync(*RRP, *Loader);
		n_assert(RRP->IsLoaded());
	}

	if (IPGApp->GPU->SwapChainExists(IPGApp->MainSwapChainIndex))
	{
		Ptr<UI::CMainMenu> MainMenu = n_new(UI::CMainMenu);
		MainMenu->Load("MainMenu.layout");

		IPGApp->MainUIContext->SetRootWindow(MainMenu);
		IPGApp->MainUIContext->ShowGUI();
		IPGApp->MainUIContext->SetDefaultMouseCursor("TaharezLook/MouseArrow");

		MenuView.GPU = IPGApp->GPU;
		MenuView.RenderPath = (Frame::CRenderPath*)RRP->GetObject();
		MenuView.RTs.SetSize(1);
		MenuView.RTs[0] = IPGApp->GPU->GetSwapChainRenderTarget(IPGApp->MainSwapChainIndex);
		MenuView.UIContext = IPGApp->MainUIContext;
		//???set GPUDrv?
	}
}
//---------------------------------------------------------------------

void CAppStateMenu::OnStateLeave(CStrID NextState)
{
	if (MenuView.UIContext.IsValidPtr())
	{
		MenuView.UIContext->HideGUI();
		MenuView.UIContext->SetRootWindow(NULL);
	}

	MenuView.RenderPath = NULL;
	MenuView.RTs.SetSize(0);
	MenuView.UIContext = NULL;
}
//---------------------------------------------------------------------

CStrID CAppStateMenu::OnFrame()
{
	if (!IPGApp->MainWindow->IsOpen()) return CStrID::Empty;

	TimeSrv->Trigger();
	EventSrv->ProcessPendingEvents();
	InputSrv->Trigger();
	DbgSrv->Trigger();
	if (UI::CUIServer::HasInstance()) UISrv->Trigger((float)TimeSrv->GetFrameTime());

	//AudioSrv->Trigger();
	VideoSrv->Trigger();

	Render::CGPUDriver* pGPU = MenuView.GPU;
	int SwapChainIdx = IPGApp->MainSwapChainIndex;
	if (MenuView.RenderPath.IsValidPtr() && pGPU->SwapChainExists(SwapChainIdx))
	{
		//???begin-end to a render path? anyway RP renders the whole view (RT/SwapChain)!
		//!!!rp/view doesn't know anything about present, so present manually!
		if (pGPU->BeginFrame())
		{
			//???!!!store RP outside the view?! logically view doesn't own RP
			//!!!use return value!
			MenuView.RenderPath->Render(MenuView);

			pGPU->EndFrame();
			pGPU->Present(SwapChainIdx);
		}
	}

	CoreSrv->Trigger();

	return GetID();
}
//---------------------------------------------------------------------

}