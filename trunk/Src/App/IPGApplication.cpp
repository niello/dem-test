#include "IPGApplication.h"

#include <System/OSWindow.h>
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
#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/SwapChain.h>
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

namespace App
{
__ImplementSingleton(App::CIPGApplication);

bool CIPGApplication::Open()
{
	srand((UINT)time(NULL));

	n_new(Core::CCoreServer);

	n_new(IO::CIOServer);

	if (!ProjDir.IsValid()) ProjDir = IOSrv->GetAssign("Home");
	IOSrv->SetAssign("Proj", ProjDir);

	CString AppData;
	AppData.Format("AppData:%s/%s", GetVendorName().CStr(), GetAppName().CStr());
	IOSrv->SetAssign("AppData", IOSrv->ManglePath(AppData));

	IOSrv->MountNPK("Proj:Export.npk"); //???only add CFileSystemNPK here?

	n_new(Data::CDataServer);

	Data::PParams PathList = DataSrv->LoadHRD("Proj:PathList.hrd", false);
	if (PathList.IsValid())
		for (int i = 0; i < PathList->GetCount(); ++i)
			IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ManglePath(PathList->Get<CString>(i)));

	if (!AppEnv->InitEngine())
	{
		Close();
		FAIL;
	}

	CString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();
	MainWindow = n_new(Sys::COSWindow);
	MainWindow->SetTitle(WindowTitle.CStr());
	MainWindow->SetIcon("Icon");
	MainWindow->SetRect(Data::CRect(50, 50, 800, 600));
	MainWindow->Open();

	VideoDrvFct = n_new(Render::CD3D11DriverFactory);
	Render::PGPUDriver GPU = VideoDrvFct->CreateGPUDriver(0, Render::GPU_Hardware);

	Render::CRenderTargetDesc BBDesc;
	BBDesc.Format = Render::PixelFmt_X8R8G8B8;
	BBDesc.MSAAQuality = Render::MSAA_None;
	BBDesc.UseAsShaderInput = false;
	BBDesc.Width = 0;
	BBDesc.Height = 0;

	Render::CSwapChainDesc SCDesc;
	SCDesc.BackBufferCount = 2;
	SCDesc.SwapMode = Render::SwapMode_CopyDiscard;
	SCDesc.Flags = Render::SwapChain_AutoAdjustSize | Render::SwapChain_VSync;

	DWORD SCIdx = GPU->CreateSwapChain(BBDesc, SCDesc, MainWindow);
	
	Sys::Log("AppEnv->InitEngine() - OK\n");

	SI::RegisterGlobals();
	SI::RegisterEventServer();
	SI::RegisterTimeServer();
	
	Sys::Log("Engine SI registration - OK\n");
	
	InputSrv->SetContextLayout(CStrID("Debug"), CStrID("Debug"));
	InputSrv->SetContextLayout(CStrID("Game"), CStrID("Game"));

	InputSrv->EnableContext(CStrID("Debug"));

	Sys::Log("Setup input - OK\n");

	//!!!to HRD params! data/cfg/UI.hrd
	if (UI::CUIServer::HasInstance())
	{
		UISrv->LoadFont("DejaVuSans-8.font");
		UISrv->LoadFont("DejaVuSans-10.font");
		UISrv->LoadFont("DejaVuSans-14.font");
		UISrv->LoadFont("CourierNew-10.font");
		UISrv->LoadScheme("TaharezLook.scheme");
		UISrv->SetDefaultMouseCursor("TaharezLook/MouseArrow");
	}

	Sys::Log("AppEnv->InitGameSystem() ...\n");

	if (!AppEnv->InitGameSystem())
	{
		Close();
		FAIL;
	}

	Sys::Log("AppEnv->InitGameSystem() - OK\n");

	//!!!get from global settings!
	CString UserProfileName;
	if (UserProfileName.IsEmpty()) UserProfileName = "Default";
	CArray<CString> Profiles;
	GameSrv->EnumProfiles(Profiles);
	if (!Profiles.Contains(UserProfileName))
		GameSrv->CreateProfile(UserProfileName);
	GameSrv->SetCurrentProfile(UserProfileName);

	Sys::Log("Set user profile '%s' - OK\n", UserProfileName.CStr());

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

	Sys::Log("Setup entity loaders and register props - OK\n");

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

	Sys::Log("Setup gameplay systems - OK\n");

	SI::RegisterQuestSystem();
	SI::RegisterDlgSystem();
	SI::RegisterClassCFaction();
	
	Sys::Log("Setup L3 SI - OK\n");

	FSM.AddStateHandler(n_new(CAppStateMenu(CStrID("Menu"))));
	//FSM.AddStateHandler(n_new(CAppStateLoading(CStrID("Loading"))));
	//FSM.AddStateHandler(n_new(CAppStateGame(CStrID("Game"))));
	
	Sys::Log("Seting state: Menu...\n");
	
	FSM.Init(CStrID("Menu"));
	
	Sys::Log("State setup - OK\n");

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
	n_delete(DataSrv);
	n_delete(IOSrv);
	n_delete(CoreSrv);
}
//---------------------------------------------------------------------

bool CIPGApplication::OnDisplayClose(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FSM.RequestState(CStrID::Empty);
	OK;
}
//---------------------------------------------------------------------

}