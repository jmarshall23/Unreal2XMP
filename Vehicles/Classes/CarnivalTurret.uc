class CarnivalTurret extends XMPStationaryTurret
	placeable;

var (CarnivalTurret )float MaxHeight;
var (CarnivalTurret )float MoveSpeed;
var vector DesiredLocation;

replication
{
	reliable if (Role==ROLE_Authority && bNetInitial)
		MaxHeight, MoveSpeed;
}

simulated function PostBeginPlay()
{
    Super.PostBeginPlay();
	if (!DesiredLocation?)
		DesiredLocation = Location;

}

simulated function Tick(float deltaSeconds)
{
	Super.Tick(deltaSeconds);
	//DesiredLocation = Location;
	if ( Throttle != 0 )
	{
		if (Throttle > 0)
		{
			DesiredLocation.Z += deltaSeconds * MoveSpeed;
		}else
		{
			DesiredLocation.Z -= deltaSeconds * MoveSpeed;
		}
		DesiredLocation.Z = clamp(DesiredLocation.Z, InitialLocation.Z, InitialLocation.Z+MaxHeight);
	}

	if (DesiredLocation != Location)
	{
		SetLocation(DesiredLocation);
	}
}

defaultproperties
{
	MoveSpeed=500
	MaxHeight=2700
	MaxRotation=25000
	bUsable=false
	bProxyControlled=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'Arch_TurretsM.Turret_PostArm'
	bCollideWhenPlacing=False
	DrawScale=1
	bArticulated=true
	bRelativeRotation=true
	GunBarrelClass=class'CarnivalTurretBarrel'
	Barrel1FiringOffset=(X=550,Y=0,Z=435)
	Barrel2FiringOffset=(X=0,Y=0,Z=0)
	CameraOffset=(X=-222.2,Y=0,Z=60.38)
	//CameraOffset=(X=-550,Y=0,Z=435)
	TurretOffset=(x=0,y=0.00,z=0)
	RateOfFire=0.2
	//RotationCenter=(Pitch=3640,Yaw=0,Roll=0)
	RotationCenter=(Pitch=0,Yaw=0,Roll=0)
	//PitchConstraint=4551
	PitchConstraint=19114
	YawConstraint=65536
	Health=600
	bNetInitialRotation=true
	bHardAttach=false
	bStatic=false
	bMovable=true
	bCollideWorld=false
}