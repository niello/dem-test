#include "IPGApplication.h"

#include "AppStateMenu.h"
#include "AppStateLoading.h"
#include "AppStateGame.h"
#include <App/Environment.h>
#include <Game/EntityLoaderCommon.h>
#include <Game/EntityLoaderStatic.h>
#include <Scene/PropSceneNode.h> //???!!!move all props from Prop:: to Game::?
#include <AI/PropSmartObject.h>
#include <AI/PropAIHints.h>
#include <AI/PropActorBrain.h>
#include <Scripting/PropScriptable.h>
#include <Animation/PropAnimation.h>
#include <UI/PropUIControl.h>
#include <Physics/PropPhysics.h>
#include <Physics/PropCharacterController.h>
#include <Dlg/PropTalking.h>
#include <Items/Prop/PropEquipment.h>
#include <Items/Prop/PropItem.h>
#include <IO/IOServer.h>
#include <SI/SI_L1.h>
#include <SI/SI_L2.h>
#include <SI/SI_L3.h>
#include <time.h> //???!!!wrap needed func in Time::?

#undef DeleteFile

namespace App
{
__ImplementSingleton(App::CIPGApplication);

CIPGApplication::CIPGApplication()
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CIPGApplication::~CIPGApplication()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

void CIPGApplication::SetupDisplayMode()
{
	CDisplayMode Mode;
	Mode.PosX = 0;
	Mode.PosY = 0;
	Mode.Width = 1024;
	Mode.Height = 640;
	Mode.PixelFormat = D3DFMT_X8R8G8B8;
	AppEnv->SetDisplayMode(Mode);

	CString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();
	AppEnv->SetWindowTitle(WindowTitle.CStr());
	AppEnv->SetWindowIcon("Icon");
}
//---------------------------------------------------------------------

bool CIPGApplication::Open()
{
	srand((UINT)time(NULL));

	AppEnv->SetAppName(GetAppName());
	AppEnv->SetAppVersion(GetAppVersion());
	AppEnv->SetVendorName(GetVendorName());

	// Old:
	//SetupFromDefaults();
	//SetupFromProfile();
	//SetupFromCmdLineArgs();

	if (!AppEnv->InitCore())
	{
		AppEnv->ReleaseCore();
		FAIL;
	}

	SetupDisplayMode();

	//???only add CFileSystemNPK here?
	IOSrv->MountNPK("Proj:Export.npk");

	if (!AppEnv->InitEngine())
	{
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}
	
	n_printf("AppEnv->InitEngine() - OK\n");

	SI::RegisterGlobals();
	SI::RegisterEventServer();
	SI::RegisterTimeServer();
	
	n_printf("Engine SI registration - OK\n");
	
	InputSrv->SetContextLayout(CStrID("Debug"), CStrID("Debug"));
	InputSrv->SetContextLayout(CStrID("Game"), CStrID("Game"));

	InputSrv->EnableContext(CStrID("Debug"));

	n_printf("Setup input - OK\n");

	//!!!to HRD params! data/cfg/UI.hrd
	UISrv->LoadFont("DejaVuSans-8.font");
	UISrv->LoadFont("DejaVuSans-10.font");
	UISrv->LoadFont("DejaVuSans-14.font");
	UISrv->LoadFont("CourierNew-10.font");
	UISrv->LoadScheme("TaharezLook.scheme");
	UISrv->SetDefaultMouseCursor("TaharezLook", "MouseArrow");

	n_printf("AppEnv->InitGameSystem() ...\n");

	if (!AppEnv->InitGameSystem())
	{
		AppEnv->ReleaseGameSystem();
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	n_printf("AppEnv->InitGameSystem() - OK\n");

	//!!!get from global settings!
	CString UserProfileName;
	if (UserProfileName.IsEmpty()) UserProfileName = "Default";
	CArray<CString> Profiles;
	GameSrv->EnumProfiles(Profiles);
	if (!Profiles.Contains(UserProfileName))
		GameSrv->CreateProfile(UserProfileName);
	GameSrv->SetCurrentProfile(UserProfileName);

	n_printf("Set user profile '%s' - OK\n", UserProfileName.CStr());

	GameSrv->SetEntityLoader(CStrID::Empty, n_new(Game::CEntityLoaderCommon));
	GameSrv->SetEntityLoader(CStrID("Static"), n_new(Game::CEntityLoaderStatic));

	EntityMgr->RegisterProperty<Prop::CPropSceneNode>(256);
	EntityMgr->RegisterProperty<Prop::CPropSmartObject>(64);
	EntityMgr->RegisterProperty<Prop::CPropUIControl>(64);
	EntityMgr->RegisterProperty<Prop::CPropAnimation>(128);
	EntityMgr->RegisterProperty<Prop::CPropPhysics>(128);
	EntityMgr->RegisterProperty<Prop::CPropScriptable>(64);
	EntityMgr->RegisterProperty<Prop::CPropAIHints>(64);
	EntityMgr->RegisterProperty<Prop::CPropActorBrain>(32);
	EntityMgr->RegisterProperty<Prop::CPropTalking>(32);
	//EntityMgr->RegisterProperty<Prop::CPropDestructible>(32);
	//EntityMgr->RegisterProperty<Prop::CPropWeapon>(32);
	EntityMgr->RegisterProperty<Prop::CPropInventory>(32);
	EntityMgr->RegisterProperty<Prop::CPropItem>(32);
	EntityMgr->RegisterProperty<Prop::CPropCharacterController>(16);

	n_printf("Setup entity loaders and register props - OK\n");

	SI::RegisterGameServer();
	SI::RegisterEntityManager();
	SI::RegisterScriptObjectSIEx();
	SI::RegisterNavMesh();

	// Initialize gameplay systems
	WorldManager = n_new(RPG::CWorldManager);
	QuestManager = n_new(Story::CQuestManager);
	DialogueManager = n_new(Story::CDialogueManager);
	ItemManager = n_new(Items::CItemManager);
	FactionManager = n_new(RPG::CFactionManager);

	n_printf("Setup gameplay systems - OK\n");

	SI::RegisterQuestSystem();
	SI::RegisterDlgSystem();
	SI::RegisterClassCFaction();
	
	n_printf("Setup L3 SI - OK\n");

	FSM.AddStateHandler(n_new(CAppStateMenu(CStrID("Menu"))));
	FSM.AddStateHandler(n_new(CAppStateLoading(CStrID("Loading"))));
	FSM.AddStateHandler(n_new(CAppStateGame(CStrID("Game"))));
	
	n_printf("Seting state: Menu...\n");
	
	FSM.Init(CStrID("Menu"));
	
	n_printf("State setup - OK\n");

	SUBSCRIBE_PEVENT(OnDisplayClose, CIPGApplication, OnDisplayClose);

	OK;
}
//---------------------------------------------------------------------

bool CIPGApplication::AdvanceFrame()
{
	return FSM.Advance();
}
//---------------------------------------------------------------------

void CIPGApplication::Close()
{
	UNSUBSCRIBE_EVENT(OnDisplayClose);

	FSM.Clear();

	GameSrv->ExitGame();

	WorldManager = NULL;
	DialogueManager = NULL;
	QuestManager = NULL;
	ItemManager = NULL;
	FactionManager = NULL;

	AppEnv->ReleaseGameSystem();
	AppEnv->ReleaseEngine();
	AppEnv->ReleaseCore();
}
//---------------------------------------------------------------------

bool CIPGApplication::OnDisplayClose(const Events::CEventBase& Event)
{
	FSM.RequestState(APP_STATE_EXIT);
	OK;
}
//---------------------------------------------------------------------

}