// AI ==============================================

// Sensors, perceptors, stimuli
#include <AI/Sensors/SensorVision.h>
#include <AI/Perceptors/PerceptorObstacle.h>
#include <AI/Perceptors/PerceptorOverseer.h>
#include <AI/Perceptors/PerceptorSmartObj.h>
#include <AI/Stimuli/StimulusVisible.h>
#include <AI/Stimuli/StimulusSound.h>

// Goals, action tpls
#include <AI/Goals/GoalWander.h>
#include <AI/Goals/GoalWork.h>
#include <AI/Planning/ActionTplIdle.h>
#include <AI/ActionTpls/ActionTplWander.h>
#include <Items/ActionTpls/ActionTplEquipItem.h>
#include <Items/ActionTpls/ActionTplPickItemWorld.h>
#include <AI/SmartObj/ActionTpls/ActionTplGotoSmartObj.h>
#include <AI/SmartObj/ActionTpls/ActionTplUseSmartObj.h>

// Actions
#include <AI/Behaviour/ActionSequence.h>
#include <AI/Movement/Actions/ActionFaceTarget.h>
#include <AI/Movement/Actions/ActionGoto.h>
#include <AI/Movement/Actions/ActionSteerToPosition.h>
#include <AI/Planning/WorldStateSourceScript.h>

// RENDERERS =======================================

#include <Render/Renderers/ModelRenderer.h>
#include <Render/Renderers/TerrainRenderer.h>
#include <Render/Renderers/DebugGeomRenderer.h>
#include <Render/Renderers/DebugTextRenderer.h>
#include <UI/UIRenderer.h>

// OTHER ===========================================

#include <Items/ItemTplWeapon.h>
#include <Debug/LuaConsole.h>
#include <Debug/WatcherWindow.h>

void ForceFactoryRegistration()
{
	Debug::CLuaConsole::ForceFactoryRegistration();
	Debug::CWatcherWindow::ForceFactoryRegistration();

	Render::CModelRenderer::ForceFactoryRegistration();
	Render::CTerrainRenderer::ForceFactoryRegistration();
	Render::CUIRenderer::ForceFactoryRegistration();
	Render::CDebugGeomRenderer::ForceFactoryRegistration();
	Render::CDebugTextRenderer::ForceFactoryRegistration();

	AI::CActionTplIdle::ForceFactoryRegistration();
	AI::CActionTplWander::ForceFactoryRegistration();
	AI::CActionTplGotoSmartObj::ForceFactoryRegistration();
	AI::CActionTplUseSmartObj::ForceFactoryRegistration();
	AI::CActionTplPickItemWorld::ForceFactoryRegistration();
	AI::CActionTplEquipItem::ForceFactoryRegistration();
	AI::CActionSequence::ForceFactoryRegistration();
	AI::CActionFaceTarget::ForceFactoryRegistration();
	AI::CActionGoto::ForceFactoryRegistration();
	AI::CActionSteerToPosition::ForceFactoryRegistration();
	AI::CPerceptorObstacle::ForceFactoryRegistration();
	AI::CPerceptorOverseer::ForceFactoryRegistration();
	AI::CPerceptorSmartObj::ForceFactoryRegistration();
	AI::CSensorVision::ForceFactoryRegistration();
	AI::CStimulusVisible::ForceFactoryRegistration();
	AI::CStimulusSound::ForceFactoryRegistration();
	AI::CWorldStateSourceScript::ForceFactoryRegistration();

	Items::CItemTplWeapon::ForceFactoryRegistration();
}
//---------------------------------------------------------------------
