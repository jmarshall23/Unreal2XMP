class LowlandsGiantTurret extends XMPStationaryTurret
	placeable;	

defaultproperties
{
	bUsable=false
	bProxyControlled=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.TurretLowlandsGiantTurntable'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'LowlandsGiantTurretBarrel'
	Barrel1FiringOffset=(X=0,Y=0,Z=0)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	//CameraOffset=(X=-222.5,Y=0,Z=226.5)
	CameraOffset=(X=-251.875,Y=0, Z=190.55)
	TurretOffset=(x=0,y=0.00,z=0)
	RateOfFire=0.2
	RotationCenter=(Pitch=1820,Yaw=0,Roll=0)
	PitchConstraint=3459
	YawConstraint=13198
	Health=600
	//bNetInitialRotation=true
}