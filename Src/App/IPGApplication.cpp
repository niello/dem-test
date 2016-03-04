#include "IPGApplication.h"

#include <Game/EntityLoaderCommon.h>
#include <Game/EntityLoaderStatic.h>
#include <Scene/PropSceneNode.h> //???!!!move all props from Prop:: to Game::?
#include <AI/PropSmartObject.h>
#include <AI/PropAIHints.h>
#include <AI/PropActorBrain.h>
#include <Scripting/PropScriptable.h>
#include <Animation/PropAnimation.h>
#include <UI/PropUIControl.h>
#include <Render/GPUDriver.h>
#include <Render/Shader.h>
#include <Render/Texture.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/SwapChain.h>
#include <Render/RenderStateDesc.h>
#include <Render/Mesh.h>
#include <Render/MeshLoaderNVX2.h>
#include <Render/SkinInfo.h>
#include <Render/SkinInfoLoaderSKN.h>
#include <Animation/KeyframeClip.h>
#include <Animation/KeyframeClipLoaderKFA.h>
#include <Animation/MocapClip.h>
#include <Animation/MocapClipLoaderNAX2.h>
#include <Frame/RenderPath.h>
#include <Frame/RenderPathLoader.h>
#include <Physics/CollisionShapeLoader.h>
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

#include "AppStateMenu.h"
#include "AppStateLoading.h"
#include "AppStateGame.h"
#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11ShaderLoaders.h>
#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9ShaderLoaders.h>
#include <System/OSWindowClass.h>

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

	//!!!DBG TMP!
	//Check refcount
	Data::CData Data = Data::PParams(n_new(Data::CParams));

	CString AppData;
	AppData.Format("AppData:%s/%s", GetVendorName(), GetAppName());
	IOSrv->SetAssign("AppData", IOSrv->ResolveAssigns(AppData));

	IOSrv->MountNPK("Proj:Export.npk"); //???only add CFileSystemNPK here?

	n_new(Resources::CResourceManager);
	//!!!register loaders!

	n_new(Data::CDataServer); //???need at all? can store DSS as rsrc!

	Data::PParams PathList = DataSrv->LoadHRD("Proj:PathList.hrd", false);
	if (PathList.IsValidPtr())
		for (UPTR i = 0; i < PathList->GetCount(); ++i)
			IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));

	// Store reference just in case. It is a dispatcher and may be assigned to a smart ptr somewhere.
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

	EngineWindowClass = n_new(Sys::COSWindowClass);
	n_assert(EngineWindowClass->Create("DeusExMachina::MainWindow", "Icon"));

	CString WindowTitle = CString(GetVendorName()) + " - " + GetAppName() + " - " + GetAppVersion();
	MainWindow = n_new(Sys::COSWindow);
	MainWindow->SetWindowClass(*(Sys::COSWindowClassWin32*)EngineWindowClass.GetUnsafe()); //!!!bad design!
	MainWindow->SetTitle(WindowTitle.CStr());
	MainWindow->SetRect(Data::CRect(50, 50, 1024, 768));
	MainWindow->Open();

	DISP_SUBSCRIBE_PEVENT(MainWindow, OnClosing, CIPGApplication, OnOSWindowClosing);

///////////////////////
//!!!DBG TMP!
	Wnd2 = n_new(Sys::COSWindow);
	Wnd2->SetWindowClass(*(Sys::COSWindowClassWin32*)EngineWindowClass.GetUnsafe()); //!!!bad design!
	Wnd2->SetTitle("Window 2");
	Wnd2->SetRect(Data::CRect(1100, 50, 150, 200));
	Wnd2->Open();
	SCIdx2 = -1;
///////////////////////

	// Rendering

	const bool UseD3D9 = false;
	const char* pCEGUIVS;
	const char* pCEGUIPS;
	if (UseD3D9)
	{
		Render::PD3D9DriverFactory Fct = n_new(Render::CD3D9DriverFactory);
		Fct->Open(MainWindow);
		VideoDrvFct = Fct;

		//!!!GPU intentionally not set in loader for testing!
		Resources::PD3D9ShaderLoader ShaderLoader = n_new(Resources::CD3D9ShaderLoader);
		ResourceMgr->RegisterDefaultLoader("vsh", &Render::CShader::RTTI, ShaderLoader.GetUnsafe());
		ResourceMgr->RegisterDefaultLoader("psh", &Render::CShader::RTTI, ShaderLoader.GetUnsafe());

		pCEGUIVS = "Shaders:Bin/1.vsh";
		pCEGUIPS = "Shaders:Bin/2.psh";
	}
	else
	{
		Render::PD3D11DriverFactory Fct = n_new(Render::CD3D11DriverFactory);
		Fct->Open();
		VideoDrvFct = Fct;

		//!!!GPU intentionally not set in loader for testing!
		//???rsrc storage - not singleton? may register one global, one D3D9 and one D3D11 resource storage,
		//and manage resource location in a central manager. How to load both versions of the same resource transparently?
		Resources::PD3D11VertexShaderLoader VShaderLoader = n_new(Resources::CD3D11VertexShaderLoader);
		ResourceMgr->RegisterDefaultLoader("vsh", &Render::CShader::RTTI, VShaderLoader.GetUnsafe());
		Resources::PD3D11PixelShaderLoader PShaderLoader = n_new(Resources::CD3D11PixelShaderLoader);
		ResourceMgr->RegisterDefaultLoader("psh", &Render::CShader::RTTI, PShaderLoader.GetUnsafe());

		pCEGUIVS = "Shaders:Bin/4.vsh";
		pCEGUIPS = "Shaders:Bin/5.psh";
	}

	GPU = VideoDrvFct->CreateGPUDriver(Render::Adapter_Primary, Render::GPU_Hardware);
	n_assert(GPU.IsValidPtr());

	Render::CRenderTargetDesc BBDesc;
	BBDesc.Format = Render::PixelFmt_DefaultBackBuffer;
	BBDesc.MSAAQuality = Render::MSAA_None;
	BBDesc.UseAsShaderInput = false;
	BBDesc.MipLevels = 0;
	BBDesc.Width = 0;
	BBDesc.Height = 0;

	Render::CSwapChainDesc SCDesc;
	SCDesc.BackBufferCount = 2;
	SCDesc.SwapMode = Render::SwapMode_CopyDiscard;
	SCDesc.Flags = Render::SwapChain_AutoAdjustSize | Render::SwapChain_VSync;

	MainSwapChainIndex = GPU->CreateSwapChain(BBDesc, SCDesc, MainWindow);

	const Render::CRenderTargetDesc& RealBackBufDesc = GPU->GetSwapChainRenderTarget(MainSwapChainIndex)->GetDesc();

	{
		Render::CRenderTargetDesc DSDesc;
		DSDesc.Format = Render::PixelFmt_DefaultDepthBuffer;
		DSDesc.MSAAQuality = Render::MSAA_None;
		DSDesc.UseAsShaderInput = false;
		DSDesc.MipLevels = 0;
		DSDesc.Width = RealBackBufDesc.Width;
		DSDesc.Height = RealBackBufDesc.Height;

		Render::PDepthStencilBuffer DSBuf = GPU->CreateDepthStencilBuffer(DSDesc);
		n_assert(DSBuf.IsValidPtr());
	}

////////////////////////////
//!!!DBG TMP!
	SCIdx2 = GPU->CreateSwapChain(BBDesc, SCDesc, Wnd2);
	n_assert(GPU->SwapChainExists(SCIdx2));
////////////////////////////

	Resources::PRenderPathLoader RPLoader = n_new(Resources::CRenderPathLoader);
	ResourceMgr->RegisterDefaultLoader("hrd", &Frame::CRenderPath::RTTI, RPLoader);
	ResourceMgr->RegisterDefaultLoader("prm", &Frame::CRenderPath::RTTI, RPLoader);

	Resources::PCollisionShapeLoader CollShapeLoader = n_new(Resources::CCollisionShapeLoader);
	ResourceMgr->RegisterDefaultLoader("hrd", &Physics::CCollisionShape::RTTI, CollShapeLoader);
	ResourceMgr->RegisterDefaultLoader("prm", &Physics::CCollisionShape::RTTI, CollShapeLoader);

	Resources::PMeshLoaderNVX2 MeshLoaderNVX2 = n_new(Resources::CMeshLoaderNVX2);
	MeshLoaderNVX2->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("nvx2", &Render::CMesh::RTTI, MeshLoaderNVX2, false);

	Resources::PSkinInfoLoaderSKN SkinInfoLoaderSKN = n_new(Resources::CSkinInfoLoaderSKN);
	ResourceMgr->RegisterDefaultLoader("skn", &Render::CSkinInfo::RTTI, SkinInfoLoaderSKN);

	Resources::PKeyframeClipLoaderKFA KeyframeClipLoaderKFA = n_new(Resources::CKeyframeClipLoaderKFA);
	ResourceMgr->RegisterDefaultLoader("kfa", &Anim::CAnimClip::RTTI, KeyframeClipLoaderKFA);

	Resources::PMocapClipLoaderNAX2 MocapClipLoaderNAX2 = n_new(Resources::CMocapClipLoaderNAX2);
	ResourceMgr->RegisterDefaultLoader("nax2", &Anim::CAnimClip::RTTI, MocapClipLoaderNAX2, true);

	InputServer = n_new(Input::CInputServer);
	InputServer->Open();

	VideoServer = n_new(Video::CVideoServer);
	VideoServer->Open();

	//!!!need to compile properly named non-effect shaders! parse (CE)GUI settings HRD!
	//???redesign not to create default context with new CEGUI?
	UIServer = n_new(UI::CUIServer)(*GPU, MainSwapChainIndex, (float)RealBackBufDesc.Width, (float)RealBackBufDesc.Height, pCEGUIVS, pCEGUIPS);
	DbgSrv->AllowUI(true);

	//!!!to HRD params! data/cfg/UI.hrd
	if (UI::CUIServer::HasInstance())
	{
		UISrv->LoadFont("DejaVuSans-8.font");
		UISrv->LoadFont("DejaVuSans-10.font");
		UISrv->LoadFont("DejaVuSans-14.font");
		UISrv->LoadFont("CourierNew-10.font");
		UISrv->LoadScheme("TaharezLook.scheme");
	}

	MainUIContext = UISrv->GetDefaultContext();
	n_assert(MainUIContext.IsValidPtr());

	Sys::Log("Setup UI - OK\n");

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

	Sys::Log("InitGameSystem() ...\n");

	PhysicsServer = n_new(Physics::CPhysicsServer);
	PhysicsServer->Open();

	n_new(Game::CGameServer);
	GameSrv->Open();

	AIServer = n_new(AI::CAIServer);

	// Actor action templates
	Data::PParams ActTpls = DataSrv->LoadPRM("AI:AIActionTpls.prm");
	if (ActTpls.IsValidPtr())
	{
		for (UPTR i = 0; i < ActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = ActTpls->Get(i);
			AISrv->GetPlanner().RegisterActionTpl(Prm.GetName().CStr(), Prm.GetValue<Data::PParams>());
		}
		AISrv->GetPlanner().EndActionTpls();
	}

	// Smart object action templates
	Data::PParams SOActTpls = DataSrv->LoadPRM("AI:AISOActionTpls.prm");
	if (SOActTpls.IsValidPtr())
		for (UPTR i = 0; i < SOActTpls->GetCount(); ++i)
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
	FSM.AddStateHandler(n_new(CAppStateLoading(CStrID("Loading"))));
	FSM.AddStateHandler(n_new(CAppStateGame(CStrID("Game"))));
	
	Sys::Log("Seting state: Menu...\n");
	
	FSM.Init(CStrID("Menu"));
	
	Sys::Log("State setup - OK\n");

	OK;
}
//---------------------------------------------------------------------

bool CIPGApplication::AdvanceFrame()
{
	//!!!TMP REDESIGN!
	MSG Msg;
	while (::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		if (Msg.hwnd == MainWindow->GetHWND())
		{
			HACCEL hAccel = MainWindow->GetWin32AcceleratorTable();
			if (hAccel && ::TranslateAccelerator(MainWindow->GetHWND(), hAccel, &Msg) != FALSE) continue;
		}
		if (Msg.hwnd == Wnd2->GetHWND())
		{
			HACCEL hAccel = Wnd2->GetWin32AcceleratorTable();
			if (hAccel && ::TranslateAccelerator(Wnd2->GetHWND(), hAccel, &Msg) != FALSE) continue;
		}
		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
	}

///////////////////////
//!!!DBG TMP!
	if (!Wnd2->IsOpen())
	{
		Wnd2->SetWindowClass(*(Sys::COSWindowClassWin32*)EngineWindowClass.GetUnsafe()); //!!!bad design! use handle?
		Wnd2->SetTitle("Window 2");
		Wnd2->SetRect(Data::CRect(900, 50, 150, 200));
		Wnd2->Open();

		Render::CRenderTargetDesc BBDesc;
		BBDesc.Format = Render::PixelFmt_DefaultBackBuffer;
		BBDesc.MSAAQuality = Render::MSAA_None;
		BBDesc.UseAsShaderInput = false;
		BBDesc.MipLevels = 0;
		BBDesc.Width = 0;
		BBDesc.Height = 0;

		Render::CSwapChainDesc SCDesc;
		SCDesc.BackBufferCount = 2;
		SCDesc.SwapMode = Render::SwapMode_CopyDiscard;
		SCDesc.Flags = Render::SwapChain_AutoAdjustSize | Render::SwapChain_VSync;

		SCIdx2 = GPU->CreateSwapChain(BBDesc, SCDesc, Wnd2);
	}
///////////////////////

	//!!!TMP DBG!
	if (SCIdx2 >= 0)
	{
		Render::PRenderTarget RT = GPU->GetSwapChainRenderTarget(SCIdx2);
		if (RT.IsValidPtr() && RT->IsValid())
		{
			GPU->SetRenderTarget(0, RT);
			GPU->PresentBlankScreen(SCIdx2, vector4(0.7f, 0.1f, 0.7f, 1.f));
		}
	}

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
	
	GameSrv->Close();
	n_delete(GameSrv);

	PhysicsServer->Close();
	PhysicsServer = NULL;

	DbgSrv->AllowUI(false);
	MainUIContext = NULL;
	UIServer = NULL;

	if (VideoServer.IsValidPtr() && VideoServer->IsOpen()) VideoServer->Close();
	VideoServer = NULL;

	//if (AudioServer.IsValid() && AudioServer->IsOpen()) AudioServer->Close();
	//AudioServer = NULL;

	DD->Close();
	DD = NULL;

	GPU = NULL;
	VideoDrvFct = NULL;

	Wnd2->Close();
	Wnd2 = NULL;

	MainWindow->Close();
	MainWindow = NULL;

	EngineWindowClass->Destroy();
	EngineWindowClass = NULL;

	if (InputServer.IsValidPtr() && InputServer->IsOpen()) InputServer->Close();
	InputServer = NULL;

//	n_delete(FrameSrv);

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