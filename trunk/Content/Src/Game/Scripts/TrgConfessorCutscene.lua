--???also send events from CPropTrigger? It would be useful for trigger-affectable entities without scripts

function TrgConfessorCutscene()
   print("TrgConfessorCutscene created: " .. name .. "\n")
   --can subscribe on events here
   --!!!can't do such call now! this:DisableTrigger()
end

-- local function TrgEffect(Entity)
	-- print("Entity " .. Entity.name .. " received effect from TrgConfessorCutscene\n")
-- end

function OnTriggerEnter(EntityID)
	local Entity = Entities[EntityID]
	if Entity then
		print("Entity " .. Entity.name .. " entered TrgConfessorCutscene\n")
		-- TrgEffect(Entity)
		
		if (Factions.Party:IsMember(EntityID)) then
			print("!!!HERE WE LAUNCH CUTSCENE!!!\n")
			this:DisableTrigger()
			--???remove trigger entity?
			Entities.Ftr_Confessor:DoAction(EntityID, "Talk")
		end
	end
end

--[[
function OnTriggerApply(EntityID)
	local Entity = Entities[EntityID]
	if Entity then TrgEffect(Entity) end
end
--]]

function OnTriggerLeave(EntityID)
	local Entity = Entities[EntityID]
	if Entity then
		print("Entity " .. Entity.name .. " left TrgConfessorCutscene\n")
	end
end