class HarbingerTurret extends XMPVehicleTurret
	notplaceable;
	

function OnUse (Actor User)
{
XMPVehicle(Owner).TryToDrive(Controller(User).Pawn, 1);
}

defaultproperties
{
	bUseMe=false
	bBlockActors=true
	bCollideActors=true	
	bBlockPlayers=true
	bBlockZeroExtentTraces=true
	bUsable=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.HarbingerTurretCollar_TD_001'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'HarbingerTurretBarrel'
	Barrel1FiringOffset=(X=0,Y=0,Z=0)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	//CameraOffset=(X=0,Y=0,Z=41.01)
	CameraOffset=(X=215.88,Y=0,Z=119.92)
	TurretOffset=(x=156.085,Y=0.00,Z=79.06)
	RateOfFire=0.2
	RotationCenter=(Pitch=-6626,Yaw=0,Roll=0)
	PitchConstraint=9400
	YawConstraint=65536
	AlternateSkins(0)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Blue'
}