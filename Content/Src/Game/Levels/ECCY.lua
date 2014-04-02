-- ECCY level script

function OnLevelLoaded(e)
	print("*** Level " .. e.ID .. " is loaded\n")
-- !!!HERE IS CUTSCENE with monks going to work!
	if (QuestMgr.GetQuestStatus("Ch0/Sweep") == QuestMgr.QSNo) then
		Entities.Arei:DoAction(Factions.Party:GetLeader(), "Talk")
	end
end

this:SubscribeEvent("OnLevelLoaded")