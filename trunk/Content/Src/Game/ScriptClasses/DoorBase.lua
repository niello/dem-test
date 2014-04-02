// Uses specific entity attr NavRegion

Base = "CEntityScriptObject"

Code = <"

function DoorBase()
	this:SubscribeLocalEvent("OnSOStateEnter")
	this:SubscribeLocalEvent("OnSOStateLeave")
end

--!!!fenv is broken on call to this function!
function InitState(this, StateID)
	if (StateID == "Opened") then
		this:EnableAction("Close")
		this:EnableAction("Open", false)
		NavMesh.UnlockRegion(LevelID, NavRegion)
	elseif (StateID == "Closed") then
		this:EnableAction("Open")
		this:EnableAction("Close", false)
		NavMesh.LockRegion(LevelID, NavRegion)
	end
end

function OnSOStateEnter(e)
	if (e.From == "") then
		this:InitState(e.To)
	else
		if (e.To == "Opened") then
			this:EnableAction("Open", false)
			NavMesh.UnlockRegion(LevelID, NavRegion)
		elseif (e.To == "Closed") then
			this:EnableAction("Close", false)
		end
	end
end

function OnSOStateLeave(e)
	if (e.From == "Opened") then
		this:EnableAction("Open")
		NavMesh.LockRegion(LevelID, NavRegion)
	elseif (e.From == "Closed") then
		this:EnableAction("Close")
	end
end

"> 