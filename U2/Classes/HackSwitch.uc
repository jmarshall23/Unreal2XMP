//=============================================================================
// HackSwitch.uc
//=============================================================================

class HackSwitch extends HackTrigger
	placeable;

//-----------------------------------------------------------------------------

#exec Texture Import File=Textures\S_HackSwitch.pcx Name=S_HackSwitch Mips=Off MASKED=1

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'TexPropSphere'
	Description="HackSwitch"
	bIgnoreAlreadyHacked=true
	bTriggerTeam=true
}
