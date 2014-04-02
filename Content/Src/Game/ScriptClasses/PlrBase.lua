Base = "CEntityScriptObject"

Code = <"

function PlrBase()
	this:SubscribeLocalEvent("OnSOActionDone")
	this:SubscribeLocalEvent("OnSOActionEnd")
	this:SubscribeLocalEvent("OnDlgRequest")
	--this:SubscribeLocalEvent("OnDlgStart")
end

function OnSOActionDone(e)
	if (e.Action == "OpenContainer") then
		this:SubscribeEvent("OnContainerWindowClosed")
		--!!!call UISrv.ShowWindow("Name", Params)!
		EventSrv.FireEvent("ShowContainerWindow", e)
	end
end

function OnSOActionEnd(e)
	if (e.Action == "OpenContainer") then
		this:UnsubscribeEvent("OnContainerWindowClosed")
		--!!!call UISrv.HideWindow("Name")!
		EventSrv.FireEvent("HideContainerWindow")
	end
end

--!!!???need universal event OnUIWindowClosed with id as param!?
function OnContainerWindowClosed(e)
	this:AbortCurrAction() --Success by default
	this:UnsubscribeEvent("OnContainerWindowClosed")
end

function OnDlgRequest(e)

	--???how to check if we are already talking, even if we are target?
	if (DlgMgr.GetDialogueState(name) ~= DlgMgr.DlgState_None) then
		DlgMgr.RejectDialogue(e.Initiator, name)
		return
	end

	--!!!here we can queue actions, then listen specified end event and then accept dialogue!
	DlgMgr.AcceptDialogue(e.Initiator, name)
	
end

--[[function OnDlgStart(e)
	if (e.IsForeground) then FireEvent("DisableInput") end --!!!or use script function instead of event!
end--]]

"> 