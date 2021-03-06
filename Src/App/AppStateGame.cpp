#include "AppStateGame.h"

#include <AI/AIServer.h>
#include <AI/PropActorBrain.h>
#include <AI/Movement/Actions/ActionGotoPosition.h>
#include <App/AppStates.h>
#include <Quests/QuestManager.h>
#include <Factions/FactionManager.h>
#include <Debug/DebugServer.h>
#include <UI/IngameScreen.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Frame/RenderPath.h>
#include <Frame/NodeAttrCamera.h>
#include <Resources/ResourceManager.h>
#include <Resources/Resource.h>
#include <Events/EventServer.h>
#include <Core/CoreServer.h>
#include <IO/IOServer.h>
#include <Game/GameServer.h>
#include <Game/GameLevel.h>
#include <Game/GameLevelView.h>
#include <Scene/Events/SetTransform.h>
#include <Scene/SceneNode.h>
#include <Scene/NodeControllerThirdPerson.h>
#include <UI/PropUIControl.h>
//#include <Audio/AudioServer.h>
#include <Physics/PhysicsServer.h>
#include <Video/VideoServer.h>
#include <Input/InputTranslator.h>
#include <Data/DataArray.h>
#include <IO/PathUtils.h>
#include <App/IPGApplication.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateGame, App::CStateHandler);

CAppStateGame::CAppStateGame(CStrID StateID):
	CStateHandler(StateID),
	hMainLevelView(INVALID_HANDLE),
    RenderDbgAI(false),
    RenderDbgPhysics(false),
    RenderDbgGfx(false),
    RenderDbgEntities(false)
{
	PROFILER_INIT(profCompleteFrame, "CompleteFrame");
	PROFILER_INIT(profRender, "Render_FrameTime");
}
//---------------------------------------------------------------------

void CAppStateGame::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	CoreSrv->Trigger();

	RenderDbgPhysics = false;
	RenderDbgGfx = false;

	IPGApp->pInputTranslator->EnableContext(CStrID("Game"));

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

	//!!!???only if game was (re)started!?
	// Load views, create additional windows if required, load cameras

	// If there are no views for any level:
	if (IPGApp->GPU->SwapChainExists(IPGApp->MainSwapChainIndex))
	{
		CStrID PartyLeaderID = FactionMgr->GetFaction(CStrID("Party"))->GetLeader();
		Game::CGameLevel* pActiveLevel = GameSrv->GetEntityMgr()->GetEntity(PartyLeaderID)->GetLevel();
		hMainLevelView = GameSrv->CreateLevelView(pActiveLevel->GetID());
		Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);

		if (pView)
		{
			Ptr<UI::CIngameScreen> IngameScreen = n_new(UI::CIngameScreen);
			IngameScreen->Load("IngameScreen.layout");
			IngameScreen->SetView(pView);

			IPGApp->MainUIContext->SetRootWindow(IngameScreen);
			IPGApp->MainUIContext->ShowGUI();
			IPGApp->MainUIContext->ShowMouseCursor();
			IPGApp->MainUIContext->SetDefaultMouseCursor("TaharezLook/MouseArrow");
			IPGApp->MainUIContext->SubscribeOnInput(IPGApp->MainWindow.GetUnsafe(), 100);

			Render::PRenderTarget MainRT = IPGApp->GPU->GetSwapChainRenderTarget(IPGApp->MainSwapChainIndex);

			const Render::CRenderTargetDesc& RealBackBufDesc = MainRT->GetDesc();

			Render::CRenderTargetDesc DSDesc;
			DSDesc.Format = Render::PixelFmt_DefaultDepthBuffer;
			DSDesc.MSAAQuality = Render::MSAA_None;
			DSDesc.UseAsShaderInput = false;
			DSDesc.MipLevels = 0;
			DSDesc.Width = RealBackBufDesc.Width;
			DSDesc.Height = RealBackBufDesc.Height;

			Render::PDepthStencilBuffer MainDS = IPGApp->GPU->CreateDepthStencilBuffer(DSDesc);

			Frame::CView& FrameView = pView->GetFrameView();
			FrameView.GPU = IPGApp->GPU;
			FrameView.SetRenderPath(RRP->GetObject<Frame::CRenderPath>());
			FrameView.RTs[0] = MainRT;
			FrameView.DSBuffers[0] = MainDS;
			FrameView.UIContext = IPGApp->MainUIContext;

			//!!!create default camera for that level!
			//!!!if no default camera defined, create app-default camera looking at the character selected!

			//!!!DBG TMP!
			Scene::CSceneNode* pCameraNode = pActiveLevel->GetSceneRoot()->CreateChild(CStrID("_DefaultCamera"));
			Scene::PNodeControllerThirdPerson Ctlr = n_new(Scene::CNodeControllerThirdPerson);
			pCameraNode->SetController(Ctlr);
			Ctlr->Activate(true);
			//Ctlr->SetCOI(vector3(227.0f, 0.05f, 195.0f)); // Boxes only
			//Ctlr->SetCOI(vector3(223.0f, 0.05f, 189.0f)); // Character close
			//Ctlr->SetCOI(vector3(220.0f, 3.2f, 200.0f)); // All objects
			Ctlr->SetCOI(vector3(220.0f, 3.2f, 200.0f)); // All objects and sky
			//Ctlr->SetAngles(PI * 0.2f, PI * 0.3f); // All objects
			Ctlr->SetAngles(PI * 0.12f, PI * 0.15f); // All objects and sky
			Ctlr->SetDistance(10.f);
			//Ctlr->SetDistance(350.f);
			Frame::PNodeAttrCamera MainCamera = n_new(Frame::CNodeAttrCamera);
			pCameraNode->AddAttribute(*MainCamera);
			MainCamera->SetWidth((float)MainRT->GetDesc().Width);
			MainCamera->SetHeight((float)MainRT->GetDesc().Height);
			FrameView.SetCamera(MainCamera);
			pCameraNode->UpdateTransform(NULL, 0, true); // Set valid camera transform before updating/rending the first frame

			//!!!DBG TMP!
			pView->AddToSelection(CStrID("GG"));
		}
	}

			/*
	// Save camera state //!!!save in game server for all views in a common desc!
	if (CameraManager.IsValidPtr())
	{
		Data::PParams SGScene = n_new(Data::CParams);

		bool IsThirdPerson = CameraManager->IsCameraThirdPerson();
		n_assert(IsThirdPerson); // Until a first person camera is implemented

		Data::PParams CurrCameraDesc = n_new(Data::CParams);
		CurrCameraDesc->Set(CStrID("ThirdPerson"), IsThirdPerson);

		if (IsThirdPerson)
		{
			Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)CameraManager->GetCameraController();
			if (pCtlr)
			{
				CurrCameraDesc->Set(CStrID("MinVAngle"), n_rad2deg(pCtlr->GetVerticalAngleMin()));
				CurrCameraDesc->Set(CStrID("MaxVAngle"), n_rad2deg(pCtlr->GetVerticalAngleMax()));
				CurrCameraDesc->Set(CStrID("MinDistance"), pCtlr->GetDistanceMin());
				CurrCameraDesc->Set(CStrID("MaxDistance"), pCtlr->GetDistanceMax());
				CurrCameraDesc->Set(CStrID("COI"), pCtlr->GetCOI());
				CurrCameraDesc->Set(CStrID("HAngle"), n_rad2deg(pCtlr->GetAngles().Phi));
				CurrCameraDesc->Set(CStrID("VAngle"), n_rad2deg(pCtlr->GetAngles().Theta));
				CurrCameraDesc->Set(CStrID("Distance"), pCtlr->GetDistance());
			}
		}

		Data::PParams InitialScene;
		Data::PParams InitialCamera;
		if (pInitialDesc &&
			pInitialDesc->Get(InitialScene, CStrID("Scene")) &&
			InitialScene->Get(InitialCamera, CStrID("Camera")))
		{
			Data::PParams SGCamera = n_new(Data::CParams);
			InitialCamera->GetDiff(*SGCamera, *CurrCameraDesc);
			if (SGCamera->GetCount()) SGScene->Set(CStrID("Camera"), SGCamera);
		}
		else SGScene->Set(CStrID("Camera"), CurrCameraDesc);

		if (SGScene->GetCount()) OutDesc.Set(CStrID("Scene"), SGScene);
	}

	if (AutoAdjustCameraAspect && MainCamera.IsValidPtr() && EvID == CStrID("OnRenderDeviceReset"))
	{
		//MainCamera->SetWidth((float)RenderSrv->GetBackBufferWidth());
		//MainCamera->SetHeight((float)RenderSrv->GetBackBufferHeight());
	}
			*/

	if (IPGApp->GPU->BeginFrame())
	{
		IPGApp->GPU->Clear(Render::Clear_Color, vector4::Zero, 1.f, 0);
		IPGApp->GPU->EndFrame();
	}

	// Game
	SUBSCRIBE_PEVENT(OnWorldTransitionRequested, CAppStateGame, OnWorldTransitionRequested);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, QuickSave, CAppStateGame, OnQuickSave);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, QuickLoad, CAppStateGame, OnQuickLoad);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ToggleGamePause, CAppStateGame, OnToggleGamePause);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, QueueAction, CAppStateGame, OnQueueAction);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, DoAction, CAppStateGame, OnDoAction);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ContextMenu, CAppStateGame, OnContextMenu);

	// Camera
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, CameraRotateH, CAppStateGame, OnCameraRotateH);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, CameraRotateV, CAppStateGame, OnCameraRotateV);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, CameraZoom, CAppStateGame, OnCameraZoom);

	// Debugging
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ShowDebugConsole, CAppStateGame, OnShowDebugConsole);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ShowDebugWatcher, CAppStateGame, OnShowDebugWatcher);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ToggleRenderDbgAI, CAppStateGame, OnToggleRenderDbgAI);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ToggleRenderDbgPhysics, CAppStateGame, OnToggleRenderDbgPhysics);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ToggleRenderDbgGfx, CAppStateGame, OnToggleRenderDbgGfx);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, ToggleRenderDbgEntities, CAppStateGame, OnToggleRenderDbgEntities);
	DISP_SUBSCRIBE_PEVENT(IPGApp->pInputTranslator, TeleportSelected, CAppStateGame, OnTeleportSelected);
}
//---------------------------------------------------------------------

void CAppStateGame::OnStateLeave(CStrID NextState)
{
	UNSUBSCRIBE_EVENT(OnWorldTransitionRequested);
	UNSUBSCRIBE_EVENT(QuickSave);
	UNSUBSCRIBE_EVENT(QuickLoad);
	UNSUBSCRIBE_EVENT(ToggleGamePause);
	UNSUBSCRIBE_EVENT(QueueAction);
	UNSUBSCRIBE_EVENT(DoAction);
	UNSUBSCRIBE_EVENT(ContextMenu);

	UNSUBSCRIBE_EVENT(CameraRotateH);
	UNSUBSCRIBE_EVENT(CameraRotateV);
	UNSUBSCRIBE_EVENT(CameraZoom);

	UNSUBSCRIBE_EVENT(ShowDebugConsole);
	UNSUBSCRIBE_EVENT(ShowDebugWatcher);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgAI);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgPhysics);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgGfx);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgEntities);
	UNSUBSCRIBE_EVENT(TeleportSelected);

	GameSrv->DestroyLevelView(hMainLevelView);
	hMainLevelView = INVALID_HANDLE;

	IPGApp->pInputTranslator->DisableContext(CStrID("Game"));
}
//---------------------------------------------------------------------

CStrID CAppStateGame::OnFrame()
{
	PROFILER_START(profCompleteFrame);

	float FrameTime = (float)CoreSrv->GetFrameTime();

	CoreSrv->Trigger();

	//!!!can move or clone to render server!
	static UPTR FPSFrameCount = 0;
	static float FPSTimeAccum = 0.f;
	++FPSFrameCount;
	FPSTimeAccum += FrameTime;
	if (FPSTimeAccum > 0.5f)
	{
		CoreSrv->SetGlobal<float>(CString("FPS"), FPSFrameCount / FPSTimeAccum);
		FPSFrameCount = 0;
		FPSTimeAccum = 0.f;
	}

	EventSrv->ProcessPendingEvents();
	CoreSrv->SetGlobal<int>(CString("Events_FiredTotal"), (int)EventSrv->GetFiredEventsCount());

	DbgSrv->Trigger();
	UISrv->Trigger(FrameTime);
	IPGApp->pInputTranslator->Trigger(FrameTime);
	IPGApp->pInputTranslator->FireQueuedEvents();

	GameSrv->Trigger();

	//!!!gameplay managers, subscribe on GameSrv event!!!
	QuestMgr->Trigger();
	DlgMgr->Trigger();

	VideoSrv->Trigger();
//	AudioSrv->Trigger();

	//if (RenderDbgAI) GameSrv->GetActiveLevel()->GetAI()->RenderDebug();
	//if (RenderDbgEntities) GameSrv->GetActiveLevel()->RenderDebug();
	//???if (RenderDbgGfx) GameSrv->RenderCurrentLevelSceneDebug();?
	//???if (RenderDbgPhysics) GameSrv->RenderCurrentLevelPhysicsDebug();

	Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
	const Scene::CSceneNode* pCameraNode = pView->GetFrameView().GetCamera()->GetNode();
	Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCameraNode->GetController();
	if (pCtlr &&
		!IPGApp->pInputTranslator->CheckState(CStrID("CameraRotationOn")) &&
		!pView->GetFrameView().UIContext->IsMouseOverGUI())
	{
		float XRel, YRel;
		pView->GetFrameView().UIContext->GetCursorPositionRel(XRel, YRel);

		float CameraMoveX, CameraMoveZ;
		if (XRel < 0.05f) CameraMoveX = (XRel - 0.05f) / 0.05f;
		else if (XRel > 0.95f) CameraMoveX = (XRel - 0.95f) / 0.05f;
		else CameraMoveX = 0.f;
		if (YRel < 0.05f) CameraMoveZ = (YRel - 0.05f) / 0.05f;
		else if (YRel > 0.95f) CameraMoveZ = (YRel - 0.95f) / 0.05f;
		else CameraMoveZ = 0.f;

		const float CameraMoveSpeed = 0.2f;
		if (CameraMoveX != 0.f)
		{
			vector3 Axis = pCameraNode->GetWorldMatrix().AxisX();
			Axis.y = 0.f;
			Axis.norm();
			pCtlr->Move(Axis * CameraMoveX * CameraMoveSpeed);
		}
		if (CameraMoveZ != 0.f)
		{
			vector3 Axis = pCameraNode->GetWorldMatrix().AxisZ();
			Axis.y = 0.f;
			Axis.norm();
			pCtlr->Move(Axis * CameraMoveZ * CameraMoveSpeed);
		}
	}

	PROFILER_START(profRender);

	IPGApp->GPU->Present(IPGApp->MainSwapChainIndex);
	if (hMainLevelView != INVALID_HANDLE)
	{
		Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
		if (pView) pView->GetFrameView().Render();
		else hMainLevelView = INVALID_HANDLE;
	}

	PROFILER_STOP(profRender);

	PROFILER_STOP(profCompleteFrame);

	return GetID();
}
//---------------------------------------------------------------------

bool CAppStateGame::OnWorldTransitionRequested(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;

	Data::PDataArray IDs = P->Get<Data::PDataArray>(CStrID("EntityIDs"));
	if (!IDs->GetCount()) FAIL;

	RPG::CFaction* pParty = FactionMgr->GetFaction(CStrID("Party"));
	bool IsPartyTravel = false;
	for (UPTR i = 0; i < IDs->GetCount(); ++i)
	{
		Data::CData& Elm = IDs->Get(i);
		CStrID EntityID;
		if (Elm.IsA<CStrID>()) EntityID = Elm.GetValue<CStrID>();
		else
		{
			EntityID = CStrID(Elm.GetValue<CString>().CStr());
			Elm = EntityID;
		}
		if (!IsPartyTravel && pParty->IsMember(EntityID)) IsPartyTravel = true;
	}

	CStrID LevelID = GetStrID(*P, CStrID("LevelID"));
	bool IsFarTravel = P->Get<bool>(CStrID("IsFarTravel"));

	if (!IsFarTravel && GameSrv->IsLevelLoaded(LevelID))
	{
		CArray<CStrID> TravellerIDs(IDs->GetCount(), 0);
		for (UPTR i = 0; i < IDs->GetCount(); ++i)
			TravellerIDs.Add(IDs->Get<CStrID>(i));

		CStrID MarkerID = GetStrID(*P, CStrID("MarkerID"));

		if (WorldMgr->MakeTransition(TravellerIDs, LevelID, MarkerID, false) && IsPartyTravel)
		{
			NOT_IMPLEMENTED;
			/*
			Game::CGameLevel* pLevel = GameSrv->GetLevel(LevelID);
			pLevel->ClearSelection();
			RPG::CFaction* pParty = FactionMgr->GetFaction(CStrID("Party"));
			if (pParty)
			{
				for (UPTR i = 0; i < TravellerIDs.GetCount(); ++i)
				{
					CStrID TravellerID = TravellerIDs[i];
					if (pParty->IsMember(TravellerID))
						pLevel->AddToSelection(TravellerID); //!!!can add to selection only entities selected before transition!
				}
			}
			GameSrv->SetActiveLevel(LevelID);
			*/
		}
	}
	else
	{
		P->Set(CStrID("Request"), (int)App::Request_Transition);
		P->Set(CStrID("IsPartyTravel"), IsPartyTravel);
		IPGApp->FSM.RequestState(CStrID("Loading"), P);
	}

	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnQuickSave(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	int QuickSaveCount = GameSrv->GetGlobalAttr(CStrID("QuickSaveCount"), 3);
	int CurrQuickSave = GameSrv->GetGlobalAttr(CStrID("CurrQuickSave"), QuickSaveCount);
	CurrQuickSave = (CurrQuickSave >= QuickSaveCount) ? 1 : CurrQuickSave + 1;
	GameSrv->SetGlobalAttr(CStrID("CurrQuickSave"), CurrQuickSave);
	CString SaveName;
	SaveName.Format("QuickSave%03d", CurrQuickSave);
	GameSrv->SaveGame(SaveName); //???can save at any point of the frame?
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnQuickLoad(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	int CurrQuickSave;
	if (!GameSrv->GetGlobalAttr(CurrQuickSave, CStrID("CurrQuickSave"))) OK;
	CString SaveName;
	SaveName.Format("QuickSave%03d", CurrQuickSave);

	if (GameSrv->SavedGameExists(SaveName))
	{
		Data::PParams P = n_new(Data::CParams(2));
		P->Set(CStrID("Request"), (int)Request_LoadGame);
		P->Set(CStrID("SavedGameName"), SaveName);
		IPGApp->FSM.RequestState(CStrID("Loading"), P);
	}

	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnToggleGamePause(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	GameSrv->ToggleGamePause();
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::IssueActorCommand(bool Run, bool ClearQueue)
{
	Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
	if (!pView) FAIL;

	CStrID TargetUID = pView->GetEntityUnderMouseUID();

	Game::CEntity* pTargetEntity = GameSrv->GetEntityMgr()->GetEntity(TargetUID);
	Prop::CPropUIControl* pCtl = pTargetEntity ? pTargetEntity->GetProperty<Prop::CPropUIControl>() : NULL;

	CStrID ActorID = FactionMgr->GetFaction(CStrID("Party"))->GetGroupLeader(pView->GetSelection());
	Game::CEntity* pActorEntity = GameSrv->GetEntityMgr()->GetEntity(ActorID);
	Prop::CPropActorBrain* pActor = pActorEntity ? pActorEntity->GetProperty<Prop::CPropActorBrain>() : NULL;

	//!!!if group selected, clear queues of all members!
	if (ClearQueue && pActor) pActor->ClearTaskQueue();

	if (pCtl)
	{
		// NB: Pure UI actions, like Select and Explore, don't require an actor, so we allow NULL actor here
		pCtl->ExecuteDefaultAction(pActorEntity); //???get default action, check, for pure UI don't clear queue?
	}
	else
	{
		//!!!TMP! use formation to issue goto commands for all the group
		if (!pActorEntity) FAIL;

		AI::PActionGotoPosition Action = n_new(AI::CActionGotoPosition);
		Action->Init(pView->GetMousePos3D());
		//!!!MvmtType = Run ? AI::AIMvmt_Type_Run : AI::AIMvmt_Type_Walk;

		AI::CTask Task;
		Task.Plan = Action;
		Task.Relevance = AI::Relevance_Absolute;
		Task.FailOnInterruption = false;
		Task.ClearQueueOnFailure = true;
		pActor->EnqueueTask(Task);
	}

	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnDoAction(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	return IssueActorCommand(false, true);
}
//---------------------------------------------------------------------

bool CAppStateGame::OnQueueAction(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	return IssueActorCommand(false, false);
}
//---------------------------------------------------------------------

bool CAppStateGame::OnContextMenu(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
	if (!pView) FAIL;

	CStrID ActorID = FactionMgr->GetFaction(CStrID("Party"))->GetGroupLeader(pView->GetSelection());
	Game::CEntity* pActorEntity = GameSrv->GetEntityMgr()->GetEntity(ActorID);
	if (!pActorEntity) FAIL;

	CStrID TargetUID = pView->GetEntityUnderMouseUID();
	Game::CEntity* pTargetEntity = GameSrv->GetEntityMgr()->GetEntity(TargetUID);
	if (!pTargetEntity) FAIL;
	Prop::CPropUIControl* pCtl = pTargetEntity->GetProperty<Prop::CPropUIControl>();
	if (!pCtl) FAIL;
	pCtl->ShowPopup(pActorEntity);

	OK;
}
//---------------------------------------------------------------------

//!!!hold cursor while rotating or remember on start and then restor its position!
bool CAppStateGame::OnCameraRotateH(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
	if (!pView || !pView->GetFrameView().GetCamera()) FAIL;

	const Frame::CNodeAttrCamera* pCamera = pView->GetFrameView().GetCamera();
	Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCamera->GetNode()->GetController();
	if (!pCtlr) FAIL;

	Data::PParams P = ((const Events::CEvent&)Event).Params;
	float AmountRel = P->Get<float>(CStrID("AmountRel"));

	static const float CameraOrbitSpeed = 1.5f;
	pCtlr->OrbitHorizontal(AmountRel * CameraOrbitSpeed);

	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnCameraRotateV(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
	if (!pView || !pView->GetFrameView().GetCamera()) FAIL;

	const Frame::CNodeAttrCamera* pCamera = pView->GetFrameView().GetCamera();
	Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCamera->GetNode()->GetController();
	if (!pCtlr) FAIL;

	Data::PParams P = ((const Events::CEvent&)Event).Params;
	float AmountRel = P->Get<float>(CStrID("AmountRel"));

	static const float CameraOrbitSpeed = 1.5f;
	pCtlr->OrbitVertical(AmountRel * CameraOrbitSpeed);

	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnCameraZoom(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Game::CGameLevelView* pView = GameSrv->GetLevelView(hMainLevelView);
	if (!pView || !pView->GetFrameView().GetCamera()) FAIL;

	const Frame::CNodeAttrCamera* pCamera = pView->GetFrameView().GetCamera();
	Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCamera->GetNode()->GetController();
	if (!pCtlr) FAIL;

	Data::PParams P = ((const Events::CEvent&)Event).Params;
	float AmountRel = P->Get<float>(CStrID("AmountRel"));

	static const float CameraZoomSpeed = 0.5f;
	pCtlr->Zoom(-AmountRel * CameraZoomSpeed);

	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnShowDebugConsole(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	DbgSrv->TogglePluginWindow(CStrID("Console"));
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnShowDebugWatcher(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	DbgSrv->TogglePluginWindow(CStrID("Watcher"));
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnToggleRenderDbgAI(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	RenderDbgAI = !RenderDbgAI;
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnToggleRenderDbgPhysics(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	RenderDbgPhysics = !RenderDbgPhysics;
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnToggleRenderDbgGfx(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	RenderDbgGfx = !RenderDbgGfx;
n_assert(false);
//	RenderSrv->SetWireframe(RenderDbgGfx);
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnToggleRenderDbgEntities(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	RenderDbgEntities = !RenderDbgEntities;
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnTeleportSelected(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	NOT_IMPLEMENTED;
	//if (!GameSrv->GetActiveLevel()->GetSelectedCount()) OK;

	//CStrID ID =	GameSrv->GetActiveLevel()->GetSelection()[0];
	//Game::CEntity* pEnt = GameSrv->GetEntityMgr()->GetEntity(ID);
	//if (!pEnt) OK;

	//matrix44 Tfm = pEnt->GetAttr<matrix44>(CStrID("Transform"));
	//Tfm.Translation() = GameSrv->GetMousePos3D();
	//pEnt->FireEvent(Event::SetTransform(Tfm));

	OK;
}
//---------------------------------------------------------------------

}