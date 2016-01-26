#pragma once
#ifndef __IPG_UI_INGAME_SCREEN_H__
#define __IPG_UI_INGAME_SCREEN_H__

#include "TipWindow.h"

#include <UI/ContainerWindow.h>
#include <Events/EventsFwd.h>
#include <Events/Event.h>

// Ingame UI screen, containing dialogue window, action list popup etc

namespace UI
{
class CActionListPopup;
class CDialogueWindow;
class CIngameMenuPanel;
class CInventory;
class CMoveItemsWindow;
class CTipWindow;

class CIngameScreen: public CUIWindow
{
	__DeclareClassNoFactory;

protected:

	Ptr<CActionListPopup>	ActionPopup;
	Ptr<CDialogueWindow>	DlgWindow;
	Ptr<CIngameMenuPanel>	IngameMenuPanel;
	Ptr<CInventory>			Inventory;
	Ptr<CContainerWindow>	ContainerWindow;
	Ptr<CMoveItemsWindow>	MoveItemsWindow;
	//CArray<Ptr<CUIWindow>>	IAOTip;
	Ptr<CTipWindow>			IAOTip; //!!!for now only 1 tip available simultaneously!
	CArray<Ptr<CTipWindow>> PhraseTips;

	CEGUI::Listbox*			Console; //!!!later will be a class!

	bool OnDbgExitBtnClick(const CEGUI::EventArgs& e);

	DECLARE_EVENT_HANDLER(ShowIAOTip, ShowIAOTip);
	DECLARE_EVENT_HANDLER(HideIAOTip, HideIAOTip);
	DECLARE_EVENT_HANDLER(ShowPhrase, ShowPhrase);
	DECLARE_EVENT_HANDLER(HidePhrase, HidePhrase)

	DECLARE_EVENT_HANDLER(OnQuestStatusChanged, OnQuestStatusChanged);
	DECLARE_EVENT_HANDLER(OnObjectDescRequested, OnObjectDescRequested);
	DECLARE_EVENT_HANDLER(OnIAOActionStart, OnIAOActionStart);
	DECLARE_EVENT_HANDLER(OnIAOActionAbort, OnIAOActionAbort);
	
	Ptr<CTipWindow>	CreateTipWindow(int TipID);
	Ptr<CTipWindow> GetOrCreatePhraseTip(CStrID EntityID);
	Ptr<CTipWindow> GetPhraseTip(CStrID EntityID);
	static bool		ShowTip(CStrID EntityID, CTipWindow* pTipWnd, const CString& Text, ETipAlignment Alignment);

public:

	CIngameScreen();
	virtual ~CIngameScreen();

	virtual void	Init(CEGUI::Window* pWindow);

	//!!!may be very bad design!
	bool			IsContainerWndVisible() const { return ContainerWindow->IsVisible(); }
};

inline Ptr<CTipWindow> CIngameScreen::GetPhraseTip(CStrID EntityID)
{
	for (CArray<Ptr<CTipWindow>>::CIterator It = PhraseTips.Begin(); It != PhraseTips.End(); ++It)
		if ((*It)->GetEntityID() == EntityID)
			return *It;
	return NULL;
}
//---------------------------------------------------------------------

}

#endif
