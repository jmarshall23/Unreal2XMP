
class Harbinger extends XMPVehicle
	    placeable;

var Sound	Honkey;
var Float	UntilNextHonk;
simulated event Destroyed()
{
	//Log("XMPVehicle Destroyed");
	// This will destroy wheels & joints
	Super.Destroyed();
}

function Fire( optional float F )
{
	if (UntilNextHonk <= 0)
	{
		PlaySound(Honkey, SLOT_None, 255, , 128 );
		UntilNextHonk = GetSoundDuration(Honkey);
	}
}
simulated function Tick(float Delta)
{
	if (UntilNextHonk > 0)
		UntilNextHonk -= Delta;
	MaxWheelSpeed = WheelSpinSpeed + 100000*Delta;
	frontLeft.WheelJoint.KMaxSpeed = MaxWheelSpeed;
	frontRight.WheelJoint.KMaxSpeed = MaxWheelSpeed;
	rearLeft.WheelJoint.KMaxSpeed = MaxWheelSpeed;
	rearRight.WheelJoint.KMaxSpeed = MaxWheelSpeed;

	super.Tick(Delta);
}

defaultproperties
{
	EngineBraking=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.HarbingerBody_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.Harbinger.HarbingerDash_TD_001'
	MaxPassengers=3
	TurretClass(0)=class'HarbingerDriverTurret'
	TurretClass(1)=class'HarbingerTurret'
	TurretClass(2)=class'HarbingerBubbleTurret'

	CabClass=class'HarbingerCab'

    Begin Object Class=KarmaParamsRBFull Name=KParams0
        KInertiaTensor(0)=20
        KInertiaTensor(1)=0
        KInertiaTensor(2)=0
        KInertiaTensor(3)=30
        KInertiaTensor(4)=0
        KInertiaTensor(5)=48
		KCOMOffset=(X=0.8,Y=0.0,Z=-2.7)
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

	SprintImpulseOffset=(X=-30,Y=0,Z=-115)

	DrawScale=1

	FrontTireClass=class'HarbingerTire'
	RearTireClass=class'HarbingerTire'

	WheelFrontAlong=17.17
	WheelFrontAcross=-101.69
	WheelRearAlong=253.24
	WheelRearAcross=-101.693
	WheelVert=-118.18

	// Sounds

	// Driver positions
	
	ExitPositions(0)=(X=0,Y=200,Z=100)
	ExitPositions(1)=(X=0,Y=-200,Z=100)
	ExitPositions(2)=(X=350,Y=200,Z=100)
	ExitPositions(3)=(X=-350,Y=200,Z=100)
	ExitPositions(4)=(X=350,Y=-200,Z=100)
	ExitPositions(5)=(X=-350,Y=-200,Z=100)

    OutputBrake=1
    Steering=0
    Throttle=0
    Gear=1
	StopThreshold=40

    MaxSteerAngle=3400
    MaxBrakeTorque=55

	TorqueCurve=(Points=((InVal=0,OutVal=3070),(InVal=80000,OutVal=2070),(InVal=160000,OutVal=0)))
    TorqueSplit=0.5
    SteerPropGap=500
    SteerTorque=15000
    SteerSpeed=20000
	MaxWheelSpeed=150000
    TireMass=2.5    
    ChassisMass=8

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
    SuspHighLimit=0.0
    SuspLowLimit=-1.0
    SuspRef=-0.5

	AirSpeed = 10000

	Health=800
   	UntilNextImpact=0

	EnginePitchScale=1655350
	SquealVelThresh=15
	SoundRadius=255
	XMPVehicleStartSound=Sound'VehiclesA.Harbinger.HarbingerStartup'
	XMPVehicleIdleSound=Sound'VehiclesA.Harbinger.HarbingerIdle'
	XMPVehicleSquealSound=Sound'VehiclesA.Generic.Skid'
	XMPVehicleHitSound=Sound'VehiclesA.Generic.Crash'

	VehicleHuskClass=class'HarbingerHusk'

	SprintForce=1500
	SprintTime=0.2

	AlternateSkins(0)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Harbinger.HarbingerFX_TD_01_Blue'

	Honkey=Sound'VehiclesA.Harbinger.HarbingerHorn'
}