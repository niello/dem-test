local NoParcelQuest

function OnStart()
	NoParcelQuest = QuestMgr.GetQuestStatus("Ch0/Parcel") == QuestMgr.QSNo
end

function CanOpenParcelQuest()
	return QuestMgr.GetQuestStatus("Ch0/Sweep") == QuestMgr.QSDone and NoParcelQuest
end

function ParcelQuestInactive()
	return NoParcelQuest
end

function StatistLeavesScene()
	Entities.ECCY_MonkStatist1_1:Go(295, 5, 302)
end

function OpenParcelQuest()
	QuestMgr.StartQuest("Ch0/Parcel")
	Entities[Factions.Party:GetLeader()]:AddItem("Misc/ParcelFromConfessor")
end