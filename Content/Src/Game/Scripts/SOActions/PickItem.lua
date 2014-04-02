
function IsValid(ActorID, SOID)
--!!!check weight and volume
	local Actor = Entities[ActorID]
	return Actor and Actor:IsPropertyActive("Inventory")
end

function GetDuration(ActorID, SOID)
	local Actor = Entities[ActorID]
	return (Actor and Actor:IsPropertyActive("Animation")) and Actor:GetAnimLength("PickItem") or 0
end