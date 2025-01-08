class JuggernautFrontTurret extends XMPVehicleTurret
	notplaceable;

function OnUse (Actor User)
{
XMPVehicle(Owner).TryToDrive(Controller(User).Pawn, 2);
}


defaultproperties
{
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.JuggernautFrontTurret_TD_001'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=false
	GunBarrelClass=class'JuggernautFrontTurretBarrel'
	Barrel1FiringOffset=(X=0,Y=0,Z=0)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	CameraOffset=(X=-0.3,Y=-121.32,Z=-50.28)
	TurretOffset=(x=-25.20,Y=98.03,Z=50.28)
	RateOfFire=0.2
	RotationCenter=(Pitch=-3003,Yaw=0,Roll=0)
	PitchConstraint=5188
	YawConstraint=13489
	MaxRotation=175000
	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}