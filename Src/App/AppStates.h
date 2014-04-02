#pragma once
#ifndef __IPG_APP_STATES_H__
#define __IPG_APP_STATES_H__

#include <App/StateHandler.h>

// Forward declarations and constant definitions for the game application states

namespace App
{

enum ELoadingRequest
{
	Request_NewLevel,
	Request_LoadLevel,
	Request_NewGame,
	Request_ContinueGame,
	Request_LoadGame,
	Request_Transition		// Travel from one location to another
};

}

#endif
