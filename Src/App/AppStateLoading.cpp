#include "AppStateLoading.h"

#include <App/AppStates.h>
#include <Game/GameServer.h>
#include <Game/GameLevel.h>
#include <UI/LoadingScreen.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <Render/GPUDriver.h>
#include <Frame/RenderPath.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Data/ParamsUtils.h>
#include <Data/DataArray.h>
#include <Debug/DebugServer.h>
#include <Core/CoreServer.h>
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
//!!!IMPLEMENT!
	Sys::DbgOut("IMPLEMENT CAppStateLoading::DeleteUnreferencedResources()!\n");
//n_assert(false);
	//ResourceMgr->UnloadUnreferencedResources(); //???restrict types?
}
//---------------------------------------------------------------------

void CAppStateLoading::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	StateParams = Params;

	CoreSrv->Trigger();
	GameSrv->PauseGame(true);

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
		Ptr<UI::CLoadingScreen> LoadingScreen = n_new(UI::CLoadingScreen);
		LoadingScreen->Load("LoadingScreen.layout");

		IPGApp->MainUIContext->SetRootWindow(LoadingScreen);
		IPGApp->MainUIContext->ShowGUI();
		IPGApp->MainUIContext->HideMouseCursor();

		View.GPU = IPGApp->GPU;
		View.SetRenderPath(RRP->GetObject<Frame::CRenderPath>());
		View.RTs[0] = IPGApp->GPU->GetSwapChainRenderTarget(IPGApp->MainSwapChainIndex);
		View.UIContext = IPGApp->MainUIContext;
	}

	//!!!spawn async loading task(s) here, if async!
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

	View.SetRenderPath(NULL);
	View.GPU = NULL;

	GameSrv->PauseGame(false);
}
//---------------------------------------------------------------------

CStrID CAppStateLoading::OnFrame()
{
	CoreSrv->Trigger();
	EventSrv->ProcessPendingEvents();
	DbgSrv->Trigger();
	UISrv->Trigger((float)CoreSrv->GetFrameTime());

	VideoSrv->Trigger();
//	AudioSrv->Trigger();

	Render::CGPUDriver* pGPU = View.GPU;
	int SwapChainIdx = IPGApp->MainSwapChainIndex;
	if (View.GetRenderPath() && pGPU->SwapChainExists(SwapChainIdx))
	{
		//???begin-end to a render path? anyway RP renders the whole view (RT/SwapChain)!
		pGPU->Present(SwapChainIdx);
		if (pGPU->BeginFrame())
		{
			//???!!!store RP outside the view?! logically view doesn't own RP
			//!!!use return value!
			View.GetRenderPath()->Render(View);

			pGPU->EndFrame();
		}
	}

	///// Emulates loading! /////////////////////////////////
	//???spawn async based on request? some requests are sync?
	//if so, loading screen will not be rendered properly for sync tasks!

	bool LoadingTaskFinished = false;

	ELoadingRequest Request = (ELoadingRequest)StateParams->Get<int>(CStrID("Request"));
	switch (Request)
	{
		case Request_NewLevel:
		{
			CStrID LevelID = StateParams->Get<CStrID>(CStrID("LevelID"), CStrID("__NewLevel__"));
			Data::CParams DefaultLevelDesc;
			//!!!fill or load DefaultLevelDesc! or level will get defaults inside
			//or desc must be added as a state param along with LevelID!
			GameSrv->LoadLevel(LevelID, DefaultLevelDesc);
			break;
		}
		case Request_LoadLevel:
		{
			CStrID LevelID = StateParams->Get<CStrID>(CStrID("LevelID"));
			const CString& FileName = StateParams->Get<CString>(CStrID("FileName"));
			n_assert(FileName.IsValid());
			Data::PParams Desc;
			ParamsUtils::LoadParamsFromPRM(FileName, Desc);
			n_assert(Desc.IsValidPtr());
			GameSrv->LoadLevel(LevelID, *Desc);
			break;
		}
		case Request_NewGame:
		case Request_ContinueGame:
		{
			bool WasGameStarted = GameSrv->IsGameStarted();

			//???where to get file name? ???Params->Get<CString>(CStrID("FileName"));
			const char* pGameFile = "Export:Game/Main.prm";
			if (Request == Request_NewGame)
				GameSrv->StartNewGame(pGameFile);
			else
				GameSrv->ContinueGame(pGameFile);

			if (WasGameStarted) DeleteUnreferencedResources();
			GameSrv->ValidateAllLevels(pGPU);

			break;
		}
		case Request_LoadGame:
		{
			bool WasGameStarted = GameSrv->IsGameStarted();
			GameSrv->LoadGame(StateParams->Get<CString>(CStrID("SavedGameName")));
			if (WasGameStarted) DeleteUnreferencedResources();
			GameSrv->ValidateAllLevels(pGPU);
			break;
		}
		case Request_Transition:
		{
			Data::PDataArray IDs = StateParams->Get<Data::PDataArray>(CStrID("EntityIDs"));
			n_assert(IDs->GetCount());

			CStrID LevelID = GetStrID(*StateParams, CStrID("LevelID"));
			CStrID MarkerID = GetStrID(*StateParams, CStrID("MarkerID"));
			bool IsFarTravel = StateParams->Get<bool>(CStrID("IsFarTravel"));
			bool IsPartyTravel = StateParams->Get<bool>(CStrID("IsPartyTravel"));

			CArray<CStrID> TravellerIDs(IDs->GetCount(), 0);
			for (UPTR i = 0; i < IDs->GetCount(); ++i)
				TravellerIDs.Add(GetStrID(IDs->Get(i)));

			if (WorldMgr->MakeTransition(TravellerIDs, LevelID, MarkerID, IsFarTravel))
			{
				if (IsFarTravel) DeleteUnreferencedResources();

				Game::CGameLevel* pLevel = GameSrv->GetLevel(LevelID);
				pLevel->Validate(pGPU);

				if (IsPartyTravel)
				{
					//!!!do in view!
					/*
					pLevel->ClearSelection();
					RPG::CFaction* pParty = FactionMgr->GetFaction(CStrID("Party"));
					if (pParty)
					{
						for (UPTR i = 0; i < TravellerIDs.GetCount(); ++i)
						{
							CStrID TravellerID = TravellerIDs[i];
							if (pParty->IsMember(TravellerID))
								pLevel->AddToSelection(TravellerID);
						}
					}
					*/
				}
			}
			else Sys::Error("Transition to %s failed!", LevelID.CStr());

			break;
		}
		default: Sys::Error("Unknown game setup mode: %d!", Request);
	}

	LoadingTaskFinished = true;
	//////////////////////

	//!!!DBG TMP! to render loading screen while loading is sync
	if (LoadingTaskFinished && View.GetRenderPath() && pGPU->SwapChainExists(SwapChainIdx))
		pGPU->Present(SwapChainIdx);

	return LoadingTaskFinished ? CStrID("Game") : GetID();
}
//---------------------------------------------------------------------

}