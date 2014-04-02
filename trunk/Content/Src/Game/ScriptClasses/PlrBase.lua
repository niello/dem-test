Base = "CEntityScriptObject"

Code = <"

function PlrBase()
	this:SubscribeLocalEvent("OnSOActionDone")
	this:SubscribeLocalEvent("OnSOActionEnd")
	--this:SubscribeLocalEvent("OnDlgStart")
	--OnDlgRequest
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

--[[function OnDlgStart(e)
	if (e.IsForeground) then FireEvent("DisableInput") end --!!!or use script function instead of event!
end--]]

"> 