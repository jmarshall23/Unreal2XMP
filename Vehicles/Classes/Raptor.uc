
class Raptor extends XMPVehicle
	    placeable;

var		RaptorGrinder Grinder1, Grinder2;
var		TurretCam	GrinderBase1, GrinderBase2;
var		ParticleGenerator GutBlowA,GutBlowB,GutBlowC;
var		Vector	GutBlowALocation;
var		Rotator	GutBlowARotation;
var		ParticleGenerator GutBlowAEffect;
var		Vector	GutBlowBLocation;
var		Rotator	GutBlowBRotation;
var		ParticleGenerator GutBlowBEffect;
var		Vector	GutBlowCLocation;
var		Rotator	GutBlowCRotation;
var		ParticleGenerator GutBlowCEffect;
var		float	GrinderDelta;
var		byte GutIndex,LastGutIndex;
replication
{
unreliable if( bNetDirty && Role==ROLE_Authority )
		GutIndex;
}

function Smack( actor Other )
{
	local vector EnemyVector;

	if ( Pawn(Other) != None )
	{
		if ( Pawn(Other).GetTeam() != GetTeam() && Role == ROLE_Authority && Pawn(Other).Health > 0 && Other.bHasBlood)
		{
		EnemyVector = Normal( Other.Location - Location );
			if (EnemyVector dot vector(Rotation) < -0.89)
			{
				GutIndex++;
				Other.bHidden=true;
				Other.TakeDamage(
					3000
					, Self
					, Other.Location
					, vect(0,0,0)
					, class'Crushed');
			}
		} else
			Super.Smack(Other);
	}
}


simulated function PostNetBeginPlay()
{
	Super.PostNetBeginPlay();
	LastGutIndex = GutIndex;
}

simulated function PostBeginPlay()
{
	local rotator GrinderRotation;
    Super.PostBeginPlay();
	if (bPendingDelete)
		return;
//only have grinders on client
	if(Level.NetMode != NM_DedicatedServer)
	{	
		GutBlowA = class'ParticleSalamander'.static.CreateNew( self, GutBlowAEffect, Location );
		GutBlowA.SetBase(Self);
		GutBlowA.IdleTime=0.0;
		GutBlowB = class'ParticleSalamander'.static.CreateNew( self, GutBlowBEffect, Location );
		GutBlowB.SetBase(Self);
		GutBlowB.IdleTime=0.0;
		GutBlowC = class'ParticleSalamander'.static.CreateNew( self, GutBlowCEffect, Location );
		GutBlowC.SetBase(Self);
		GutBlowC.IdleTime=0.0;
		GrinderRotation=Rotation;
		GrinderRotation.Pitch-=6918;
		GrinderBase1 = spawn (class'TurretCam', self,, Location + (class'RaptorGrinder'.default.LocationOffset >> Rotation),GrinderRotation);
		GrinderBase1.SetBase(self);
		Grinder1 = spawn (class'RaptorGrinder', self,, Location + (class'RaptorGrinder'.default.LocationOffset >> Rotation),GrinderRotation);
		Grinder1.SetBase(GrinderBase1);
		Grinder1.MySpin.Yaw=0;
		Grinder1.MySpin.Pitch=0;
		Grinder1.MySpin.Roll=0;
		GrinderBase2 = spawn (class'TurretCam', self,, Location + (class'RaptorGrinder'.default.LocationOffset2 >> Rotation),GrinderRotation);
		GrinderBase2.SetBase(self);
		Grinder2 = spawn (class'RaptorGrinder', self,, Location + (class'RaptorGrinder'.default.LocationOffset2 >> Rotation),GrinderRotation);
		Grinder2.SetBase(GrinderBase2);
		Grinder2.MySpin.Yaw=4096;//2730;
		Grinder2.MySpin.Pitch=0;
		Grinder2.MySpin.Roll=0;
		GrinderDelta=0;
	}
}

simulated function Tick(float Delta)
{
	super.Tick(Delta);

	if( GutIndex != LastGutIndex )
	{
		LastGutIndex = GutIndex;
		if (GutBlowA?){
			GutBlowA.SetRelativeRotation(GutBlowARotation);
			GutBlowA.SetRelativeLocation(GutBlowALocation);
			GutBlowA.Trigger(self,Instigator);
		}
		if (GutBlowB?)
		{
			GutBlowB.SetRelativeRotation(GutBlowBRotation);
			GutBlowB.SetRelativeLocation(GutBlowBLocation);
			GutBlowB.Trigger(self,Instigator);
		}
		if (GutBlowC?)
		{
			GutBlowC.SetRelativeRotation(GutBlowCRotation);
			GutBlowC.SetRelativeLocation(GutBlowCLocation);
			GutBlowC.Trigger(self,Instigator);
		}
	}
	//Rotate the grinders

	if (WheelSpinSpeed != 0 && Level.NetMode != NM_DedicatedServer) //only bother rotating if the vehicle is moving
	{
		GrinderDelta+=Delta;
		if (GrinderDelta > 0.01666) //rotate a maximum of sixty times per second
		{
			Grinder1.mySpin.Yaw+= Min((GrinderDelta * WheelSpinSpeed),GrinderDelta * 131072);// * abs(forwardvel)/forwardvel;
			Grinder2.mySpin.Yaw-= Min((GrinderDelta * WheelSpinSpeed),GrinderDelta * 131072);// * abs(forwardvel)/forwardvel;
			Grinder1.setRelativeRotation(Grinder1.MySpin);
			Grinder2.setRelativeRotation(Grinder2.MySpin);
			GrinderDelta=0;
		}
	} 
}

simulated event Destroyed()
{
	//Log("XMPVehicle Destroyed");

	// Clean up random stuff attached to the car
	if(Level.NetMode != NM_DedicatedServer)

	{
		if (GutBlowA?)
		{
			GutBlowA.ParticleDestroy();
			GutBlowA = None;
		}
		if (GutBlowB?)
		{
			GutBlowB.ParticleDestroy();
			GutBlowB = None;
		}
		if (GutBlowC?)
		{
			GutBlowC.ParticleDestroy();
			GutBlowC = None;
		}
		if (Grinder1?)
			Grinder1.Destroy();
		if (Grinder2?)
			Grinder2.Destroy();
		if (GrinderBase1?)
			GrinderBase1.Destroy();
		if (GrinderBase2?)
			GrinderBase2.Destroy();
	}
		// This will destroy wheels & joints
		Super.Destroyed();
}


defaultproperties
{
	EngineBraking=true
	MaxPassengers=2
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorBody_TD_001'
	DriverViewMesh=StaticMesh'VehiclesM.RaptorDash_TD_001'
	TurretClass(0)=class'RaptorDriverTurret'
	TurretClass(1)=class'RaptorTurret'
	CabClass=class'RaptorCab'
    Begin Object Class=KarmaParamsRBFull Name=KParams0
        KInertiaTensor(0)=20
        KInertiaTensor(1)=0
        KInertiaTensor(2)=0
        KInertiaTensor(3)=30
        KInertiaTensor(4)=0
        KInertiaTensor(5)=48
		KCOMOffset=(X=0.05,Y=0.0,Z=-2.7)
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

	SprintImpulseOffset=(X=-2.5,Y=0,Z=-135)

	DrawScale=1

	FrontTireClass=class'RaptorTire'
	RearTireClass=class'RaptorTire'

	WheelFrontAlong=-106.26
	WheelFrontAcross=-96.03
	WheelRearAlong=139.92
	WheelRearAcross=-96.03
	WheelVert=-82.04

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

//    MaxSteerAngle=3400
	MaxSteerAngle=4400
    MaxBrakeTorque=120

	TorqueCurve=(Points=((InVal=0,OutVal=270),(InVal=200000,OutVal=670),(InVal=300000,OutVal=0)))
    TorqueSplit=0.4
    SteerPropGap=500
    SteerTorque=25000
    SteerSpeed=30000

    TireMass=1.5    
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
	//TireHandbrakeSlip=0.2
	//TireHandbrakeFriction=-1.2
	TireHandbrakeSlip=0.6
	TireHandbrakeFriction=-3.2

    SuspStiffness=150
    SuspDamping=24.5
    SuspHighLimit=0.5
    SuspLowLimit=-0.5
    SuspRef=0

	AirSpeed = 10000

	Health=400
    
    UntilNextImpact=0

	AlternateSkins(0)=Shader'VehiclesT.Raptor.RaptorFX_TD_01_Red'
	AlternateSkins(1)=Shader'VehiclesT.Raptor.RaptorFX_TD_01_Blue'

	EnginePitchScale=1655350
	SquealVelThresh=25
	SoundRadius=255
	XMPVehicleStartSound=Sound'VehiclesA.Raptor.RaptorStartup'
	XMPVehicleIdleSound=Sound'VehiclesA.Raptor.RaptorIdle'
	XMPVehicleSquealSound=Sound'VehiclesA.Generic.Skid'
	XMPVehicleHitSound=Sound'VehiclesA.Generic.Crash'

	VehicleHuskClass=class'RaptorHusk'

	SprintForce=1500
	SprintTime=0.5
	GutBlowAEffect=ParticleGenerator'Vehicle_GutBlower_FX.ParticleSalamander1'
	GutBlowALocation=(X=-161.97, Y=0, Z=-63.91)
	GutBlowARotation=(Yaw=0,Pitch=0,Roll=0)
	GutBlowBLocation=(X=70, Y=-120, Z=-44)
	GutBlowBRotation=(Yaw=-4000,Pitch=4000,Roll=0)
	GutBlowBEffect=ParticleGenerator'Vehicle_GutBlower_FX.ParticleSalamander0'
	GutBlowCLocation=(X=70, Y=120, Z=-44)
	GutBlowCRotation=(Yaw=4000,Pitch=4000,Roll=0)
	GutBlowCEffect=ParticleGenerator'Vehicle_GutBlower_FX.ParticleSalamander0'

}