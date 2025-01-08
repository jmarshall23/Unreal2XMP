class LowlandsMediumWingedTurret extends XMPStationaryTurret
	placeable;	

simulated function RotateTurret( Rotator TurretRot )
{		
	Barrel.SetRotation(TurretRot + initialRotation);
	SetNetDirty(true);
	Barrel.SetNetDirty(true);

}

defaultproperties
{
	bUsable=false
	bProxyControlled=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.Turret_LowlandsWinged_Base'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'LowlandsMediumWingedTurretBarrel'
	Barrel1FiringOffset=(X=550,Y=0,Z=435)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	CameraOffset=(X=-173,Y=0,Z=181.5)
	TurretOffset=(x=0,y=0.00,z=0)
	RateOfFire=0.2
	RotationCenter=(Pitch=3640,Yaw=0,Roll=0)
	PitchConstraint=6000
	YawConstraint=65536
	Health=600
	//bNetInitialRotation=true
}