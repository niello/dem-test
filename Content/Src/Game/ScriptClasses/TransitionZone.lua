Base = "CEntityScriptObject"

Code = <"

function TransitionZone()
	this:SubscribeLocalEvent("OnSOActionDone")
end

--!!!on action start, if not all entities are in zone, warn user!

--???must be in smart action script?
function OnSOActionDone(e)

	if (e.Action == "Travel") then

		local IsFarTravel = FarTravel
		local EntityIDs = { }
		if IsFarTravel and Factions.Party:IsMember(e.Actor) then
			--add all the faction: Factions.Party:GetMembersAtLevel(LevelID)
			EntityIDs[1] = e.Actor --!!!DBG TMP!
		else
			EntityIDs[1] = e.Actor
		end

		EventSrv.FireEvent("OnWorldTransitionRequested", {
			EntityIDs = EntityIDs,
			LevelID = DestLevelID,
			MarkerID = DestMarkerID,
			IsFarTravel = IsFarTravel })
	
	end

end

"> 