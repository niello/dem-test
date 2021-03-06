#pragma once
#ifndef __IPG_UI_TIP_WINDOW_H__
#define __IPG_UI_TIP_WINDOW_H__

#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>
#include <Events/Subscription.h>
#include <Data/StringID.h>
#include <Math/Vector2.h>
#include <Math/Vector3.h>

// Represents a floating tip which is bound to an entity

namespace UI
{

enum ETipAlignment
{
	TipAlign_Center	= 0x00,
	TipAlign_Top	= 0x01,
	TipAlign_Bottom	= 0x02,
	TipAlign_Left	= 0x04,
	TipAlign_Right	= 0x08
};

class CTipWindow: public CUIWindow
{
private:

	ETipAlignment	Alignment;
	CStrID			EntityID;
	vector3			WorldOffset;
	vector2			ScreenOffset;

	DECLARE_EVENT_HANDLER(OnUIUpdate, OnUIUpdate);

	void			UpdateBinding();

public:

	CTipWindow(): EntityID(CStrID::Empty) {}

	void			BindToEntity(CStrID _EntityID, ETipAlignment Alignment, vector2 ScreenOffset = vector2(0.0f, 0.0f), vector3 WorldOffset = vector3(0.0f, 0.0f, 0.0f));
	void			RecalcScreenPosition();
	virtual void	SetVisible(bool Visible);
	CStrID			GetEntityID() const { return EntityID; }
};

}

#endif