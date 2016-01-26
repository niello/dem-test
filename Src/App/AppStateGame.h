#pragma once
#ifndef __IPG_APP_STATE_GAME_H__
#define __IPG_APP_STATE_GAME_H__

#include <App/StateHandler.h>
#include <Debug/Profiler.h>
#include <Events/EventsFwd.h>

// The game state handler runs the game loop.

namespace App
{

class CAppStateGame: public CStateHandler
{
	__DeclareClassNoFactory;

protected:

	bool	RenderDbgAI;
	bool	RenderDbgPhysics;
	bool	RenderDbgGfx;
	bool	RenderDbgEntities;

	//???per-frame control to camera manager?
	float	CameraMoveX;
	float	CameraMoveZ;
	bool	CameraRotate;

	PROFILER_DECLARE(profCompleteFrame);
	PROFILER_DECLARE(profRender);

	bool IssueActorCommand(bool Run, bool ClearQueue);

	DECLARE_EVENT_HANDLER(MouseMoveRaw, OnMouseMoveRaw);
	DECLARE_EVENT_HANDLER(MouseWheel, OnMouseWheel);
	DECLARE_EVENT_HANDLER(MouseBtnDown, OnMouseBtnDown);
	DECLARE_EVENT_HANDLER(MouseBtnUp, OnMouseBtnUp);
	DECLARE_EVENT_HANDLER(MouseDoubleClick, OnMouseDoubleClick);
	DECLARE_EVENT_HANDLER(OnWorldTransitionRequested, OnWorldTransitionRequested);
	DECLARE_EVENT_HANDLER(QuickSave, OnQuickSave);
	DECLARE_EVENT_HANDLER(QuickLoad, OnQuickLoad);
	DECLARE_EVENT_HANDLER(ToggleGamePause, OnToggleGamePause);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgAI, OnToggleRenderDbgAI);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgPhysics, OnToggleRenderDbgPhysics);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgGfx, OnToggleRenderDbgGfx);
	DECLARE_EVENT_HANDLER(ToggleRenderDbgEntities, OnToggleRenderDbgEntities);
	DECLARE_EVENT_HANDLER(TeleportSelected, OnTeleportSelected);

public:

	CAppStateGame(CStrID StateID);

	virtual void	OnStateEnter(CStrID PrevState, Data::PParams Params = NULL);
	virtual void	OnStateLeave(CStrID NextState);
	virtual CStrID	OnFrame();
};

}

#endif
