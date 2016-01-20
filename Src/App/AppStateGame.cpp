#include "AppStateGame.h"

#include <AI/AIServer.h>
#include <AI/PropActorBrain.h>
#include <AI/Movement/Actions/ActionGotoPosition.h>
#include <App/AppStates.h>
#include <Quests/QuestManager.h>
#include <Factions/FactionManager.h>
#include <Debug/DebugServer.h>
#include <UI/UIContext.h>
#include <UI/IngameScreen.h>
#include <Events/EventServer.h>
#include <Time/TimeServer.h>
#include <IO/IOServer.h>
#include <Game/GameServer.h>
#include <Game/GameLevel.h>
#include <Scene/Events/SetTransform.h>
#include <UI/PropUIControl.h>
//#include <Audio/AudioServer.h>
#include <Physics/PhysicsServer.h>
#include <Video/VideoServer.h>
#include <Input/InputServer.h>
#include <Input/Events/MouseMoveRaw.h>
#include <Input/Events/MouseWheel.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseDoubleClick.h>
#include <Data/DataArray.h>
#include <App/IPGApplication.h>

namespace App
{
__ImplementClassNoFactory(App::CAppStateGame, App::CStateHandler);

CAppStateGame::CAppStateGame(CStrID StateID):
	CStateHandler(StateID),
    RenderDbgAI(false),
    RenderDbgPhysics(false),
    RenderDbgGfx(false),
    RenderDbgEntities(false),
	CameraMoveX(0.f),
	CameraMoveZ(0.f),
	CameraRotate(false)
{
	PROFILER_INIT(profCompleteFrame, "CompleteFrame");
	PROFILER_INIT(profRender, "Render_FrameTime");
}
//---------------------------------------------------------------------

void CAppStateGame::OnStateEnter(CStrID PrevState, Data::PParams Params)
{
	TimeSrv->Trigger();

	RenderDbgPhysics = false;
	RenderDbgGfx = false;

	InputSrv->EnableContext(CStrID("Game"));

	// Here we load HUD
	if (IngameScreen.IsNullPtr())
	{
		IngameScreen = n_new(UI::CIngameScreen);
		IngameScreen->Load("IngameScreen.layout");
	}
	IPGApp->MainUIContext->SetRootWindow(IngameScreen);
	IPGApp->MainUIContext->ShowGUI();

n_assert(false);
	//if (RenderSrv->BeginFrame())
	//{
	//	RenderSrv->Clear(Render::Clear_Color, 0xff000000, 1.f, 0);
	//	RenderSrv->EndFrame();
	//}

	SUBSCRIBE_INPUT_EVENT(MouseMoveRaw, CAppStateGame, OnMouseMoveRaw, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseWheel, CAppStateGame, OnMouseWheel, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CAppStateGame, OnMouseBtnDown, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CAppStateGame, OnMouseBtnUp, Input::InputPriority_Raw);
	SUBSCRIBE_INPUT_EVENT(MouseDoubleClick, CAppStateGame, OnMouseDoubleClick, Input::InputPriority_Raw);
	SUBSCRIBE_PEVENT(OnWorldTransitionRequested, CAppStateGame, OnWorldTransitionRequested);
	SUBSCRIBE_PEVENT(QuickSave, CAppStateGame, OnQuickSave);
	SUBSCRIBE_PEVENT(QuickLoad, CAppStateGame, OnQuickLoad);
	SUBSCRIBE_PEVENT(ToggleGamePause, CAppStateGame, OnToggleGamePause);
	SUBSCRIBE_PEVENT(ToggleRenderDbgAI, CAppStateGame, OnToggleRenderDbgAI);
	SUBSCRIBE_PEVENT(ToggleRenderDbgPhysics, CAppStateGame, OnToggleRenderDbgPhysics);
	SUBSCRIBE_PEVENT(ToggleRenderDbgGfx, CAppStateGame, OnToggleRenderDbgGfx);
	SUBSCRIBE_PEVENT(ToggleRenderDbgEntities, CAppStateGame, OnToggleRenderDbgEntities);
	SUBSCRIBE_PEVENT(TeleportSelected, CAppStateGame, OnTeleportSelected);
}
//---------------------------------------------------------------------

void CAppStateGame::OnStateLeave(CStrID NextState)
{
	UNSUBSCRIBE_EVENT(MouseMoveRaw);
	UNSUBSCRIBE_EVENT(MouseWheel);
	UNSUBSCRIBE_EVENT(MouseBtnDown);
	UNSUBSCRIBE_EVENT(MouseBtnUp);
	UNSUBSCRIBE_EVENT(MouseDoubleClick);
	UNSUBSCRIBE_EVENT(OnWorldTransitionRequested);
	UNSUBSCRIBE_EVENT(QuickSave);
	UNSUBSCRIBE_EVENT(QuickLoad);
	UNSUBSCRIBE_EVENT(ToggleGamePause);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgAI);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgPhysics);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgGfx);
	UNSUBSCRIBE_EVENT(ToggleRenderDbgEntities);
	UNSUBSCRIBE_EVENT(TeleportSelected);

	InputSrv->DisableContext(CStrID("Game"));
}
//---------------------------------------------------------------------

CStrID CAppStateGame::OnFrame()
{
	PROFILER_START(profCompleteFrame);

	TimeSrv->Trigger();
	EventSrv->ProcessPendingEvents();
	InputSrv->Trigger();
//	RenderSrv->GetDisplay().ProcessWindowMessages();
	DbgSrv->Trigger();
	UISrv->Trigger((float)TimeSrv->GetFrameTime());

	GameSrv->Trigger();

	//!!!gameplay managers, subscribe on GameSrv event!!!
	QuestMgr->Trigger();
	DlgMgr->Trigger();

	VideoSrv->Trigger();
//	AudioSrv->Trigger();

	if (GameSrv->GetActiveLevel())
	{
		if (RenderDbgAI) GameSrv->GetActiveLevel()->GetAI()->RenderDebug();
		if (RenderDbgEntities) GameSrv->GetActiveLevel()->RenderDebug();
		//???if (RenderDbgGfx) GameSrv->RenderCurrentLevelSceneDebug();?
		//???if (RenderDbgPhysics) GameSrv->RenderCurrentLevelPhysicsDebug();

		//???to the camera manager? send CameraMoveX and CameraMoveZ there. Can send vector3 CameraMove.
		Scene::CCameraManager* pCamMgr = GameSrv->GetActiveLevel()->GetCameraMgr();
		if (pCamMgr)
		{
			Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCamMgr->GetCameraController();
			Scene::CSceneNode* pCamNode = pCamMgr->GetCameraNode();
			if (pCtlr && pCamNode)
			{
				if (CameraMoveX != 0.f)
				{
					vector3 Axis = pCamNode->GetWorldMatrix().AxisX();
					Axis.y = 0.f;
					Axis.norm();
					pCtlr->Move(Axis * pCamMgr->MoveSpeed * CameraMoveX);
				}
				if (CameraMoveZ != 0.f)
				{
					vector3 Axis = pCamNode->GetWorldMatrix().AxisZ();
					Axis.y = 0.f;
					Axis.norm();
					pCtlr->Move(Axis * pCamMgr->MoveSpeed * CameraMoveZ);
				}
			}
		}
	}

	PROFILER_START(profRender);

n_assert(false);
	//RenderSrv->Present(); // Must be called as late as possible after EndFrame
	//if (RenderSrv->BeginFrame())
	//{
	//	if (GameSrv->GetActiveLevel()) GameSrv->GetActiveLevel()->RenderScene();
	//	RenderSrv->EndFrame();
	//}

	PROFILER_STOP(profRender);

	CoreSrv->Trigger();

	//!!!can move or clone to render server!
	static DWORD FPSFrameCount = 0;
	static float FPSTimeAccum = 0.f;
	++FPSFrameCount;
	FPSTimeAccum += (float)TimeSrv->GetFrameTime();
	if (FPSTimeAccum > 0.5f)
	{
		CoreSrv->SetGlobal<float>(CString("FPS"), FPSFrameCount / FPSTimeAccum);
		FPSFrameCount = 0;
		FPSTimeAccum = 0.f;
	}

	CoreSrv->SetGlobal<int>(CString("Events_FiredTotal"), (int)EventSrv->GetFiredEventsCount());

	PROFILER_STOP(profCompleteFrame);

	return GetID();
}
//---------------------------------------------------------------------

bool CAppStateGame::IssueActorCommand(bool Run, bool ClearQueue)
{
	Game::CEntity* pTargetEntity = GameSrv->GetEntityUnderMouse();
	Prop::CPropUIControl* pCtl = pTargetEntity ? pTargetEntity->GetProperty<Prop::CPropUIControl>() : NULL;

	CStrID ID = FactionMgr->GetFaction(CStrID("Party"))->GetGroupLeader(GameSrv->GetActiveLevel()->GetSelection());
	Game::CEntity* pActorEntity = EntityMgr->GetEntity(ID);
	Prop::CPropActorBrain* pActor = pActorEntity ? pActorEntity->GetProperty<Prop::CPropActorBrain>() : NULL;

	//!!!if group selected, clear queues of all members!
	if (ClearQueue) pActor->ClearTaskQueue();

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
		Action->Init(GameSrv->GetMousePos3D());
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

bool CAppStateGame::OnMouseMoveRaw(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Scene::CCameraManager* pCamMgr = GameSrv->GetActiveLevel()->GetCameraMgr();
	if (!pCamMgr || !pCamMgr->IsCameraThirdPerson()) FAIL;

	Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCamMgr->GetCameraController();
	if (!pCtlr) FAIL;

	const Event::MouseMoveRaw& Ev = (const Event::MouseMoveRaw&)Event;

	if (CameraRotate)
	{
		//???use FrameTime * AngularSpeed * UnitDirection?
		pCtlr->OrbitHorizontal(Ev.X * pCamMgr->Sensitivity);
		pCtlr->OrbitVertical(Ev.Y * pCamMgr->Sensitivity);
	}
	else if (pCamMgr->GetCameraNode())
	{
		NOT_IMPLEMENTED;
		//if (UISrv->IsMouseOverGUI())
		//{
		//	CameraMoveX = 0.f;
		//	CameraMoveZ = 0.f;
		//}
		//else
		{
			float XRel, YRel;
			InputSrv->GetMousePosRel(XRel, YRel);
			if (XRel < 0.05f) CameraMoveX = (XRel - 0.05f) / 0.05f;
			else if (XRel > 0.95f) CameraMoveX = (XRel - 0.95f) / 0.05f;
			else CameraMoveX = 0.f;
			if (YRel < 0.05f) CameraMoveZ = (YRel - 0.05f) / 0.05f;
			else if (YRel > 0.95f) CameraMoveZ = (YRel - 0.95f) / 0.05f;
			else CameraMoveZ = 0.f;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnMouseWheel(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Scene::CCameraManager* pCamMgr = GameSrv->GetActiveLevel()->GetCameraMgr();
	if (!pCamMgr || !pCamMgr->IsCameraThirdPerson()) FAIL;
	Scene::CNodeControllerThirdPerson* pCtlr = (Scene::CNodeControllerThirdPerson*)pCamMgr->GetCameraController();
	if (!pCtlr) FAIL;
	pCtlr->Zoom((float)(-((const Event::MouseWheel&)Event).Delta) * pCamMgr->ZoomSpeed);
	OK;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnMouseBtnDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Input::EMouseButton Button = ((const Event::MouseBtnDown&)Event).Button;
	switch (Button)
	{
		case Input::MBLeft:
		{
			//???how to handle double-click without issuing an action twice, first walking and then running?
			//!!!don't resend action each tick, but once at near 1/4 sec can update task if mouse is pressed,
			//for smooth movement. Formation may support it badly!
			bool ShiftPressed =
				InputSrv->CheckKeyState(Input::LeftShift, KEY_IS_PRESSED) ||
				InputSrv->CheckKeyState(Input::RightShift, KEY_IS_PRESSED);
			return IssueActorCommand(false, !ShiftPressed);
		}
		case Input::MBMiddle:
		{
			CameraRotate = true;
			OK;
		}
		case Input::MBRight:
		{
			CStrID ID = FactionMgr->GetFaction(CStrID("Party"))->GetGroupLeader(GameSrv->GetActiveLevel()->GetSelection());
			Game::CEntity* pActorEntity = EntityMgr->GetEntity(ID);
			if (!pActorEntity) FAIL;
			Game::CEntity* pTargetEntity = GameSrv->GetEntityUnderMouse();
			if (!pTargetEntity) FAIL;
			Prop::CPropUIControl* pCtl = pTargetEntity->GetProperty<Prop::CPropUIControl>();
			if (!pCtl) FAIL;
			pCtl->ShowPopup(pActorEntity);
			OK;
		}
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnMouseBtnUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Input::EMouseButton Button = ((const Event::MouseBtnDown&)Event).Button;
	if (Button == Input::MBMiddle)
	{
		CameraRotate = false;
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CAppStateGame::OnMouseDoubleClick(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Input::EMouseButton Button = ((const Event::MouseBtnDown&)Event).Button;
	if (Button == Input::MBLeft)
	{
		bool ShiftPressed =
			InputSrv->CheckKeyState(Input::LeftShift, KEY_IS_PRESSED) ||
			InputSrv->CheckKeyState(Input::RightShift, KEY_IS_PRESSED);
		return IssueActorCommand(true, !ShiftPressed);
	}
	FAIL;
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
			Game::CGameLevel* pLevel = GameSrv->GetLevel(LevelID);
			pLevel->ClearSelection();
			for (UPTR i = 0; i < TravellerIDs.GetCount(); ++i) //???test and add only patry members to selection?
				pLevel->AddToSelection(TravellerIDs[i]); //!!!can add to selection only entities selected before transition!
			GameSrv->SetActiveLevel(LevelID);
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
	if (!GameSrv->GetActiveLevel()->GetSelectedCount()) OK;

	CStrID ID =	GameSrv->GetActiveLevel()->GetSelection()[0];
	Game::CEntity* pEnt = EntityMgr->GetEntity(ID);
	if (!pEnt) OK;

	matrix44 Tfm = pEnt->GetAttr<matrix44>(CStrID("Transform"));
	Tfm.Translation() = GameSrv->GetMousePos3D();
	pEnt->FireEvent(Event::SetTransform(Tfm));

	OK;
}
//---------------------------------------------------------------------

}