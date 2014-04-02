// Quest-related object

Base = "CEntityScriptObject"

Code = <"

function SweepZone()
	this:SubscribeLocalEvent("OnSOActionDone")
	this:SubscribeLocalEvent("OnSOActionEnd")
end

-- Redirects local event to global environment for it to be catched by the quest script
function OnSOActionDone(e)
	if (e.Action == "Sweep" and Factions.Party:IsMember(e.Actor)) then
		this:EnableUI(false)
		EventSrv.FireEvent("OnSweepDone")
	end
end

-- Redirects local event to global environment for it to be catched by the quest script
function OnSOActionEnd(e)
	if (e.Action == "Sweep" and Factions.Party:IsMember(e.Actor)) then
		this:EnableAction("Sweep", false) -- can be used only once (delete entity?)
		EventSrv.FireEvent("OnSweepEnd")
	end
end

"> 