class JuggernautTurret extends XMPVehicleTurret
	notplaceable;
	

function OnUse (Actor User)
{
XMPVehicle(Owner).TryToDrive(Controller(User).Pawn, 1);
}

defaultproperties
{
	bUseMe=true
	bUsable=true
	bCollideActors=true
	bProjTarget=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.JuggernautTurret_TD_001'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'JuggernautTurretBarrelBase'
	Barrel1FiringOffset=(X=0,Y=0,Z=0)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	//CameraOffset=(X=-84.22,Y=57.55,Z=69.27)
	CameraOffset=(X=44.31,Y=-57.55,Z=175.32)
	TurretOffset=(X=148.53,Y=0,Z=116.05)
	RateOfFire=0.2
	RotationCenter=(Pitch=1024,Yaw=0,Roll=0)
	PitchConstraint=3400
	YawConstraint=65536
	MaxRotation=25000
	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}