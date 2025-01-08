class RaptorDriverTurret extends XMPVehicleTurret
	notplaceable;




defaultproperties
{
	bBlockActors=false
	bCollideActors=false	
	bBlockPlayers=false
	bBlockZeroExtentTraces=false
	bUsable=false
	DrawType=DT_None
	StaticMesh=None
	bHardAttach=True
	DrawScale=1
	bArticulated=false
	bRelativeRotation=false
	GunBarrelClass=None
	CameraOffset=(X=0,Y=0,Z=0)
	TurretOffset=(X=-30,Y=0,Z=20)
	RotationCenter=(Pitch=0,Yaw=0,Roll=0)
	PitchConstraint=4096
	//YawConstraint=65536
	YawConstraint=12743
	MaxRotation=175000
}
