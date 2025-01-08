class JuggernautTireArm extends Actor
	notplaceable;

var	vector	LocationOffset;
var	vector	LocationOffset2;

defaultproperties
{
	bBlockActors=false
	bCollideActors=false	
	bBlockPlayers=false
	bBlockKarma=false
	bBlockZeroExtentTraces=false
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.JuggernautTireArm_TD_001'
	bHardAttach=True
	DrawScale=1
	LocationOffset=(X=5.68, Y=167.59, Z=-73.89)
	LocationOffset2=(X=5.68, Y=-167.59, Z=-73.89)
	RemoteRole=ROLE_None
	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}