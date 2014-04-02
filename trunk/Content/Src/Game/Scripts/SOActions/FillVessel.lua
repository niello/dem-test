
function IsValid(ActorID, SOID)
	local Actor = Entities[ActorID]
	return Actor and Actor:IsPropertyActive("Inventory") and Actor:HasItem("Misc/Flask")
end
