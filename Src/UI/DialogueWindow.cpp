#include "DialogueWindow.h"

#include <Dlg/DialogueManager.h>
#include <Game/EntityManager.h> // For entity attr "Name"
#include <Events/EventServer.h>
#include <UI/UIServer.h>
#include <UI/CEGUI/FmtLbTextItem.h>
#include <Data/StringUtils.h>

#include <CEGUI/Event.h>
#include <CEGUI/widgets/Listbox.h>
#include <CEGUI/widgets/PushButton.h>

namespace UI
{

CDialogueWindow::~CDialogueWindow()
{
}
//---------------------------------------------------------------------

void CDialogueWindow::Init(CEGUI::Window* pWindow)
{
	CUIWindow::Init(pWindow);

	CString WndName(pWindow->getName().c_str());

	pContinueBtn = (CEGUI::PushButton*)pWnd->getChild((WndName + "/MainButton").CStr());
	pContinueBtn->subscribeEvent(CEGUI::PushButton::EventClicked,
		CEGUI::Event::Subscriber(&CDialogueWindow::OnContinueBtnClicked, this));

	pTextArea = (CEGUI::Listbox*)pWnd->getChild((WndName + "/TextArea").CStr());
	pTextArea->setShowVertScrollbar(true);
	pTextArea->setMultiselectEnabled(false);

	//!!!not here - bug below!
	ConnKeyUp = pWnd->subscribeEvent(CEGUI::Window::EventKeyUp,
		CEGUI::Event::Subscriber(&CDialogueWindow::OnKeyUp, this));
	ConnTextAreaMM = pTextArea->subscribeEvent(CEGUI::Listbox::EventMouseMove,
		CEGUI::Event::Subscriber(&CDialogueWindow::OnTextAreaMouseMove, this));
	ConnAnswerClicked = pTextArea->subscribeEvent(CEGUI::Listbox::EventMouseClick,
		CEGUI::Event::Subscriber(&CDialogueWindow::OnAnswerClicked, this));

	//!!!DBG! - works
	//ConnKeyUp->disconnect();

	SUBSCRIBE_PEVENT(OnDlgStart, CDialogueWindow, OnDlgStart);
	SUBSCRIBE_PEVENT(OnDlgEnd, CDialogueWindow, OnDlgEnd);
	SUBSCRIBE_PEVENT(OnForegroundDlgNodeEnter, CDialogueWindow, OnDlgNodeEnter);
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnContinueBtnClicked(const CEGUI::EventArgs& e)
{
	n_assert_dbg(DlgID.IsValid());
	Story::CDlgContext* pCtx = DlgMgr->GetDialogue(DlgID);
	n_assert(pCtx);
	pCtx->State = Story::DlgState_InLink;
	OK;
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnAnswerClicked(const CEGUI::EventArgs& e)
{
	n_assert_dbg(DlgID.IsValid());
	const CEGUI::MouseEventArgs& Args = (const CEGUI::MouseEventArgs&)e;
	CEGUI::ListboxTextItem* pItem = (CEGUI::ListboxTextItem*)pTextArea->getItemAtPoint(Args.position);
	if (pItem)
	{
		Story::CDlgContext* pCtx = DlgMgr->GetDialogue(DlgID);
		n_assert(pCtx);
		//!!!subscribe CEGUI event only on answer (select) nodes instead!
		if (pCtx->pCurrNode->LinkMode == Story::CDlgNode::Link_Select)
		{
			UPTR ValidLinkCount = pCtx->ValidLinkIndices.GetCount();
			UPTR Idx = pTextArea->getItemIndex(pItem) + ValidLinkCount - pTextArea->getItemCount();
			if (Idx < ValidLinkCount) SelectAnswer(*pCtx, Idx);
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnTextAreaMouseMove(const CEGUI::EventArgs& e)
{
	/*const CEGUI::MouseEventArgs& Args = (const CEGUI::MouseEventArgs&)e;
	CEGUI::ListboxTextItem* pItem = (CEGUI::ListboxTextItem*)pTextArea->getItemAtPoint(Args.position);
	if (pItem != pLastItemUnderCursor)
	{
		if (pLastItemUnderCursor) pLastItemUnderCursor->setTextColours(CEGUI::colour(0xffff0000));
		if (pItem)
		{
			pItem->setTextColours(CEGUI::colour(0xffffffff));
			pItem->setText("SELECTED");
		}
		pTextArea->handleUpdatedItemData();
		pTextArea->invalidate();
		pWnd->invalidate();
		pLastItemUnderCursor = pItem;
	}*/
	OK;
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnKeyUp(const CEGUI::EventArgs& e)
{
	n_assert_dbg(DlgID.IsValid());

	const CEGUI::KeyEventArgs& KeyArgs = (const CEGUI::KeyEventArgs&)e;

	Story::CDlgContext* pCtx = DlgMgr->GetDialogue(DlgID);
	n_assert(pCtx);

	if (pCtx->pCurrNode->LinkMode == Story::CDlgNode::Link_Select &&
		KeyArgs.scancode >= CEGUI::Key::One &&
		KeyArgs.scancode < (CEGUI::Key::One + (IPTR)pCtx->ValidLinkIndices.GetCount()))
	{
		SelectAnswer(*pCtx, KeyArgs.scancode - CEGUI::Key::One);
		OK;
	}
	else if (KeyArgs.scancode == CEGUI::Key::Return && pContinueBtn->isVisible())
	{
		OnContinueBtnClicked(e);
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

void CDialogueWindow::SelectAnswer(Story::CDlgContext& Ctx, UPTR Idx)
{
	Ctx.SelectValidLink(Idx);

	int ValidLinkCount = Ctx.ValidLinkIndices.GetCount();
	while (ValidLinkCount-- > 0)
		pTextArea->removeItem(pTextArea->getListboxItemFromIndex(pTextArea->getItemCount() - 1));

	//!!!bug!
	//ConnKeyUp->disconnect();
	//ConnTextAreaMM->disconnect();
	//ConnAnswerClicked->disconnect();

	pLastItemUnderCursor = NULL;
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnDlgStart(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	if (!P->Get<bool>(CStrID("IsForeground"))) FAIL;

	n_assert_dbg(!DlgID.IsValid());

	DlgID = P->Get<CStrID>(CStrID("Initiator"));
	pContinueBtn->setVisible(false);
	//Show(); - delayed to first actual phrase
	OK;
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnDlgEnd(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	if (!P->Get<bool>(CStrID("IsForeground"))) FAIL;

	n_assert_dbg(DlgID.IsValid() && P->Get<CStrID>(CStrID("Initiator")) == DlgID);

	DlgID = CStrID::Empty;
	Hide();
	pTextArea->resetList();
	OK;
}
//---------------------------------------------------------------------

bool CDialogueWindow::OnDlgNodeEnter(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	n_assert_dbg(DlgID.IsValid());

	Story::CDlgContext* pCtx = DlgMgr->GetDialogue(DlgID);
	n_assert(pCtx);

	CStrID SpeakerEntity = pCtx->pCurrNode->SpeakerEntity;

	if (!SpeakerEntity.IsValid() || !pCtx->pCurrNode->Phrase.IsValid()) OK;

	if (SpeakerEntity == CStrID("$DlgOwner")) SpeakerEntity = pCtx->DlgOwner;
	else if (SpeakerEntity == CStrID("$PlrSpeaker")) SpeakerEntity = pCtx->PlrSpeaker;
	Game::PEntity Speaker = EntityMgr->GetEntity(SpeakerEntity, true);
	if (Speaker.IsNullPtr())
		Sys::Error("CDialogueManager::SayPhrase -> speaker entity '%s' not found", SpeakerEntity.CStr());

	CString Text;
	if (!Speaker->GetAttr<CString>(Text, CStrID("Name")))
		Text = Speaker->GetUID().CStr();
	Text += ": ";
	Text += pCtx->pCurrNode->Phrase.CStr();

	CEGUI::FormattedListboxTextItem* NewItem =
		n_new(CEGUI::FormattedListboxTextItem((CEGUI::utf8*)Text.CStr(), CEGUI::HTF_WORDWRAP_LEFT_ALIGNED));//!!!, 0, 0, true);
	n_assert(NewItem);
	NewItem->setTextColours(CEGUI::Colour(0xffb0b0b0));
	pTextArea->addItem(NewItem);
	pTextArea->ensureItemIsVisible(pTextArea->getItemCount() - 1);

	Show();
	pWnd->activate(); //???activate button when it is visible?

	UPTR ValidLinkCount = pCtx->ValidLinkIndices.GetCount();
	if (pCtx->pCurrNode->LinkMode == Story::CDlgNode::Link_Select && ValidLinkCount > 0)
	{
		pContinueBtn->setVisible(false);
		//???remove cegui event conn here?

		for (UPTR i = 0; i < ValidLinkCount; ++i)
		{
			Story::CDlgNode::CLink& Link = pCtx->pCurrNode->Links[pCtx->ValidLinkIndices[i]];

			Text = StringUtils::FromInt(i + 1);
			Text += ": ";
			Text += Link.pTargetNode ? Link.pTargetNode->Phrase.CStr() : NULL;

			//???pool instead of new?
			CEGUI::FormattedListboxTextItem* pNewItem =
				n_new(CEGUI::FormattedListboxTextItem((CEGUI::utf8*)Text.CStr(), CEGUI::HTF_WORDWRAP_LEFT_ALIGNED));
			n_assert(pNewItem);
			pNewItem->setTextColours(CEGUI::Colour(0xffff0000));
			pTextArea->addItem(pNewItem);
			pTextArea->ensureItemIsVisible(pTextArea->getItemCount());
		}

		//!!!bug!
		//ConnKeyUp = pWnd->subscribeEvent(CEGUI::Window::EventKeyUp,
		//	CEGUI::Event::Subscriber(&CDialogueWindow::OnKeyUp, this));
		//ConnTextAreaMM = pTextArea->subscribeEvent(CEGUI::Listbox::EventMouseMove,
		//	CEGUI::Event::Subscriber(&CDialogueWindow::OnTextAreaMouseMove, this));
		//ConnAnswerClicked = pTextArea->subscribeEvent(CEGUI::Listbox::EventMouseClick,
		//	CEGUI::Event::Subscriber(&CDialogueWindow::OnAnswerClicked, this));
	}
	else if (pCtx->LinkIdx >= 0 && pCtx->LinkIdx < ValidLinkCount)
	{
		pContinueBtn->setText("Continue");
		pContinueBtn->setVisible(true);
		//???cegui event conn here?
	}
	else
	{
		pContinueBtn->setText("End dialogue");
		pContinueBtn->setVisible(true);
		//???cegui event conn here?
	}

	OK;
}
//---------------------------------------------------------------------

}
