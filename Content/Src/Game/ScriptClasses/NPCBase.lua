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

	this:ClearTaskQueue() --???or just enqueue to front, to preserve other tasks?
	this:EnqueueTask({ Class = "FaceTarget", Target = e.Initiator })

	--???react on some event or enqueue response action?
	DlgMgr.AcceptDialogue(e.Initiator, name)

end

"> 