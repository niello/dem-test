#include "IPGApplication.h"

#include <System/OSWindowClass.h>
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
#include <Render/D3D11/D3D11Shader.h>
#include <Render/D3D11/D3D11ShaderLoaders.h>
#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/GPUDriver.h>
#include <Render/Texture.h>
#include <Render/RenderTarget.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/SwapChain.h>
#include <Render/RenderStateDesc.h>
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
	AppData.Format("AppData:%s/%s", GetVendorName(), GetAppName());
	IOSrv->SetAssign("AppData", IOSrv->ResolveAssigns(AppData));

	IOSrv->MountNPK("Proj:Export.npk"); //???only add CFileSystemNPK here?

	n_new(Resources::CResourceManager);
	//!!!register loaders!

	n_new(Data::CDataServer); //???need at all? can store DSS as rsrc!

	Data::PParams PathList = DataSrv->LoadHRD("Proj:PathList.hrd", false);
	if (PathList.IsValidPtr())
		for (int i = 0; i < PathList->GetCount(); ++i)
			IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ResolveAssigns(PathList->Get<CString>(i)));

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

	EngineWindowClass = n_new(Sys::COSWindowClass);
	n_assert(EngineWindowClass->Create("DeusExMachina::MainWindow", "Icon"));

	CString WindowTitle = CString(GetVendorName()) + " - " + GetAppName() + " - " + GetAppVersion();
	MainWindow = n_new(Sys::COSWindow);
	MainWindow->SetWindowClass(*(Sys::COSWindowClassWin32*)EngineWindowClass.GetUnsafe()); //!!!bad design!
	MainWindow->SetTitle(WindowTitle.CStr());
	MainWindow->SetRect(Data::CRect(50, 50, 800, 600));
	MainWindow->Open();

	DISP_SUBSCRIBE_PEVENT(MainWindow, OnClosing, CIPGApplication, OnOSWindowClosing);

///////////////////////
//!!!DBG TMP!
	Wnd2 = n_new(Sys::COSWindow);
	Wnd2->SetWindowClass(*(Sys::COSWindowClassWin32*)EngineWindowClass.GetUnsafe()); //!!!bad design!
	Wnd2->SetTitle("Window 2");
	Wnd2->SetRect(Data::CRect(900, 50, 150, 200));
	Wnd2->Open();
	SCIdx = -1; SCIdx2 = -1;
///////////////////////

	// Rendering

	const bool UseD3D9 = false;
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

		//!!!set CD3D11Shader type!
		ResourceMgr->RegisterDefaultLoader("vsh", &Render::CShader::RTTI, &Resources::CD3D11VertexShaderLoader::RTTI);
		ResourceMgr->RegisterDefaultLoader("psh", &Render::CShader::RTTI, &Resources::CD3D11PixelShaderLoader::RTTI);
	}
	//???or loaders are universal and use abstract GPUDriver as a factory?
	//???or register parallel loaders for D3D9 and D3D11, but there is no practical reason!

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

	SCIdx = GPU->CreateSwapChain(BBDesc, SCDesc, MainWindow);

	{
		Render::CTextureDesc TexDesc;
		TexDesc.Type = Render::Texture_2D;
		TexDesc.Width = 128;
		TexDesc.Height = 128;
		//TexDesc.Depth = 128;
		TexDesc.ArraySize = 1;
		TexDesc.MipLevels = 0;
		TexDesc.MSAAQuality = Render::MSAA_None;
		TexDesc.Format = Render::PixelFmt_DXT1;
		Render::PTexture Tex = GPU->CreateTexture(TexDesc, Render::Access_GPU_Read /*| Render::Access_CPU_Write*/, NULL);
		n_assert(Tex.IsValidPtr());

		//!!!profile align16 copying vs non-aligned!
		char* pData = (char*)n_malloc_aligned(4 * 1024 * 1024, 16);
		char* pData2 = (char*)n_malloc_aligned(4 * 1024 * 1024, 16);
		memset(pData, 0x40, 4 * 1024 * 1024 * sizeof(char));
		memset(pData2, 0x00, 4 * 1024 * 1024 * sizeof(char));
		Render::CImageData Data;
		Data.pData = pData;
		Data.RowPitch = Tex->GetRowPitch();
		n_assert(GPU->WriteToResource(*Tex, Data));
		Data.pData = pData2;
		n_assert(GPU->ReadFromResource(Data, *Tex));
		n_free_aligned(pData);
		n_free_aligned(pData2);
	}

	{
		Render::CVertexComponent Components[] = {
				{ Render::VCSem_Position, NULL, 0, Render::VCFmt_Float32_3, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT },
				{ Render::VCSem_Color, NULL, 0, Render::VCFmt_UInt8_4_Norm, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT },
				{ Render::VCSem_TexCoord, NULL, 0, Render::VCFmt_Float32_2, 0, DEM_VERTEX_COMPONENT_OFFSET_DEFAULT } };

		Render::PVertexLayout VertexLayout = GPU->CreateVertexLayout(Components, sizeof_array(Components));
		n_assert(VertexLayout.IsValidPtr());

		Render::PVertexBuffer VB = GPU->CreateVertexBuffer(*VertexLayout, 16, Render::Access_GPU_Read);
		n_assert(VB.IsValidPtr());

		Render::PIndexBuffer IB = GPU->CreateIndexBuffer(Render::Index_32, 64, Render::Access_GPU_Read | Render::Access_CPU_Write);
		n_assert(IB.IsValidPtr());

		char Data[65536];
		memset(Data, 0x40, sizeof(Data));
		n_assert(GPU->WriteToResource(*VB, Data));
		n_assert(GPU->WriteToResource(*IB, Data));
	}

	//{
	//	Render::PConstantBuffer CB = GPU->CreateConstantBuffer(*n_new(Render::CD3D11Shader), CStrID::Empty, 0, NULL);
	//}

	{
		//!!!load shaders! can create PShader objects manually, not from file/URI!

		Render::CRenderStateDesc RSDesc;
		Render::CRenderStateDesc::CRTBlend& RTBlendDesc = RSDesc.RTBlend[0];
		RSDesc.SetDefaults();
		RSDesc.VertexShader = NULL;
		RSDesc.PixelShader = NULL;
		RSDesc.Flags.Set(Render::CRenderStateDesc::Blend_RTBlendEnable << 0);
		RSDesc.Flags.Clear(Render::CRenderStateDesc::DS_DepthEnable |
						   Render::CRenderStateDesc::DS_DepthWriteEnable |
						   Render::CRenderStateDesc::Rasterizer_DepthClipEnable |
						   Render::CRenderStateDesc::Rasterizer_Wireframe |
						   Render::CRenderStateDesc::Rasterizer_CullFront |
						   Render::CRenderStateDesc::Rasterizer_CullBack |
						   Render::CRenderStateDesc::Blend_AlphaToCoverage |
						   Render::CRenderStateDesc::Blend_Independent);

		// Normal blend
		RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_InvDestAlpha;
		RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_One;
		RTBlendDesc.SrcBlendArg = Render::BlendArg_SrcAlpha;
		RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

		// Unclipped
		RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

		Render::PRenderState NormalUnclipped = GPU->CreateRenderState(RSDesc);
		n_assert(NormalUnclipped.IsValidPtr());

		// Clipped
		RSDesc.Flags.Set(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

		Render::PRenderState NormalClipped = GPU->CreateRenderState(RSDesc);
		n_assert(NormalClipped.IsValidPtr());

		// Premultiplied alpha blend
		RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_One;
		RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_InvSrcAlpha;
		RTBlendDesc.SrcBlendArg = Render::BlendArg_One;
		RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

		Render::PRenderState PremultipliedClipped = GPU->CreateRenderState(RSDesc);
		n_assert(PremultipliedClipped.IsValidPtr());

		// Unclipped
		RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

		Render::PRenderState PremultipliedUnclipped = GPU->CreateRenderState(RSDesc);
		n_assert(PremultipliedUnclipped.IsValidPtr());
	}

////////////////////////////
//!!!DBG TMP!
	SCIdx2 = GPU->CreateSwapChain(BBDesc, SCDesc, Wnd2);
	n_assert(GPU->SwapChainExists(SCIdx2));
////////////////////////////

	////DBG TMP check asm for fabs
	//volatile int xxx = n_fequal(0.533f, 0.53346f, 0.001f);

	{
		const Render::CRenderTargetDesc& RealRTDesc = GPU->GetSwapChainRenderTarget(SCIdx)->GetDesc();
		//if (xxx && RealRTDesc.Width > 0)
		//{
			Render::CRenderTargetDesc DSDesc;
			DSDesc.Format = Render::PixelFmt_DefaultDepthBuffer;
			DSDesc.MSAAQuality = Render::MSAA_None;
			DSDesc.UseAsShaderInput = false;
			DSDesc.MipLevels = 0;
			DSDesc.Width = RealRTDesc.Width;
			DSDesc.Height = RealRTDesc.Height;

			Render::PDepthStencilBuffer DSBuf = GPU->CreateDepthStencilBuffer(DSDesc);
			n_assert(DSBuf.IsValidPtr());
		//}
	}

	//Render::PFrameShader DefaultFrameShader = n_new(Render::CFrameShader);
	//n_assert(DefaultFrameShader->Init(*DataSrv->LoadPRM("Shaders:Default.prm")));
	//RenderServer->AddFrameShader(CStrID("Default"), DefaultFrameShader);
	//RenderServer->SetScreenFrameShaderID(CStrID("Default"));

	InputServer = n_new(Input::CInputServer);
	InputServer->Open();

	VideoServer = n_new(Video::CVideoServer);
	VideoServer->Open();

	//!!!can use different GUI contexts, one per swap chain!
	UIServer = n_new(UI::CUIServer)(*GPU, SCIdx, "Shaders:CEGUI.vsh", "Shaders:CEGUI.psh");
	DbgSrv->AllowUI(true);

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
	if (ActTpls.IsValidPtr())
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
	if (SOActTpls.IsValidPtr())
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
		Wnd2->SetWindowClass(*(Sys::COSWindowClassWin32*)EngineWindowClass.GetUnsafe()); //!!!bad design!
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
	if (SCIdx >= 0)
	{
		Render::PRenderTarget RT = GPU->GetSwapChainRenderTarget(SCIdx);
		if (RT.IsValidPtr() && RT->IsValid())
		{
			GPU->SetRenderTarget(0, RT);
			GPU->PresentBlankScreen(SCIdx, vector4(0.1f, 0.7f, 0.1f, 1.f));
		}
	}
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
	
	GameServer->Close();
	GameServer = NULL;

	PhysicsServer->Close();
	PhysicsServer = NULL;

	DbgSrv->AllowUI(false);
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