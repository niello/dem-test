Base = "CEntityScriptObject"

Code = <"

function Fountain()
	this:SubscribeLocalEvent("OnSOActionDone")
end

function OnSOActionDone(e)

	local Actor = Entities[e.Actor]
	if (not Actor) then return end

	if (e.Action == "Drink") then
		Actor:SayPhrase("Вода и вправду вкусна!")
	elseif (e.Action == "FillVessel") then
		Actor:RemoveItem("Misc/Flask")
		Actor:AddItem("Misc/FlaskFountainWater")
	end

end

">