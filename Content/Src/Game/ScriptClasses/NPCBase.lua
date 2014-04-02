Base = "CEntityScriptObject"

Code = <"

function NPCBase()
	this:SubscribeLocalEvent("OnDlgRequest")
end

function OnDlgRequest(e)

	--???how to check if we are already talking, even if we are target?
	if (DlgMgr.GetDialogueState(name) ~= DlgMgr.DlgState_None) then
		DlgMgr.RejectDialogue(e.Initiator, name)
		return
	end

	--!!!here we can queue actions, then listen specified end event and then accept dialogue!
	DlgMgr.AcceptDialogue(e.Initiator, name)
	
end

"> 