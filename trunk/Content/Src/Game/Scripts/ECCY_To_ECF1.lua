
function OnPropInit()
	this:SubscribeEntityEvent("ECCY_DoorTest_1", "OnSOStateEnter", "OnDoorStateChanged")
	local Door = Entities.ECCY_DoorTest_1
	if (Door) then
		this:EnableAction("Travel", Door:IsInState("Opened"))
	end
end

function OnDoorStateChanged(e)
	this:EnableAction("Travel", e.To == "Opened")
end