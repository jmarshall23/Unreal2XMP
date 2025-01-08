class RaptorTurret extends XMPVehicleTurret
	notplaceable;
	

function OnUse (Actor User)
{
	XMPVehicle(Owner).TryToDrive(Controller(User).Pawn, 1);
}



defaultproperties
{
	bBlockActors=true
	bCollideActors=true	
	bBlockPlayers=true
	bBlockZeroExtentTraces=true
	bUsable=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorTurretCollar_TD_001'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'RaptorTurretBarrel'
	Barrel1FiringOffset=(X=0,Y=0,Z=0)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	//CameraOffset=(X=-52.35,Y=0,Z=25.17)
	CameraOffset=(X=61.19, Y=0, Z=75.60);
	TurretOffset=(x=113.54,y=0.00,z=14.43)
	RateOfFire=0.2
	RotationCenter=(Pitch=-2048,Yaw=0,Roll=0)
	PitchConstraint=4096
	YawConstraint=65536
	AlternateSkins(0)=Shader'VehiclesT.Raptor.RaptorFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Raptor.RaptorFX_TD_01_Blue'
	MaxRotation=75000
}