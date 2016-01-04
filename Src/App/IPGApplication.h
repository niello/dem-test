#include <App/AppFSM.h>

#include <System/OSWindow.h>

#include <World/WorldManager.h>
#include <Dlg/DialogueManager.h>
#include <Quests/QuestManager.h>
#include <Items/ItemManager.h>
#include <Factions/FactionManager.h>

//???!!!forward declarations?
#include <Core/CoreServer.h>
//#include <Time/TimeServer.h>
#include <Debug/DebugServer.h>
#include <IO/IOServer.h>
#include <Data/DataServer.h>
#include <Events/EventServer.h>
#include <Scripting/ScriptServer.h>
#include <Debug/DebugDraw.h>
//#include <Audio/AudioServer.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <Game/GameServer.h>
#include <AI/AIServer.h>
#include <UI/UIServer.h>
#include <Render/DisplayMode.h>
#include <Render/VideoDriverFactory.h>
#include <Video/VideoServer.h>
//!!!move platform-specific code to separate module!
#ifdef RegisterClass
#undef RegisterClass
#endif
#ifdef GetObject
#undef GetObject
#endif

namespace Sys
{
	typedef Ptr<class COSWindowClass> POSWindowClass;
	typedef Ptr<class COSWindow> POSWindow;
}

namespace Render
{
	class CVideoDriverFactory;
}

namespace UI
{
	typedef Ptr<class CUIContext> PUIContext;
}

namespace App
{
#define IPGApp App::CIPGApplication::Instance()

class CIPGApplication //???!!!check multiple instances? bool AllowMultipleInstances
{
	__DeclareSingleton(CIPGApplication);

private:

	CString								ProjDir;

	Ptr<Events::CEventServer>			EventServer;

	//Ptr<Time::CTimeServer>				TimeServer;
	Ptr<Debug::CDebugServer>			DebugServer;
	Ptr<Debug::CDebugDraw>				DD;
	Ptr<Physics::CPhysicsServer>		PhysicsServer;
	Ptr<Input::CInputServer>			InputServer;
	Ptr<Video::CVideoServer>			VideoServer;
	Ptr<Game::CGameServer>				GameServer;
	Ptr<AI::CAIServer>					AIServer;
	Ptr<UI::CUIServer>					UIServer;

	Sys::POSWindowClass					EngineWindowClass;
	Ptr<Render::CVideoDriverFactory>	VideoDrvFct;

	Ptr<RPG::CWorldManager>				WorldManager;
	Ptr<Story::CQuestManager>			QuestManager;
	Ptr<Story::CDialogueManager>		DialogueManager;
	Ptr<Items::CItemManager>			ItemManager;
	Ptr<RPG::CFactionManager>			FactionManager;

	DECLARE_EVENT_HANDLER(OnClosing, OnOSWindowClosing);

public:

	CAppFSM								FSM;

	Render::PGPUDriver					GPU;
	Sys::POSWindow						MainWindow;
	int									MainSwapChainIndex; //???or get by window?
	UI::PUIContext						MainUIContext;

	//!!!DBG TMP!
	Sys::POSWindow Wnd2; int SCIdx2;

	CIPGApplication() { __ConstructSingleton; }
	~CIPGApplication() { __DestructSingleton; }

	const char*	GetAppName() const { return "Insane Poet"; }
	const char*	GetAppVersion() const;
	const char*	GetVendorName() const { return "DeusExMachina"; }

	bool		Open();
	bool		AdvanceFrame();
	void		Close();
};

inline const char* CIPGApplication::GetAppVersion() const
{
#ifdef _DEBUG
	return "0.2.0.0-d";
#else
	return "0.2.0.0";
#endif
}
//---------------------------------------------------------------------

}
