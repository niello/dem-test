#pragma once
#ifndef __IPG_APP_STATE_LOADING_H__
#define __IPG_APP_STATE_LOADING_H__

#include <App/StateHandler.h>
#include <Frame/View.h>

// Handles state during the level loading

namespace UI
{
	class CLoadingScreen;
}

namespace App
{

class CAppStateLoading: public CStateHandler
{
	__DeclareClassNoFactory;

protected:

	Ptr<UI::CLoadingScreen> LoadingScreen;
	Frame::CView			View; //???main App view instead of per-state?! one window anyway!

	void DeleteUnreferencedResources();

public:

	CAppStateLoading(CStrID StateID): CStateHandler(StateID) {}

	virtual void	OnStateEnter(CStrID PrevState, Data::PParams Params = NULL);
	virtual void	OnStateLeave(CStrID NextState);
	virtual CStrID	OnFrame();
};

}

#endif
