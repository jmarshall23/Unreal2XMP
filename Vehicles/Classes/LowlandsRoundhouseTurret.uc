class LowlandsRoundhouseTurret extends XMPStationaryTurret
	placeable;	

/*simulated function RotateTurret( Rotator TurretRot )
{
	local rotator TurretBaseRotation;
	local rotator TurretGunRotation;
			
	TurretBaseRotation=TurretRot;
	TurretBaseRotation.Pitch=0;
	TurretBaseRotation.Roll=0;
	TurretGunRotation=TurretRot;
	TurretGunRotation.Roll=0;
	//TurretGunRotation.Yaw=0;
	SetRotation(TurretBaseRotation + initialRotation);
	Barrel.SetRotation(TurretGunRotation);

}*/

defaultproperties
{
	bUsable=false
	bProxyControlled=false
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.RoundhouseTurretGunsBase'
	bHardAttach=True
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'LowlandsRoundhouseTurretBarrel'
	Barrel1FiringOffset=(X=550,Y=0,Z=435)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	CameraOffset=(X=-144.564,Y=0,Z=20)
	//CameraOffset=(X=-550,Y=0,Z=435)
	TurretOffset=(x=0,y=0.00,z=0)
	RateOfFire=0.2
	//RotationCenter=(Pitch=3640,Yaw=0,Roll=0)
	RotationCenter=(Pitch=0,Yaw=0,Roll=0)
	//PitchConstraint=4551
	PitchConstraint=5461
	YawConstraint=65536
	Health=600
	//bNetInitialRotation=true
}