#pragma once
#ifndef __IPG_UI_DLG_WINDOW_H__
#define __IPG_UI_DLG_WINDOW_H__

#include <UI/UIWindow.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>

// Foreground dialogue UI shows the main dialogue in a dedicated window and provides user control
// over a dialogue flow, in particular allowing him to select answers through Link_Select linking mode.

namespace CEGUI
{
	class PushButton;
	class Listbox;
	class EventArgs;
}

namespace Story
{
	class CDlgContext;
}

namespace UI
{

class CDialogueWindow: public CUIWindow
{
protected:

	CStrID						DlgID;

	CEGUI::PushButton*			pContinueBtn; //???or store as window?
	CEGUI::Listbox*				pTextArea;
	CEGUI::ListboxTextItem*		pLastItemUnderCursor;

	CEGUI::Event::Connection	ConnAnswerClicked;
	CEGUI::Event::Connection	ConnTextAreaMM;
	CEGUI::Event::Connection	ConnKeyUp;

	void SelectAnswer(Story::CDlgContext& Ctx, int Idx);

	// UI to DlgMgr event handlers
	bool OnContinueBtnClicked(const CEGUI::EventArgs& e);
	bool OnAnswerClicked(const CEGUI::EventArgs& e);
	bool OnTextAreaMouseMove(const CEGUI::EventArgs& e);
	bool OnKeyUp(const CEGUI::EventArgs& e);

	//???!!!fill textarea from DlgMgr message buffer or use tmp interface?

	// DlgMgr to UI event handlers
	DECLARE_EVENT_HANDLER(OnDlgStart, OnDlgStart);
	DECLARE_EVENT_HANDLER(OnDlgEnd, OnDlgEnd);
	DECLARE_EVENT_HANDLER(OnForegroundDlgNodeEnter, OnDlgNodeEnter);

public:

	CDialogueWindow(): pContinueBtn(NULL), pTextArea(NULL), pLastItemUnderCursor(NULL) {}
	virtual ~CDialogueWindow();

	virtual void Init(CEGUI::Window* pWindow);
	//!!!Release/Destroy/Term/Close!
};

}

#endif