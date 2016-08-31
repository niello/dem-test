
// This file forces modules, that aren't explicitly referenced, to compile

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

// INPUT =======================================

#include <Input/InputConditionComboEvent.h>
#include <Input/InputConditionComboState.h>
#include <Input/InputConditionDown.h>
#include <Input/InputConditionHold.h>
#include <Input/InputConditionMove.h>
#include <Input/InputConditionPressed.h>
#include <Input/InputConditionReleased.h>
#include <Input/InputConditionSequence.h>
#include <Input/InputConditionUp.h>

// RENDERING =======================================

#include <Frame/NodeAttrLight.h>
#include <Frame/NodeAttrSkin.h>
#include <Frame/RenderPhaseGeometry.h>
#include <UI/RenderPhaseGUI.h>
#include <Render/Model.h>
#include <Render/Terrain.h>
#include <Render/Skybox.h>
#include <Render/ModelRenderer.h>
#include <Render/TerrainRenderer.h>
#include <Render/SkyboxRenderer.h>

// OTHER ===========================================

#include <Items/ItemTplWeapon.h>
#include <Debug/LuaConsole.h>
#include <Debug/WatcherWindow.h>

void ForceFactoryRegistration()
{
	Debug::CLuaConsole::ForceFactoryRegistration();
	Debug::CWatcherWindow::ForceFactoryRegistration();

	Frame::CNodeAttrLight::ForceFactoryRegistration();
	Frame::CNodeAttrSkin::ForceFactoryRegistration();
	Frame::CRenderPhaseGeometry::ForceFactoryRegistration();
	Frame::CRenderPhaseGUI::ForceFactoryRegistration();
	Render::CModel::ForceFactoryRegistration();
	Render::CTerrain::ForceFactoryRegistration();
	Render::CSkybox::ForceFactoryRegistration();
	Render::CModelRenderer::ForceFactoryRegistration();
	Render::CTerrainRenderer::ForceFactoryRegistration();
	Render::CSkyboxRenderer::ForceFactoryRegistration();

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

	Input::CInputConditionComboEvent::ForceFactoryRegistration();
	Input::CInputConditionComboState::ForceFactoryRegistration();
	Input::CInputConditionDown::ForceFactoryRegistration();
	Input::CInputConditionHold::ForceFactoryRegistration();
	Input::CInputConditionMove::ForceFactoryRegistration();
	Input::CInputConditionPressed::ForceFactoryRegistration();
	Input::CInputConditionReleased::ForceFactoryRegistration();
	Input::CInputConditionSequence::ForceFactoryRegistration();
	Input::CInputConditionUp::ForceFactoryRegistration();

	Items::CItemTplWeapon::ForceFactoryRegistration();
}
//---------------------------------------------------------------------
