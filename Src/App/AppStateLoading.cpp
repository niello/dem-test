#include "AppStateLoading.h"

#include <App/AppStates.h>
#include <Game/GameServer.h>
#include <UI/LoadingScreen.h>
#include <UI/UIServer.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <Debug/DebugServer.h>
#include <Time/TimeServer.h>
#include <Events/EventServer.h>
//#include <Audio/AudioServer.h>
#include <Video/VideoServer.h>
#include <Physics/PhysicsServer.h>
#include <App/IPGApplication.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateLoading, App::CStateHandler);

using namespace Data;

CAppStateLoading::CAppStateLoading(CStrID StateID): CStateHandler(StateID)
{
	//!!!tmp, need to revisit window management!
	LoadingScreen = n_new(UI::CLoadingScreen);
	LoadingScreen->Load("LoadingScreen.layout");

	NOT_IMPLEMENTED;
	//UISrv->RegisterScreen(CStrID("LoadingScreen"), LoadingScreen);
}
//---------------------------------------------------------------------

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

void CAppStateLoading::OnStateEnter(CStrID PrevState, PParams Params)
{
	TimeSrv->Trigger();

	GameSrv->PauseGame(true);

	NOT_IMPLEMENTED;
	//UISrv->SetRootScreen(LoadingScreen);
	//UISrv->ShowGUI();
	//UISrv->HideMouseCursor();

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
			PParams Desc = DataSrv->LoadPRM(FileName);
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
	NOT_IMPLEMENTED;
	//UISrv->HideGUI();
	//UISrv->ShowMouseCursor();
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

n_assert(false);
	//if (RenderSrv->BeginFrame())
	//{
	//	RenderSrv->Clear(Render::Clear_All, 0xff000000, 1.f, 0); 
	//	UISrv->Render();
	//	RenderSrv->EndFrame();
	//	RenderSrv->Present(); //!!!must be called as late as possible after EndFrame!
	//}

	CoreSrv->Trigger();

	//!!!process loading if sync, wait for loading thread end if async!
	IPGApp->FSM.RequestState(CStrID("Game")); //???!!!use CAppStateLoading::OnFrame return value!?

	return GetID();
}
//---------------------------------------------------------------------

} // namespace Application
