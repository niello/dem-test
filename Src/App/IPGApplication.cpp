#include "IPGApplication.h"

#include <Scene/PropSceneNode.h> //???!!!move all props from Prop:: to Game::?
#include <AI/PropSmartObject.h>
#include <AI/PropAIHints.h>
#include <AI/PropActorBrain.h>
#include <Scripting/PropScriptable.h>
#include <Animation/PropAnimation.h>
#include <UI/PropUIControl.h>
#include <UI/UIServer.h>
#include <Input/InputTranslator.h>
#include <Input/OSWindowMouse.h>
#include <Input/OSWindowKeyboard.h>
#include <Render/GPUDriver.h>
#include <Render/Shader.h>
#include <Render/Texture.h>
#include <Render/TextureLoaderDDS.h>
#include <Render/TextureLoaderTGA.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/SwapChain.h>
#include <Render/RenderStateDesc.h>
#include <Render/Mesh.h>
#include <Render/MeshLoaderNVX2.h>
#include <Render/CDLODData.h>
#include <Render/CDLODDataLoader.h>
#include <Render/Material.h>
#include <Render/MaterialLoader.h>
#include <Render/Effect.h>
#include <Render/EffectLoader.h>
#include <Render/ShaderLibrary.h>
#include <Render/ShaderLibraryLoaderSLB.h>
#include <Render/SkinInfo.h>
#include <Render/SkinInfoLoaderSKN.h>
#include <Scene/SceneNodeLoaderSCN.h>
#include <Animation/KeyframeClip.h>
#include <Animation/KeyframeClipLoaderKFA.h>
#include <Animation/MocapClip.h>
#include <Animation/MocapClipLoaderNAX2.h>
#include <Frame/RenderPath.h>
#include <Frame/RenderPathLoaderRP.h>
#include <Physics/CollisionShapeLoader.h>
#include <Resources/Resource.h>
#include <Resources/ResourceManager.h>
#include <Physics/PropPhysics.h>
#include <Physics/PropCharacterController.h>
#include <Dlg/PropTalking.h>
#include <Items/Prop/PropEquipment.h>
#include <Items/Prop/PropItem.h>
#include <IO/IOServer.h>
#include <Data/DataArray.h>
#include <Data/ParamsUtils.h>
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

namespace App
{
__ImplementSingleton(App::CIPGApplication);

bool CIPGApplication::Open()
{
	Math::InitRandomNumberGenerator();

	n_new(Core::CCoreServer);
	n_new(Events::CEventServer);
	n_new(IO::CIOServer);

	if (!ProjDir.IsValid()) ProjDir = IOSrv->GetAssign("Home");
	IOSrv->SetAssign("Proj", ProjDir);

	CString AppData;
	AppData.Format("AppData:%s/%s", GetVendorName(), GetAppName());
	IOSrv->SetAssign("AppData", IOSrv->ResolveAssigns(AppData));

	IOSrv->MountNPK("Proj:Export.npk"); //???only add CFileSystemNPK here?

	n_new(Resources::CResourceManager);

	Data::PParams PathList;
	ParamsUtils::LoadParamsFromHRD("Proj:PathList.hrd", PathList);
	if (PathList.IsValidPtr())
		for (UPTR i = 0; i < PathList->GetCount(); ++i)
			IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));

	DebugServer = n_new(Debug::CDebugServer);
	DebugServer->RegisterPlugin(CStrID("Console"), "Debug::CLuaConsole", "DebugConsole.layout");
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
	CStrID GfxAPI; //???to GPUDrv? GetAPIID()
	Resources::PShaderLoader ShaderLoader;
	if (UseD3D9)
	{
		Render::PD3D9DriverFactory Fct = n_new(Render::CD3D9DriverFactory);
		Fct->Open(MainWindow);
		VideoDrvFct = Fct;

		ShaderLoader = n_new(Resources::CD3D9ShaderLoader);

		IOSrv->SetAssign("Effects", IOSrv->ResolveAssigns("Shaders:SM_3_0/Effects/"));
		IOSrv->SetAssign("RenderPathes", IOSrv->ResolveAssigns("Shaders:SM_3_0/"));

		GfxAPI = CStrID("D3D9");
	}
	else
	{
		Render::PD3D11DriverFactory Fct = n_new(Render::CD3D11DriverFactory);
		Fct->Open();
		VideoDrvFct = Fct;

		//???rsrc storage - not singleton? may register one global, one D3D9 and one D3D11 resource storage,
		//and manage resource location in a central manager. How to load both versions of the same resource transparently?
		ShaderLoader = n_new(Resources::CD3D11ShaderLoader);

		IOSrv->SetAssign("Effects", IOSrv->ResolveAssigns("Shaders:USM/Effects/"));
		IOSrv->SetAssign("RenderPathes", IOSrv->ResolveAssigns("Shaders:USM/"));

		GfxAPI = CStrID("D3D11");
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

////////////////////////////
//!!!DBG TMP!
	SCIdx2 = GPU->CreateSwapChain(BBDesc, SCDesc, Wnd2);
	n_assert(GPU->SwapChainExists(SCIdx2));
////////////////////////////

	ShaderLoader->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("vsh", &Render::CShader::RTTI, ShaderLoader.GetUnsafe());
	ResourceMgr->RegisterDefaultLoader("psh", &Render::CShader::RTTI, ShaderLoader.GetUnsafe());

	Resources::PShaderLibraryLoaderSLB ShaderLibraryLoaderSLB = n_new(Resources::CShaderLibraryLoaderSLB);
	ResourceMgr->RegisterDefaultLoader("slb", &Render::CShaderLibrary::RTTI, ShaderLibraryLoaderSLB, true);

	Render::PShaderLibrary ShaderLib;
	{
		Resources::PResource RShaderLib = ResourceMgr->RegisterResource("Shaders:Shaders.slb");
		if (!RShaderLib->IsLoaded())
		{
			ResourceMgr->LoadResourceSync(*RShaderLib, *ShaderLibraryLoaderSLB.GetUnsafe());
			n_assert(RShaderLib->IsLoaded());
		}
		ShaderLib = RShaderLib->GetObject<Render::CShaderLibrary>();
		ShaderLib->SetLoader(ShaderLoader);
	}

	Resources::PRenderPathLoaderRP RPLoaderRP = n_new(Resources::CRenderPathLoaderRP);
	ResourceMgr->RegisterDefaultLoader("rp", &Frame::CRenderPath::RTTI, RPLoaderRP);

	Resources::PMaterialLoader MaterialLoader = n_new(Resources::CMaterialLoader);
	MaterialLoader->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("mtl", &Render::CMaterial::RTTI, MaterialLoader, false);

	Resources::PEffectLoader EffectLoader = n_new(Resources::CEffectLoader);
	EffectLoader->GPU = GPU;
	EffectLoader->ShaderLibrary = ShaderLib;
	ResourceMgr->RegisterDefaultLoader("eff", &Render::CEffect::RTTI, EffectLoader, true);

	Resources::PCollisionShapeLoaderPRM CollShapeLoaderPRM = n_new(Resources::CCollisionShapeLoaderPRM);
	ResourceMgr->RegisterDefaultLoader("prm", &Physics::CCollisionShape::RTTI, CollShapeLoaderPRM);

	Resources::PMeshLoaderNVX2 MeshLoaderNVX2 = n_new(Resources::CMeshLoaderNVX2);
	MeshLoaderNVX2->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("nvx2", &Render::CMesh::RTTI, MeshLoaderNVX2, false);

	Resources::PCDLODDataLoader CDLODDataLoader = n_new(Resources::CCDLODDataLoader);
	CDLODDataLoader->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("cdlod", &Render::CCDLODData::RTTI, CDLODDataLoader, false);

	Resources::PTextureLoaderDDS TextureLoaderDDS = n_new(Resources::CTextureLoaderDDS);
	TextureLoaderDDS->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("dds", &Render::CTexture::RTTI, TextureLoaderDDS.GetUnsafe());

	Resources::PTextureLoaderTGA TextureLoaderTGA = n_new(Resources::CTextureLoaderTGA);
	TextureLoaderTGA->GPU = GPU;
	ResourceMgr->RegisterDefaultLoader("tga", &Render::CTexture::RTTI, TextureLoaderTGA.GetUnsafe());

	Resources::PSkinInfoLoaderSKN SkinInfoLoaderSKN = n_new(Resources::CSkinInfoLoaderSKN);
	ResourceMgr->RegisterDefaultLoader("skn", &Render::CSkinInfo::RTTI, SkinInfoLoaderSKN);

	Resources::PKeyframeClipLoaderKFA KeyframeClipLoaderKFA = n_new(Resources::CKeyframeClipLoaderKFA);
	ResourceMgr->RegisterDefaultLoader("kfa", &Anim::CAnimClip::RTTI, KeyframeClipLoaderKFA);

	Resources::PMocapClipLoaderNAX2 MocapClipLoaderNAX2 = n_new(Resources::CMocapClipLoaderNAX2);
	ResourceMgr->RegisterDefaultLoader("nax2", &Anim::CAnimClip::RTTI, MocapClipLoaderNAX2, true);

	Resources::PSceneNodeLoaderSCN SceneNodeLoaderSCN = n_new(Resources::CSceneNodeLoaderSCN);
	ResourceMgr->RegisterDefaultLoader("scn", &Scene::CSceneNode::RTTI, SceneNodeLoaderSCN);

	VideoServer = n_new(Video::CVideoServer);
	VideoServer->Open();

	Data::PParams UIDesc;
	ParamsUtils::LoadParamsFromPRM("UI:UI.prm", UIDesc);
	if (UIDesc.IsValidPtr())
	{
		Data::PParams ShadersDesc = UIDesc->Get<Data::PParams>(CStrID("Shaders"))->Get<Data::PParams>(GfxAPI);

		UI::CUISettings UISettings;
		UISettings.GPUDriver = GPU;
		UISettings.VertexShader = ShaderLib->GetShaderByID((U32)ShadersDesc->Get<int>(CStrID("VS"), 0));
		UISettings.PixelShaderRegular = ShaderLib->GetShaderByID((U32)ShadersDesc->Get<int>(CStrID("PS"), 0));
		UISettings.PixelShaderOpaque = ShaderLib->GetShaderByID((U32)ShadersDesc->Get<int>(CStrID("PSOpaque"), 0));
		UISettings.SwapChainID = MainSwapChainIndex;
		UISettings.DefaultContextWidth = (float)RealBackBufDesc.Width;
		UISettings.DefaultContextHeight = (float)RealBackBufDesc.Height;
		UIDesc->Get<Data::PParams>(UISettings.ResourceGroups, CStrID("ResourceGroups"));

		//???redesign not to create default context with new CEGUI?
		n_new(UI::CUIServer)(UISettings);

		Data::PParams LoadOnStartup;
		if (UIDesc->Get<Data::PParams>(LoadOnStartup, CStrID("LoadOnStartup")))
		{
			Data::PDataArray ResourcesToLoad;

			if (LoadOnStartup->Get<Data::PDataArray>(ResourcesToLoad, CStrID("Fonts")))
				for (UPTR i = 0; i < ResourcesToLoad->GetCount(); ++i)
					UISrv->LoadFont(ResourcesToLoad->Get<CString>(i).CStr());

			if (LoadOnStartup->Get<Data::PDataArray>(ResourcesToLoad, CStrID("Schemes")))
				for (UPTR i = 0; i < ResourcesToLoad->GetCount(); ++i)
					UISrv->LoadScheme(ResourcesToLoad->Get<CString>(i).CStr());
		}

		UIDesc = NULL;

		MainUIContext = UISrv->GetDefaultContext();
		n_assert(MainUIContext.IsValidPtr());

		DbgSrv->AllowUI(true);

		Sys::Log("Setup UI - OK\n");
	}

	n_new(Scripting::CScriptServer);
	if (!Scripting::CEntityScriptObject::RegisterClass())
	{
		Close();
		FAIL;
	}
	
	Sys::Log("InitEngine() - OK\n");

	SI::RegisterGlobals();
	SI::RegisterTime();
	SI::RegisterEventServer();
	
	Sys::Log("Engine SI registration - OK\n");

	// Input

	//!!!must be attached to all windows or to the current active window or to windows associated with a player!
	pMouseDevice = n_new(Input::COSWindowMouse);
	((Input::COSWindowMouse*)pMouseDevice)->Attach(MainWindow.GetUnsafe(), 50);
	pKeyboardDevice = n_new(Input::COSWindowKeyboard);
	((Input::COSWindowKeyboard*)pKeyboardDevice)->Attach(MainWindow.GetUnsafe(), 50);

	pInputTranslator = n_new(Input::CInputTranslator(0));

	Data::PParams Desc;
	if (ParamsUtils::LoadParamsFromPRM("Input:Layouts.prm", Desc) && Desc.IsValidPtr())
		pInputTranslator->LoadSettings(*Desc.GetUnsafe());

	pInputTranslator->EnableContext(CStrID("Debug"));
	pInputTranslator->ConnectToDevice(pMouseDevice);
	pInputTranslator->ConnectToDevice(pKeyboardDevice);

	Sys::Log("Setup input - OK\n");

	Sys::Log("InitGameSystem() ...\n");

	PhysicsServer = n_new(Physics::CPhysicsServer);
	PhysicsServer->Open();

	n_new(Game::CGameServer);
	GameSrv->Open();

	AIServer = n_new(AI::CAIServer);

	// Actor action templates
	Data::PParams ActTpls;
	ParamsUtils::LoadParamsFromPRM("AI:AIActionTpls.prm", ActTpls);
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
	Data::PParams SOActTpls;
	ParamsUtils::LoadParamsFromPRM("AI:AISOActionTpls.prm", SOActTpls);
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

	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropSceneNode>(256);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropSmartObject>(64);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropUIControl>(64);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropAnimation>(128);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropPhysics>(128);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropScriptable>(64);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropAIHints>(64);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropActorBrain>(32);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropTalking>(32);
	//GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropDestructible>(32);
	//GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropWeapon>(32);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropInventory>(32);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropItem>(32);
	GameSrv->GetEntityMgr()->RegisterProperty<Prop::CPropCharacterController>(16);

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
	if (Wnd2.IsValidPtr() && !Wnd2->IsOpen())
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

	if (Game::CGameServer::HasInstance()) GameSrv->ExitGame();

	WorldManager = NULL;
	DialogueManager = NULL;
	QuestManager = NULL;
	ItemManager = NULL;
	FactionManager = NULL;

	AIServer = NULL;

	if (Game::CGameServer::HasInstance())
	{
		GameSrv->Close();
		n_delete(GameSrv);
	}

	if (Physics::CPhysicsServer::HasInstance())
	{
		PhysicsServer->Close();
		PhysicsServer = NULL;
	}

	DbgSrv->AllowUI(false);
	MainUIContext = NULL;
	if (UI::CUIServer::HasInstance()) n_delete(UISrv);

	if (VideoServer.IsValidPtr() && VideoServer->IsOpen()) VideoServer->Close();
	VideoServer = NULL;

	DD->Close();
	DD = NULL;

	GPU = NULL;
	VideoDrvFct = NULL;

	SAFE_DELETE(pInputTranslator);
	SAFE_DELETE(pKeyboardDevice);
	SAFE_DELETE(pMouseDevice);

	if (Wnd2.IsValidPtr())
	{
		Wnd2->Close();
		Wnd2 = NULL;
	}

	if (MainWindow.IsValidPtr())
	{
		MainWindow->Close();
		MainWindow = NULL;
	}

	if (EngineWindowClass.IsValidPtr())
	{
		EngineWindowClass->Destroy();
		EngineWindowClass = NULL;
	}

	DebugServer = NULL;

	if (Scripting::CScriptServer::HasInstance()) n_delete(ScriptSrv);
	if (Events::CEventServer::HasInstance()) n_delete(EventSrv);
	if (Resources::CResourceManager::HasInstance()) n_delete(ResourceMgr);
	if (IO::CIOServer::HasInstance()) n_delete(IOSrv);
	if (Core::CCoreServer::HasInstance()) n_delete(CoreSrv);
}
//---------------------------------------------------------------------

bool CIPGApplication::OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FSM.RequestState(CStrID::Empty);
	OK;
}
//---------------------------------------------------------------------

}