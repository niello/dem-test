#include "AppStateMenu.h"

#include <App/IPGApplication.h>
#include <System/OSWindow.h>
#include <Debug/DebugServer.h>
#include <UI/UIContext.h>
#include <UI/MainMenu.h>
#include <Input/InputTranslator.h>
#include <Frame/RenderPath.h>
#include <Render/GPUDriver.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Time/TimeServer.h>
#include <Events/EventServer.h>
#include <Video/VideoServer.h>
#include <IO/PathUtils.h>
#include <Core/CoreServer.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateMenu, App::CStateHandler);

void CAppStateMenu::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	TimeSrv->Trigger();

	const char* pRenderPathURI = "RenderPathes:D3D11Forward.rp";
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
		IPGApp->MainUIContext->SubscribeOnInput(IPGApp->MainWindow.GetUnsafe(), 100);

		MenuView.GPU = IPGApp->GPU;
		MenuView.SetRenderPath(RRP->GetObject<Frame::CRenderPath>());
		MenuView.RTs[0] = IPGApp->GPU->GetSwapChainRenderTarget(IPGApp->MainSwapChainIndex);
		MenuView.UIContext = IPGApp->MainUIContext;
	}

	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ShowDebugConsole, CAppStateMenu, OnShowDebugConsole);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ShowDebugWatcher, CAppStateMenu, OnShowDebugWatcher);
}
//---------------------------------------------------------------------

void CAppStateMenu::OnStateLeave(CStrID NextState)
{
	UNSUBSCRIBE_EVENT(ShowDebugConsole);
	UNSUBSCRIBE_EVENT(ShowDebugWatcher);

	if (MenuView.UIContext.IsValidPtr())
	{
		MenuView.UIContext->UnsubscribeFromInput();
		MenuView.UIContext->HideGUI();
		MenuView.UIContext->SetRootWindow(NULL);
		MenuView.UIContext = NULL;
	}

	MenuView.SetRenderPath(NULL);
}
//---------------------------------------------------------------------

CStrID CAppStateMenu::OnFrame()
{
	if (!IPGApp->MainWindow->IsOpen()) return CStrID::Empty;

	float FrameTime = (float)TimeSrv->GetFrameTime();

	TimeSrv->Trigger();
	EventSrv->ProcessPendingEvents();
	DbgSrv->Trigger();
	if (UI::CUIServer::HasInstance()) UISrv->Trigger(FrameTime);
	IPGApp->pInputTranslator->Trigger(FrameTime);
	IPGApp->pInputTranslator->FireQueuedEvents();

	//AudioSrv->Trigger();
	VideoSrv->Trigger();

	Render::CGPUDriver* pGPU = MenuView.GPU;
	int SwapChainIdx = IPGApp->MainSwapChainIndex;
	if (MenuView.GetRenderPath() && pGPU->SwapChainExists(SwapChainIdx))
	{
		//???begin-end to a render path? anyway RP renders the whole view (RT/SwapChain)!
		//!!!rp/view doesn't know anything about present, so present manually!
		if (pGPU->BeginFrame())
		{
			//???!!!store RP outside the view?! logically view doesn't own RP
			//!!!use return value!
			MenuView.GetRenderPath()->Render(MenuView);

			pGPU->EndFrame();
			pGPU->Present(SwapChainIdx);
		}
	}

	CoreSrv->Trigger();

	return GetID();
}
//---------------------------------------------------------------------

bool CAppStateMenu::OnShowDebugConsole(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	DbgSrv->TogglePluginWindow(CStrID("Console"));
	OK;
}
//---------------------------------------------------------------------

bool CAppStateMenu::OnShowDebugWatcher(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	DbgSrv->TogglePluginWindow(CStrID("Watcher"));
	OK;
}
//---------------------------------------------------------------------

}