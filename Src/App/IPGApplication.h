#include <App/AppFSM.h>

#include <World/WorldManager.h>
#include <Dlg/DialogueManager.h>
#include <Quests/QuestManager.h>
#include <Items/ItemManager.h>
#include <Factions/FactionManager.h>

#include <Data/Singleton.h>

namespace App
{

#define IPGApp App::CIPGApplication::Instance()

class CIPGApplication
{
	__DeclareSingleton(CIPGApplication);

private:

	Ptr<RPG::CWorldManager>			WorldManager;
	Ptr<Story::CQuestManager>		QuestManager;
	Ptr<Story::CDialogueManager>	DialogueManager;
	Ptr<Items::CItemManager>		ItemManager;
	Ptr<RPG::CFactionManager>		FactionManager;
	
	void	RegisterAttributes();

	DECLARE_EVENT_HANDLER(OnDisplayClose, OnDisplayClose);

public:

	CAppFSM FSM;

	CIPGApplication();
	~CIPGApplication();

	CString	GetAppName() const { return "Insane Poet"; }
	CString	GetAppVersion() const;
	CString	GetVendorName() const { return "STILL NO TEAM NAME"; }

	bool	Open();
	bool	AdvanceFrame();
	void	Close();
};

inline CString CIPGApplication::GetAppVersion() const
{
#ifdef _DEBUG
	return "0.1d Step 19";
#else
	return "0.1 Step 19";
#endif
}
//---------------------------------------------------------------------

}
