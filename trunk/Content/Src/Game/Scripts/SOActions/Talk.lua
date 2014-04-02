
function Update(ActorID, SOID)

	local State = DlgMgr.GetDialogueState(ActorID)
	if (State == DlgMgr.DlgState_Finished) then
		DlgMgr.CloseDialogue(ActorID)
		return 1 --Success
	elseif (State == DlgMgr.DlgState_Aborted or State == DlgMgr.DlgState_None) then
		DlgMgr.CloseDialogue(ActorID)
		return 0 --Failure
	else
		return 2 --Running
	end

end
