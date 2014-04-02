function OnItemAdded(e)
	if (e.Item == "Misc/FlaskFountainWater" and Factions.Party:IsLeader(e.Entity)) then
		QuestMgr.CompleteQuest("Ch0/Water", "FillFlask")
		QuestMgr.StartQuest("Ch0/Water", "ReturnFullFlask")
	end
end

this:SubscribeEvent("OnItemAdded")