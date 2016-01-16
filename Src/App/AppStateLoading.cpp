#include "AppStateLoading.h"

#include <App/AppStates.h>
#include <Game/GameServer.h>
#include <UI/LoadingScreen.h>
#include <UI/UIContext.h>
#include <Render/GPUDriver.h>
#include <Frame/RenderPath.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Debug/DebugServer.h>
#include <Time/TimeServer.h>
#include <Events/EventServer.h>
//#include <Audio/AudioServer.h>
#include <Video/VideoServer.h>
#include <Physics/PhysicsServer.h>
#include <IO/PathUtils.h>
#include <App/IPGApplication.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateLoading, App::CStateHandler);

void CAppStateLoading::DeleteUnreferencedResources()
{
n_assert(false);
	//RenderSrv->MeshMgr.DeleteUnreferenced();
	//RenderSrv->MaterialMgr.DeleteUnreferenced();
	//RenderSrv->TextureMgr.DeleteUnreferenced();
	//GameSrv->AnimationMgr.DeleteUnreferenced();
	//PhysicsSrv->CollisionShapeMgr.DeleteUnreferenced();
}
//---------------------------------------------------------------------

void CAppStateLoading::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	TimeSrv->Trigger();
	GameSrv->PauseGame(true);

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
		Ptr<UI::CLoadingScreen> LoadingScreen = n_new(UI::CLoadingScreen);
		LoadingScreen->Load("LoadingScreen.layout");

		IPGApp->MainUIContext->SetRootWindow(LoadingScreen);
		IPGApp->MainUIContext->ShowGUI();
		IPGApp->MainUIContext->HideMouseCursor();

		View.GPU = IPGApp->GPU;
		View.RenderPath = (Frame::CRenderPath*)RRP->GetObject();
		View.RTs.SetSize(1);
		View.RTs[0] = IPGApp->GPU->GetSwapChainRenderTarget(IPGApp->MainSwapChainIndex);
		View.UIContext = IPGApp->MainUIContext;
	}


	//!!!DBG HACK for sync loading!
	OnFrame();

	ELoadingRequest Request = (ELoadingRequest)Params->Get<int>(CStrID("Request"));
	switch (Request)
	{
		case Request_NewLevel:
		{
			CStrID LevelID = Params->Get<CStrID>(CStrID("LevelID"), CStrID("__NewLevel__"));
			Data::CParams DefaultLevelDesc;
			//!!!fill or load DefaultLevelDesc! or level will get defaults inside
			//or desc must be added as a state param along with LevelID!
			GameSrv->LoadLevel(LevelID, DefaultLevelDesc);
			break;
		}
		case Request_LoadLevel:
		{
			CStrID LevelID = Params->Get<CStrID>(CStrID("LevelID"));
			const CString& FileName = Params->Get<CString>(CStrID("FileName"));
			n_assert(FileName.IsValid());
			Data::PParams Desc = DataSrv->LoadPRM(FileName);
			n_assert(Desc.IsValidPtr());
			GameSrv->LoadLevel(LevelID, *Desc);
			break;
		}
		case Request_NewGame:
		{
			bool WasGameStarted = GameSrv->IsGameStarted();
			//???where to get file name? ???Params->Get<CString>(CStrID("FileName"));
			GameSrv->StartNewGame(CString("Export:Game/Main.prm"));
			if (WasGameStarted) DeleteUnreferencedResources();
			GameSrv->ValidateAllLevels();
			break;
		}
		case Request_ContinueGame:
		{
			bool WasGameStarted = GameSrv->IsGameStarted();
			//???where to get file name? ???Params->Get<CString>(CStrID("FileName"));
			GameSrv->ContinueGame(CString("Export:Game/Main.prm"));
			if (WasGameStarted) DeleteUnreferencedResources();
			GameSrv->ValidateAllLevels();
			break;
		}
		case Request_LoadGame:
		{
			bool WasGameStarted = GameSrv->IsGameStarted();
			GameSrv->LoadGame(Params->Get<CString>(CStrID("SavedGameName")));
			if (WasGameStarted) DeleteUnreferencedResources();
			GameSrv->ValidateAllLevels();
			break;
		}
		case Request_Transition:
		{
			Data::PDataArray IDs = Params->Get<Data::PDataArray>(CStrID("EntityIDs"));
			n_assert(IDs->GetCount());

			CStrID LevelID = GetStrID(*Params, CStrID("LevelID"));
			CStrID MarkerID = GetStrID(*Params, CStrID("MarkerID"));
			bool IsFarTravel = Params->Get<bool>(CStrID("IsFarTravel"));
			bool IsPartyTravel = Params->Get<bool>(CStrID("IsPartyTravel"));

			CArray<CStrID> TravellerIDs(IDs->GetCount(), 0);
			for (int i = 0; i < IDs->GetCount(); ++i)
				TravellerIDs.Add(GetStrID(IDs->Get(i)));

			if (WorldMgr->MakeTransition(TravellerIDs, LevelID, MarkerID, IsFarTravel))
			{
				if (IsFarTravel) DeleteUnreferencedResources();
				GameSrv->ValidateLevel(LevelID);
				if (IsPartyTravel)
				{
					Game::CGameLevel* pLevel = GameSrv->GetLevel(LevelID);
					pLevel->ClearSelection();
					for (int i = 0; i < TravellerIDs.GetCount(); ++i) //???test and add only patry members to selection?
						pLevel->AddToSelection(TravellerIDs[i]);
					GameSrv->SetActiveLevel(LevelID);
				}
			}
			else Sys::Error("Transition to %s failed!", LevelID.CStr());

			break;
		}
		default: Sys::Error("Unknown game setup mode: %d!", Request);
	}
}
//---------------------------------------------------------------------

void CAppStateLoading::OnStateLeave(CStrID NextState)
{
	if (View.UIContext.IsValidPtr())
	{
		View.UIContext->HideGUI();
		View.UIContext->ShowMouseCursor();
		View.UIContext->SetRootWindow(NULL);
		View.UIContext = NULL;
	}

	View.RenderPath = NULL;
	View.RTs.SetSize(0);

	GameSrv->PauseGame(false);
}
//---------------------------------------------------------------------

CStrID CAppStateLoading::OnFrame()
{
	TimeSrv->Trigger();
	EventSrv->ProcessPendingEvents();
//	RenderSrv->GetDisplay().ProcessWindowMessages();
	DbgSrv->Trigger();
	UISrv->Trigger((float)TimeSrv->GetFrameTime());

	VideoSrv->Trigger();
//	AudioSrv->Trigger();

	Render::CGPUDriver* pGPU = View.GPU;
	int SwapChainIdx = IPGApp->MainSwapChainIndex;
	if (View.RenderPath.IsValidPtr() && pGPU->SwapChainExists(SwapChainIdx))
	{
		//???begin-end to a render path? anyway RP renders the whole view (RT/SwapChain)!
		//!!!rp/view doesn't know anything about present, so present manually!
		if (pGPU->BeginFrame())
		{
			//???!!!store RP outside the view?! logically view doesn't own RP
			//!!!use return value!
			View.RenderPath->Render(View);

			pGPU->EndFrame();
			pGPU->Present(SwapChainIdx);
		}
	}

	CoreSrv->Trigger();

	//!!!process loading if sync, wait for loading thread end if async!
	IPGApp->FSM.RequestState(CStrID("Game")); //???!!!use CAppStateLoading::OnFrame return value!?

	return GetID();
}
//---------------------------------------------------------------------

} // namespace Application
