
class Juggernaut extends XMPVehicle
	    placeable;

var		JuggernautTireArm JuggerArm1, JuggerArm2;


simulated function PostBeginPlay()
{
    Super.PostBeginPlay();
	if (bPendingDelete)
		return;
	//only have tirearms on client

	if(Level.NetMode != NM_DedicatedServer)
	{	
		JuggerArm1 = spawn (class'JuggernautTireArm', self,, Location + (class'JuggernautTireArm'.default.LocationOffset >> Rotation));
		JuggerArm1.SetBase(Self);
		JuggerArm1.SetRotation (Rotation);
		JuggerArm1.bCollideWorld = false;
		JuggerArm1.SetCollision(false, false, false);
		
		JuggerArm2 = spawn (class'JuggernautTireArm', self,, Location + (class'JuggernautTireArm'.default.LocationOffset2 >> Rotation));
		JuggerArm2.SetBase(Self);
		JuggerArm2.SetRotation (Rotation);
		JuggerArm2.SetDrawScale3D(vect(1, -1, 1));
		JuggerArm2.bCollideWorld = false;
		JuggerArm2.SetCollision(false, false, false);

	}
}

simulated function Tick(float Delta)
{
	local Rotator TireArmRotation;

    Super.Tick(Delta);
	if(Level.NetMode != NM_DedicatedServer)
	{
		TireArmRotation.Pitch=0;
		TireArmRotation.Roll=0;
		TireArmRotation.Yaw += CarState.FrontWheelAng[0]*10430; // radians to Yaw
		JuggerArm1.SetRelativeRotation(TireArmRotation);
		TireArmRotation.Yaw += CarState.FrontWheelAng[1]*10430; //radians to Yaw
		JuggerArm2.SetRelativeRotation(TireArmRotation);
	}

}

simulated event Destroyed()
{
	//Log("XMPVehicle Destroyed");

	// Clean up random stuff attached to the car
	if(Level.NetMode != NM_DedicatedServer)

	{
		if (JuggerArm1?)
			JuggerArm1.Destroy();
		if (JuggerArm2?)
			JuggerArm2.Destroy();
	}
		// This will destroy wheels & joints
		Super.Destroyed();
}

function SetTeam( int NewTeam )
{
	Super.SetTeam( NewTeam );
	if (JuggerArm1?)
		class'UtilGame'.static.SetTeamSkin( JuggerArm1, NewTeam );
	if (JuggerArm2?)
		class'UtilGame'.static.SetTeamSkin( JuggerArm2, NewTeam );
	if (JuggernautTurretBarrelBase(Turrets[1].Barrel).TurretBarrel?)
		class'UtilGame'.static.SetTeamSkin( JuggernautTurretBarrelBase(Turrets[1].Barrel).TurretBarrel, NewTeam );

}

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.Juggernaut.JuggernautBody_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.Juggernaut.JuggernautDash_TD_001'
	MaxPassengers=2
	TurretClass(0)=class'JuggernautFrontTurret'
	TurretClass(1)=class'JuggernautTurret'

	CabClass=class'JuggernautCab'

    Begin Object Class=KarmaParamsRBFull Name=KParams0
        KInertiaTensor(0)=20
        KInertiaTensor(1)=0
        KInertiaTensor(2)=0
        KInertiaTensor(3)=30
        KInertiaTensor(4)=0
        KInertiaTensor(5)=48
		KCOMOffset=(X=.3,Y=0.0,Z=-0.7)
		KFriction=1.6
		KLinearDamping=0
		KAngularDamping=0
		bKNonSphericalInertia=False
        bHighDetailOnly=False
        bClientOnly=False
		bKDoubleTickRate=True
        Name="KParams0"
    End Object
    KParams=KarmaParams'KParams0'

	SprintImpulseOffset=(X=-15,Y=0,Z=-35)

	DrawScale=1

	FrontTireClass=class'JuggernautTire'
	RearTireClass=class'JuggernautTire'

	WheelFrontAlong=5.68
	WheelFrontAcross=-167.59
	WheelRearAlong=309.15
	WheelRearAcross=-143.17
	WheelVert=-83.89

	// Sounds

	// Driver positions
	
	ExitPositions(0)=(X=0,Y=300,Z=100)
	ExitPositions(1)=(X=0,Y=-300,Z=100)
	ExitPositions(2)=(X=350,Y=300,Z=100)
	ExitPositions(3)=(X=-350,Y=300,Z=100)
	ExitPositions(4)=(X=350,Y=-300,Z=100)
	ExitPositions(5)=(X=-350,Y=-300,Z=100)

    OutputBrake=1
    Steering=0
    Throttle=0
    Gear=1
	StopThreshold=40
	EngineBraking=true

    MaxSteerAngle=3400
    MaxBrakeTorque=200

	TorqueCurve=(Points=((InVal=0,OutVal=2800),(InVal=30000,OutVal=6400),(InVal=60000,OutVal=0)))
    TorqueSplit=0.2
    SteerPropGap=500
    SteerTorque=15000
    SteerSpeed=20000
	MaxWheelSpeed=88000

    TireMass=3.5    
    ChassisMass=12

    TireRollFriction=2.5
    TireLateralFriction=1.5
    TireRollSlip=0.06
    TireLateralSlip=0.04
    TireMinSlip=0.001
    TireSlipRate=0.007
    TireSoftness=0.0
    TireAdhesion=0
    TireRestitution=0

	HandbrakeThresh=1000
	TireHandbrakeSlip=0.2
	TireHandbrakeFriction=-1.2

    SuspStiffness=150
    SuspDamping=14.5
    SuspHighLimit=0.5
    SuspLowLimit=-0.5
    SuspRef=0

	AirSpeed = 10000

	Health=999
    
    UntilNextImpact=0

	EnginePitchScale=1655350
	SquealVelThresh=15
	SoundRadius=255
	XMPVehicleStartSound=Sound'VehiclesA.Juggernaut.JuggernautStartup'
	XMPVehicleIdleSound=Sound'VehiclesA.Juggernaut.JuggernautIdle'
	XMPVehicleSquealSound=Sound'VehiclesA.Generic.Skid'
	XMPVehicleHitSound=Sound'VehiclesA.Generic.Crash'

	VehicleHuskClass=class'JuggernautHusk'

	SprintForce=2000
	SprintTime=0.3

	AlternateSkins(0)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Juggernaut.JuggernautFX_TD_01_Blue'
}