-- Floor sweeping in the Cathedral of the Three-Faced's Great Glory (Prologue)
-- Starting task "Get a broom"

local function NextTask()
	QuestMgr.CompleteQuest("Ch0/Sweep", "GetBroom")
	QuestMgr.StartQuest("Ch0/Sweep", "Sweep")
end

function OnItemAdded(e)
	if (e.Item == "Misc/Broom" and Factions.Party:IsLeader(e.Entity)) then
		NextTask()
	end
end

if (Entities.GG:HasItem("Misc/Broom")) then
	NextTask()
else
	this:SubscribeEvent("OnItemAdded")
end