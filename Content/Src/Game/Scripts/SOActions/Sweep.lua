
function IsValid(ActorID, SOID)
	local Actor = Entities[ActorID]
	return Actor and Actor:IsPropertyActive("Equipment") and Actor:GetEquippedItemID("MainWpn") == "Misc/Broom"
end

function GetPreconditions(ActorID, SOID)
	return IsValid(ActorID, SOID) and true or { WSP_ItemEquipped = "Misc/Broom" }
end

function GetDuration(ActorID, SOID)
	return RandomFloat(2, 4)
end