Base = "CEntityScriptObject"

Code = <"

function ContainerBase()
	this:SubscribeLocalEvent("OnSOStateEnter")
end

function OnSOStateEnter(e)
	if (e.From == "") then
		this:EnableAction("OpenContainer")
	end
end


"> 