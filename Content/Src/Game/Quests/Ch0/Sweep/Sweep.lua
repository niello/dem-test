-- Floor sweeping in the Cathedral of the Three-Faced's Great Glory (Prologue)
-- Task "Sweep three places in a Cathedral's yard"

--???enable character's sweep zones only here?

function OnSweepDone()
	local Zones = GameSrv.GetGlobal("Ch0_Sweep_Zones")
	Zones = Zones + 1
	if (Zones >= 3) then
		GameSrv.SetGlobal("Ch0_Sweep_Zones", nil)
		QuestMgr.CompleteQuest("Ch0/Sweep", "Sweep")
		QuestMgr.StartQuest("Ch0/Sweep", "ReportToArei")
	else
		GameSrv.SetGlobal("Ch0_Sweep_Zones", Zones)
		TimeSrv.CreateTimer("AreiBlame", 5, false, "OnAreiBlame")
		this:SubscribeEvent("OnSweepEnd")
	end
end

function OnSweepEnd()
	if (Entities.Arei.WantsToBlame) then
		Entities.Arei.WantsToBlame = nil
		Entities.Arei:AbortCurrAction()
	end
	TimeSrv.DestroyTimer("AreiBlame")
	this:UnsubscribeEvent("OnSweepEnd")
end

function OnAreiBlame(e)
	Entities.Arei.WantsToBlame = true
	Entities.Arei:DoAction("GG", "Talk")
end

if (not GameSrv.HasGlobal("Ch0_Sweep_Zones")) then
	GameSrv.SetGlobal("Ch0_Sweep_Zones", 0)
end

this:SubscribeEvent("OnSweepDone")
this:SubscribeEvent("OnAreiBlame")
