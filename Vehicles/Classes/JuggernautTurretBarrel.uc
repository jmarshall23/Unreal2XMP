class JuggernautTurretBarrel extends actor
	notplaceable;

var		Vector	BarrelOffset;

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
		Owner.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
}

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.JuggernautTurretBarrel_TD_001'
	bHardAttach=True
	DrawScale=1
	BarrelOffset=(X=-195.35,Y=0,Z=0)
	RemoteRole=Role_None
	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}