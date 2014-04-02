#pragma once
#ifndef __IPG_APP_STATE_MENU_H__
#define __IPG_APP_STATE_MENU_H__

#include <App/StateHandler.h>

// Main menu handler

namespace App
{

class CAppStateMenu: public CStateHandler
{
	__DeclareClassNoFactory;

protected:

public:

	CAppStateMenu(CStrID StateID): CStateHandler(StateID) {}

	virtual void	OnStateEnter(CStrID PrevState, Data::PParams Params = NULL);
	virtual void	OnStateLeave(CStrID NextState);
	virtual CStrID	OnFrame();
};

}

#endif
