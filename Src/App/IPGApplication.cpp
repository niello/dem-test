#include "IPGApplication.h"

#include "AppStateMenu.h"
#include "AppStateLoading.h"
#include "AppStateGame.h"
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
#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/GPUDriver.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/SwapChain.h>
#include <Resources/ResourceManager.h>
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

	n_new(Resources::CResourceManager);
	//!!!register loaders!

	n_new(Data::CDataServer); //???need at all? can store DSS as rsrc!

	Data::PParams PathList = DataSrv->LoadHRD("Proj:PathList.hrd", false);
	if (PathList.IsValid())
		for (int i = 0; i < PathList->GetCount(); ++i)
			IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ManglePath(PathList->Get<CString>(i)));

	// Store reference just in case. It is a dispatcher and ma be assigned to a smart ptr somewhere.
	EventServer = n_new(Events::CEventServer);

	n_new(Time::CTimeServer);

	DebugServer = n_new(Debug::CDebugServer);
	DebugServer->RegisterPlugin(CStrID("Console"), "Debug::CLuaConsole", "Console.layout");
	DebugServer->RegisterPlugin(CStrID("Watcher"), "Debug::CWatcherWindow", "Watcher.layout");

	DD = n_new(Debug::CDebugDraw);
	if (!DD->Open())
	{
		Close();
		FAIL;
	}

	// Application window

	CString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();
	MainWindow = n_new(Sys::COSWindow);
	MainWindow->SetTitle(WindowTitle.CStr());
	MainWindow->SetIcon("Icon");
	MainWindow->SetRect(Data::CRect(50, 50, 800, 600));
	MainWindow->Open();

	DISP_SUBSCRIBE_PEVENT(MainWindow, OnClosing, CIPGApplication, OnOSWindowClosing);


	// Rendering

	const bool UseD3D9 = true;
	if (UseD3D9)
	{
		Render::PD3D9DriverFactory Fct = n_new(Render::CD3D9DriverFactory);
		Fct->Open(MainWindow);
		VideoDrvFct = Fct;
		//!!!register shader, mesh & texture loaders!
	}
	else
	{
		Render::PD3D11DriverFactory Fct = n_new(Render::CD3D11DriverFactory);
		Fct->Open();
		VideoDrvFct = Fct;
		//!!!register shader, mesh & texture loaders!
	}

	GPU = VideoDrvFct->CreateGPUDriver(Render::Adapter_Primary, Render::GPU_Hardware);

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

	int SCIdx = GPU->CreateSwapChain(BBDesc, SCDesc, MainWindow);
	n_assert(GPU->SwapChainExists(SCIdx));
	Render::PRenderTarget SCRT = GPU->GetSwapChainRenderTarget(SCIdx);

	//Render::CRenderTargetDesc DSDesc;
	//BBDesc.Format = Render::PixelFmt_X8R8G8B8;
	//BBDesc.MSAAQuality = Render::MSAA_None;
	//BBDesc.UseAsShaderInput = false;
	//BBDesc.Width = 0; //???from created RT?
	//BBDesc.Height = 0; //???from created RT?

	//Render::PDepthStencilBuffer DSBuf = GPU->CreateDepthStencilBuffer(DSDesc);

	//!!!set render target and ds surface!
	if (GPU->BeginFrame())
	{
		GPU->Clear(Render::Clear_Color, 0xffff0000, 1.f, 0);
		GPU->EndFrame();
		GPU->Present(SCIdx);
	}

///////////////////////
	//!!!need multiwindow!
	Wnd2 = n_new(Sys::COSWindow);
	Wnd2->SetTitle("Window 2");
	Wnd2->SetIcon("Icon");
	Wnd2->SetRect(Data::CRect(900, 50, 150, 200));
	Wnd2->Open();
///////////////////////

	//Render::PFrameShader DefaultFrameShader = n_new(Render::CFrameShader);
	//n_assert(DefaultFrameShader->Init(*DataSrv->LoadPRM("Shaders:Default.prm")));
	//RenderServer->AddFrameShader(CStrID("Default"), DefaultFrameShader);
	//RenderServer->SetScreenFrameShaderID(CStrID("Default"));

	InputServer = n_new(Input::CInputServer);
	InputServer->Open();

	VideoServer = n_new(Video::CVideoServer);
	VideoServer->Open();
	
	//UIServer = n_new(UI::CUIServer);
	//DbgSrv->AllowUI(true);

	n_new(Scripting::CScriptServer);
	if (!Scripting::CEntityScriptObject::RegisterClass())
	{
		Close();
		FAIL;
	}
	
	Sys::Log("InitEngine() - OK\n");

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

	Sys::Log("InitGameSystem() ...\n");

	PhysicsServer = n_new(Physics::CPhysicsServer);
	PhysicsServer->Open();

	GameServer = n_new(Game::CGameServer);
	GameServer->Open();

	AIServer = n_new(AI::CAIServer);

	// Actor action templates
	Data::PParams ActTpls = DataSrv->LoadPRM("AI:AIActionTpls.prm");
	if (ActTpls.IsValid())
	{
		for (int i = 0; i < ActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = ActTpls->Get(i);
			AISrv->GetPlanner().RegisterActionTpl(Prm.GetName().CStr(), Prm.GetValue<Data::PParams>());
		}
		AISrv->GetPlanner().EndActionTpls();
	}

	// Smart object action templates
	Data::PParams SOActTpls = DataSrv->LoadPRM("AI:AISOActionTpls.prm");
	if (SOActTpls.IsValid())
		for (int i = 0; i < SOActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = SOActTpls->Get(i);
			AISrv->AddSmartAction(Prm.GetName(), *Prm.GetValue<Data::PParams>());
		}

	Sys::Log("InitGameSystem() - OK\n");

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
	UNSUBSCRIBE_EVENT(OnClosing);

	FSM.Clear();

	GameSrv->ExitGame();

	WorldManager = NULL;
	DialogueManager = NULL;
	QuestManager = NULL;
	ItemManager = NULL;
	FactionManager = NULL;

	AIServer = NULL;
	
	GameServer->Close();
	GameServer = NULL;

	PhysicsServer->Close();
	PhysicsServer = NULL;

	DbgSrv->AllowUI(false);
	UIServer = NULL;

	if (VideoServer.IsValid() && VideoServer->IsOpen()) VideoServer->Close();
	VideoServer = NULL;

	//if (AudioServer.IsValid() && AudioServer->IsOpen()) AudioServer->Close();
	//AudioServer = NULL;

	DD->Close();
	DD = NULL;

	//if (RenderServer.IsValid() && RenderServer->IsOpen()) RenderServer->Close();
	//RenderServer = NULL;

	MainWindow->Close();
	MainWindow = NULL;

	if (InputServer.IsValid() && InputServer->IsOpen()) InputServer->Close();
	InputServer = NULL;

	//if (LoaderServer.IsValid() && LoaderServer->IsOpen()) LoaderServer->Close();
	//LoaderServer = NULL;

	//DBServer = NULL;
	DebugServer = NULL;

	n_delete(TimeSrv);
	n_delete(ScriptSrv);

	EventServer = NULL;

	n_delete(DataSrv);
	n_delete(ResourceMgr);
	n_delete(IOSrv);
	n_delete(CoreSrv);
}
//---------------------------------------------------------------------

bool CIPGApplication::OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FSM.RequestState(CStrID::Empty);
	OK;
}
//---------------------------------------------------------------------

}