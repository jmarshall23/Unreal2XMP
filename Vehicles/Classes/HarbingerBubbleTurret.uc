class HarbingerBubbleTurret extends XMPVehicleTurret
	notplaceable;

defaultproperties
{
	bUsable=false
	DrawType=DT_None
	StaticMesh=None
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'HarbingerBubbleTurretBarrel'
	CameraOffset=(X=344,Y=0.00,Z=41)
	TurretOffset=(X=383.00,Y=0.00,Z=12.71)
	RateOfFire=0.2
	RotationCenter=(Pitch=3094,Yaw=32768,Roll=0)
	PitchConstraint=9739
	YawConstraint=11005
	CollisionHeight=0
	CollisionRadius=0
	AlternateSkins(0)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Blue'
}