class ACTION_SetHidden extends ScriptedAction;

var(Action) bool bHidden;
var(Action) name HideActorTag;

function bool InitActionFor(ScriptedController C)
{
	local Actor A;
	if ( HideActorTag != '' )
	{
		ForEach C.AllActors(class'Actor',A,HideActorTag)
			A.bHidden = bHidden;
	}
	else
	C.GetInstigator().bHidden = bHidden;
	return false;	
}