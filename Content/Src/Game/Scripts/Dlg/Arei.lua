local QuestStatus

function OnStart()
	QuestStatus = QuestMgr.GetQuestStatus("Ch0/Sweep")
end

function IsFirstTalk()
	return QuestStatus == QuestMgr.QSNo
end

function OpenSweepQuest()
	QuestMgr.StartQuest("Ch0/Sweep")
	--Entities.ECCY_MonkStatist1_1:Go("ECCYMarkerMonk1Move1")
end

function IsBlaming()
	return Entities.Arei.WantsToBlame
end

function ClearBlameFlag()
	Entities.Arei.WantsToBlame = nil
	Quests.Ch0_Sweep_Sweep:UnsubscribeEvent("OnSOActionEnd")
end

function IsQuestOpen()
	return QuestStatus == QuestMgr.QSOpened
end

function CanCloseSweepQuest()
	return QuestMgr.GetQuestStatus("Ch0/Sweep", "ReportToArei") == QuestMgr.QSOpened
end

function CloseSweepQuest()
	QuestMgr.CompleteQuest("Ch0/Sweep")
	Entities.ECCYTrgConfessorCutscene:EnableTrigger()
end