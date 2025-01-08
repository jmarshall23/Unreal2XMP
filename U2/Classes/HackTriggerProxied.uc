//=============================================================================
// HackTriggerProxied.uc
//=============================================================================

class HackTriggerProxied extends HackTrigger
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_Sprite
	Texture=S_HackTrigger
	Description="HackTriggerProxied"

	bHidden=true
	bUsable=false
	bCollideActors=false
	bProjTarget=false
	bBlockZeroExtentTraces=false
}
