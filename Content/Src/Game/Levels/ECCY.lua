-- ECCY level script

--!!!must be OnLevelActivated and be called once!
function OnLevelValidated(e)
	print("*** Level " .. e.ID .. " is loaded and validated\n")
-- !!!HERE IS CUTSCENE with monks going to work!
	if (QuestMgr.GetQuestStatus("Ch0/Sweep") == QuestMgr.QSNo) then
		Entities.Arei:DoAction(Factions.Party:GetLeader(), "Talk")
	end
end

this:SubscribeEvent("OnLevelValidated")