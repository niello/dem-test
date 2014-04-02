function IsFirstTalk()
	return QuestMgr.GetQuestStatus("Ch0/Water") == QuestMgr.QSNo
end

function OpenWaterQuest()
	QuestMgr.StartQuest("Ch0/Water")
	Entities.GG:AddItem("Misc/Flask")
end

function CanCloseWaterQuest()
	return QuestMgr.GetQuestStatus("Ch0/Water", "ReturnFullFlask") == QuestMgr.QSOpened
		and Entities.GG:HasItem("Misc/FlaskFountainWater")
end

function CloseWaterQuest()
	Entities.GG:RemoveItem("Misc/FlaskFountainWater")
	QuestMgr.CompleteQuest("Ch0/Water")
end

function IsWaterQuestInProgress()
	return QuestMgr.GetQuestStatus("Ch0/Water") == QuestMgr.QSOpened
end