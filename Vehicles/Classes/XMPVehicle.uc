#exec OBJ LOAD FILE=..\sounds\VehiclesA.uax
class XMPVehicle extends KCar
	abstract
    notplaceable;

var() class<DamageFilter> DamageFilterClass;

// Effect spawned when vehicle is destroyed
var (KVehicle) ParticleGenerator	VehicleExplosionType;
var				int							ExplosionDamage;
//Dust Effects
var				bool				OldFrontLeftOnGround;
var				bool				OldFrontRightOnGround;
var				bool				OldRearLeftOnGround;
var				bool				OldRearRightOnGround;
var				ParticleSalamander	FrontLeftDust;
var				ParticleSalamander	FrontRightDust;
var				ParticleSalamander	RearLeftDust;
var				ParticleSalamander	RearRightDust;
var				ParticleSalamander	FrontLeftDustClass;
var				ParticleSalamander	FrontRightDustClass;
var				ParticleSalamander	RearLeftDustClass;
var				ParticleSalamander	RearRightDustClass;
var				int					VelocityThreshold;
var				float				DustVolumeMax;

var() class<VehicleHusk> VehicleHuskClass;

// Used to prevent triggering sounds continuously
var float			UntilNextImpact;
// XMPVehicle sound things
var (XMPVehicle) float	EnginePitchScale;
var (XMPVehicle) sound	XMPVehicleStartSound;
var (XMPVehicle) sound	XMPVehicleIdleSound;
var (XMPVehicle) float	HitSoundThreshold;
var (XMPVehicle) sound	XMPVehicleSquealSound;
var (XMPVehicle) float	SquealVelThresh;
var (XMPVehicle) sound	XMPVehicleHitSound;

var (XMPVehicle) String XMPVehicleStartForce;
var (XMPVehicle) String XMPVehicleIdleForce;
var (XMPVehicle) String XMPVehicleSquealForce;
var (XMPVehicle) String XMPVehicleHitForce;
var		float	InvertedDamagePerSec;
var		float	InvertedDamageDelta;
var		int		HitThreshold;
var		class<XMPVehicleCab>	CabClass;
var		XMPVehicleCab		MyCab;
var		Rotator		OldTurretRotation[HardMaxPassengers];
var		bool	TakingInvertedDamage;

//collision damage
var		Actor	CollisionTarget;
var		Int		PendingCollisionDamage;

// smoke effect
var()	ParticleSalamander SmokeEffectTemplate;
var()	float SmokeVolumeMin,SmokeVolumeMax;
var		ParticleSalamander SmokeEffect;

replication
{
	reliable if(Role==ROLE_Authority && bNetInitial)
		MyCab;
}

function KDriverEnter(Pawn p, int PassengerNum)
{
	Super.KDriverEnter(p, PassengerNum);

	if (PassengerNum == 0)
	{
		PlaySound(XMPVehicleStartSound, SLOT_None, 255, , 128 );
		AmbientSound=XMPVehicleIdleSound;
	}
	OnLine = XMPGame(Level.Game).IsTeamFeatureOnline( 'XMPVehicle', GetTeam() );
}

function bool KDriverLeave(int PassengerNumber, bool bForceLeave)
{
	local bool success;

	success = Super.KdriverLeave(PassengerNumber, bForceLeave);
	if (PassengerNumber == 0)
		AmbientSound = None;
	return success;
}

function OnUse (Actor User)
{
	//TODO Repair/Resupply
	//TryToDrive(Controller(User).Pawn, 0);
}

function SetUsability (int PassengerNum)
{
	if( PassengerNum == 0 )
		MyCab.bUsable = false;
	else if (Turrets[PassengerNum].Barrel != None && Turrets[PassengerNum].Barrel.bUseMe)
		Turrets[PassengerNum].Barrel.bUsable=false;
	else
		Turrets[PassengerNum].bUsable=false;
}

function UnsetUsability (int PassengerNumber)
{
	if( PassengerNumber == 0 )
		MyCab.bUsable=true;
	else if (Turrets[PassengerNumber].Barrel != None && Turrets[PassengerNumber].Barrel.bUseMe)
		Turrets[PassengerNumber].Barrel.bUsable=true; 
	else 
		Turrets[PassengerNumber].bUsable=true;	
}

function SetTeam( int NewTeam )
{
	Super.SetTeam( NewTeam );

	if (MyCab?)
		class'UtilGame'.static.SetTeamSkin( MyCab, NewTeam );

}
function PostBeginPlay()
{
	local int i,j;
    Super.PostBeginPlay();
	if (bPendingDelete)
		return;
    ConglomerateID=self;
	if (Role==ROLE_Authority)
	{	

		MyCab = spawn(CabClass, self,, Location, Rotation);
		MyCab.setBase(self);
		MyCab.ConglomerateID=self;
		MyCab.NetPriority = Self.NetPriority - 0.1;
		Self.NetDependents.Insert(0,1);
		Self.NetDependents[0] = MyCab;

		for (i=0; i<MaxPassengers; i++)
		{
			Turrets[i] = spawn(TurretClass[i], self,, Location + (TurretClass[i].default.TurretOffset >> Rotation));
			Turrets[i].SetBase(Self);
			Turrets[i].SetRelativeLocation(TurretClass[i].default.TurretOffset);
			Turrets[i].ConglomerateID=self;
			Turrets[i].NetPriority = Self.NetPriority - 0.05;
			TurretRotation[i] = TurretClass[i].default.RotationCenter;
			InitialTurretRotation[i] = TurretRotation[i];
			OldTurretRotation[i] = TurretRotation[i];
			Turrets[i].SetCollision(true,false,false);
			Self.NetDependents.Insert(0,1);
			Self.NetDependents[0] = Turrets[i];
			if (Turrets[i].bArticulated)
			{
				Turrets[i].Barrel = spawn(Turrets[i].GunBarrelClass, Turrets[i],, Turrets[i].Location + (Turrets[i].GunBarrelClass.default.BarrelOffset >> Turrets[i].Rotation) , Turrets[i].Rotation );
				Turrets[i].Barrel.SetBase(Turrets[i]);
				Turrets[i].Barrel.MyTurret = Turrets[i];
				Turrets[i].Barrel.bReplicateMovement=false;
				Turrets[i].Barrel.SetCollision(true,false,false);
				Turrets[i].Barrel.SetRelativeLocation(Turrets[i].GunBarrelClass.default.BarrelOffset);
				Turrets[i].Barrel.ConglomerateID = ConglomerateID;
				Turrets[i].Barrel.SetOwner(self);
				Turrets[i].Barrel.BarrelNumber=i;
				Turrets[i].Barrel.NetPriority = Self.NetPriority - 0.07;
				Self.NetDependents.Insert(0,1);
				Self.NetDependents[0] = Turrets[i].Barrel;
				if (i==0)
					Turrets[i].Barrel.Instigator = Self;
			}
			Turrets[i].TurretNumber=i;
			Turrets[i].RotateTurret (TurretRotation[i]);
			Turrets[i].SetNetDirty(true);
			if (Turrets[i].Barrel?)
				Turrets[i].Barrel.SetNetDirty(true);
		}
		SetNetDirty(true);
		for (i=0; i<Self.NetDependents.Length; i++)
		{
			if (Self.NetDependents[i] != MyCab)
			{
				MyCab.NetDependents.Insert(0,1);
				MyCab.NetDependents[0] = Self.NetDependents[i];
			}
		}
		MyCab.NetDependents.Insert(0,1);
		MyCab.NetDependents[0] = Self;
		for (j=0; j<MaxPassengers; j++)
		{
			for (i=0; i<Self.NetDependents.Length; i++)
			{
				if (Self.NetDependents[i] != Turrets[j])
				{
					Turrets[j].NetDependents.Insert(0,1);
					Turrets[j].NetDependents[0] = Self.NetDependents[i];
				}
				if (Turrets[j].bArticulated)
				{
					if (Self.NetDependents[j] != Turrets[j].Barrel)
					{
						Turrets[j].Barrel.NetDependents.Insert(0,1);
						Turrets[j].Barrel.NetDependents[0] = Self.NetDependents[i];
					}
				}
			}
			Turrets[j].NetDependents.Insert(0,1);
			Turrets[j].NetDependents[0] = Self;
			if (Turrets[j].bArticulated)
			{
				Turrets[j].Barrel.NetDependents.Insert(0,1);
				Turrets[j].Barrel.NetDependents[0] = Self;
			}
		}
		InvertedDamageDelta=0;
		TakingInvertedDamage=false;
	}
}

simulated function PostNetBeginPlay()
{
	local int i;
	Super.PostNetBeginPlay();
	KSetImpactThreshold(HitThreshold);

	if(Role != ROLE_Authority)
	{
		KarmaParams(KParams).bDestroyOnSimError = False;
		KarmaParams(frontLeft.KParams).bDestroyOnSimError = False;
		KarmaParams(frontRight.KParams).bDestroyOnSimError = False;
		KarmaParams(rearLeft.KParams).bDestroyOnSimError = False;
		KarmaParams(rearRight.KParams).bDestroyOnSimError = False;

		for (i=0; i<MaxPassengers; i++)
		{
			if (Turrets[i]?)
			{
				if (Turrets[i].ConglomerateID != Self)
					Turrets[i].ConglomerateID = Self;
				if (Turrets[i].Base != Self)
					Turrets[i].SetBase(Self);
				if (Turrets[i].RelativeLocation != Turrets[i].TurretOffset)
				{
					Turrets[i].SetBase(Self);
					Turrets[i].SetRelativeLocation(Turrets[i].TurretOffset);
				}
				if (Turrets[i].Barrel?)
				{
					if (Turrets[i].Barrel.MyTurret != Turrets[i])
						Turrets[i].Barrel.MyTurret = Turrets[i];
					if (Turrets[i].Barrel.Base != Turrets[i])
						Turrets[i].Barrel.SetBase(Turrets[i]);
					if (Turrets[i].Barrel.RelativeLocation != Turrets[i].Barrel.BarrelOffset)
					{
						Turrets[i].Barrel.SetBase(Turrets[i]);
						Turrets[i].Barrel.SetRelativeLocation(Turrets[i].Barrel.BarrelOffset);
					}					
					if (Turrets[i].Barrel.ConglomerateID != Self)
						Turrets[i].Barrel.ConglomerateID = Self;
					Turrets[i].RotateTurret(TurretRotation[i]);
				}
			}
		}
		if (MyCab? && MyCab.Base != Self)
		{
			MyCab.SetBase(Self);
			MyCab.SetRelativeLocation(vect(0,0,0));
		}
	}
}

simulated event PostNetReceive()
{
	local int i;
	for (i=0; i<MaxPassengers; i++)
	{
		if (Turrets[i]?)
		{
			if (Turrets[i].ConglomerateID != Self)
				Turrets[i].ConglomerateID = Self;
			if (Turrets[i].Base != Self)
				Turrets[i].SetBase(Self);
			if (Turrets[i].RelativeLocation != Turrets[i].TurretOffset)
			{
				Turrets[i].SetBase(Self);
				Turrets[i].SetRelativeLocation(Turrets[i].TurretOffset);
			}
			if (Turrets[i].Barrel?)
			{
				if (Turrets[i].Barrel.MyTurret != Turrets[i])
					Turrets[i].Barrel.MyTurret = Turrets[i];
				if (Turrets[i].Barrel.Base != Turrets[i])
					Turrets[i].Barrel.SetBase(Turrets[i]);
				if (Turrets[i].Barrel.RelativeLocation != Turrets[i].Barrel.BarrelOffset)
				{
					Turrets[i].Barrel.SetBase(Turrets[i]);
					Turrets[i].Barrel.SetRelativeLocation(Turrets[i].Barrel.BarrelOffset);
				}					
				if (Turrets[i].Barrel.ConglomerateID != Self)
					Turrets[i].Barrel.ConglomerateID = Self;
				Turrets[i].RotateTurret(TurretRotation[i]);
			}
		}
	}
	if (MyCab? && MyCab.Base != Self)
	{
		MyCab.SetBase(Self);
		MyCab.SetRelativeLocation(vect(0,0,0));
	}
}

simulated event Destroyed()
{
	local int i;
	if(Level.NetMode != NM_Client)
	{
		for (i=0; Turrets[i]?;i++)
			Turrets[i].Destroy();
		if (MyCab?)
			MyCab.Destroy();
	}
	if( SmokeEffect? )
	{
		SmokeEffect.ParticleDestroy();
		SmokeEffect = None;
	}
	if( FrontLeftDust? )
	{
		FrontLeftDust.ParticleDestroy();
		FrontLeftDust = None;
	}
	if( FrontRightDust? )
	{
		FrontRightDust.ParticleDestroy();
		FrontRightDust = None;
	}
	if( RearLeftDust? )
	{
		RearLeftDust.ParticleDestroy();
		RearLeftDust = None;
	}
	if( RearRightDust? )
	{
		RearRightDust.ParticleDestroy();
		RearRightDust = None;
	}

	Super.Destroyed();
}

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
//	local VehicleExplosion MyExplosion;
    local int i;
	local PlayerController pc;
	local Pawn PCPawn;
	local int ActualDamage;

	if( Role < ROLE_Authority )
		return;

	if (Health >0)
	{
		if( DamageFilterClass? )
		 	DamageFilterClass.static.ApplyFilter( Damage, momentum, DamageType );

		//do reducedamage so that team-based damage filtering gets utilized
		actualDamage = Level.Game.ReduceDamage(Damage, self, instigatedBy, HitLocation, Momentum, DamageType);
		CalcHitIndication( actualDamage, HitLocation, Momentum );
		Health -= ActualDamage;
		if(Health <= 0)
		{

		// If there were passengers in the vehicle, destroy them too
		for (i=0;i < MaxPassengers; i++)
		{
			if ( PassengerControllers[i] != None )
			{
				pc = PlayerController(PassengerControllers[i]);
				PCPawn = PassengerPawns[i];
				KDriverLeave(i, true);
				if (!PCPawn.bPendingDelete)
					PCPawn.TakeDamage(3000, instigatedBy , PCPawn.Location , (Momentum * 20), damageType );
			}
		}
		Instigator = instigatedBy;

		HurtRadius(ExplosionDamage,512, DamageType, float(ExplosionDamage*200), Self.Location );

		if( VehicleHuskClass? )
			Spawn(VehicleHuskClass,Self,,Location+vect(0,0,64),Rotation);
/*OLD
			MyExplosion =  spawn(class'VehicleExplosion', self,, Location, Rotation);
			MyExplosion.VehicleExplosionType = VehicleExplosionType;
*/
			Destroy(); // Destroy the vehicle itself (see Destroyed below)
		}
		if (instigatedBy != Self && !TakingInvertedDamage)
			KAddImpulse(momentum, hitlocation);
	}
}

function KImpact(actor other, vector pos, vector impactVel, vector impactNorm)
{
	local float DirectionalImpact;
	local float VehicleDirectionalImpact;
	local vector NormalizedVel;
	local vector NormalizedVehicleOrientation;

    if(UntilNextImpact < 0)
    {
		NormalizedVel = Normal(impactVel);
		NormalizedVehicleOrientation = Normal(-vector(Rotation));
		DirectionalImpact = NormalizedVel dot impactNorm;
		VehicleDirectionalImpact = NormalizedVehicleOrientation dot impactNorm;
		
		if (level.netmode != NM_Client)
		{
			if (Other? && Other.ConglomerateID?)
			{
				UntilNextImpact=1;
				PlaySound(XMPVehicleHitSound, SLOT_None, 255,,128 );
				CollisionTarget = Other;
				PendingCollisionDamage = 2 * DirectionalImpact*Vsize(impactVel);
				if (PendingCollisionDamage < 200)
					PendingCollisionDamage=200;
			} else if (abs(VehicleDirectionalImpact) > 0.8)
			{
				UntilNextImpact=1;
				PlaySound(XMPVehicleHitSound, SLOT_None, 255,,128 );
				CollisionTarget = Other;
				PendingCollisionDamage = DirectionalImpact*Vsize(impactVel);
			} else 
		}
    }
}

simulated function Tick(float Delta)
{
	local int HealthThreshold;
	local int i;
	local float EnginePitch;
    Super.Tick(Delta);
    UntilNextImpact -= Delta;

//Dust Effects
	// Smoke if damaged badly.
	if( !bPendingDelete && Level.NetMode!=NM_DedicatedServer )
	{

		if( !frontLeftDust )
		{
			frontLeftDust = ParticleSalamander(class'ParticleGenerator'.static.CreateNew( frontLeft, frontLeftDustClass, frontLeft.Location ));
			frontLeftDust.SetBase( frontLeft );
			frontLeftDust.TurnOn();
		}
		frontLeftDust.Volume = FMax(0,Blend( 0, DustVolumeMax, ABlend( VelocityThreshold, 500, Vsize(Velocity) ) ));

		if( !FrontRightDust )
		{
			FrontRightDust = ParticleSalamander(class'ParticleGenerator'.static.CreateNew( FrontRight, FrontRightDustClass, FrontRight.Location ));
			FrontRightDust.SetBase( FrontRight );
			FrontRightDust.TurnOn();
		}
		FrontRightDust.Volume = FMax(0,Blend( 0, DustVolumeMax, ABlend( VelocityThreshold, 500, Vsize(Velocity) ) ));

		if( !RearLeftDust )
		{
			RearLeftDust = ParticleSalamander(class'ParticleGenerator'.static.CreateNew( RearLeft, RearLeftDustClass, RearLeft.Location ));
			RearLeftDust.SetBase( RearLeft );
			RearLeftDust.TurnOn();
		}
		RearLeftDust.Volume = FMax(0,Blend( 0, DustVolumeMax, ABlend( VelocityThreshold, 500, Vsize(Velocity) ) ));

		if( !RearRightDust )
		{
			RearRightDust = ParticleSalamander(class'ParticleGenerator'.static.CreateNew( RearRight, RearRightDustClass, RearRight.Location ));
			RearRightDust.SetBase( RearRight );
			RearRightDust.TurnOn();
		}
		RearRightDust.Volume = FMax(0,Blend( 0, DustVolumeMax, ABlend( VelocityThreshold, 500, Vsize(Velocity) ) ));
	}
	if (frontLeftDust? && !frontLeft.bTireOnGround)
		frontLeftDust.Volume = 0;
	if (frontRightDust? && !frontRight.bTireOnGround)
		frontRightDust.Volume = 0;
	if (rearLeftDust? && !rearLeft.bTireOnGround)
		rearLeftDust.Volume = 0;
	if (rearRightDust? && !rearRight.bTireOnGround)
		rearRightDust.Volume = 0;

	EnginePitch = 64 + ((WheelSpinSpeed/EnginePitchScale) * (255-64));
    SoundPitch = FClamp(EnginePitch, 0, 254);
    SoundVolume = 128;

	// Smoke if damaged badly.
	if( !bPendingDelete && Level.NetMode!=NM_DedicatedServer )
	{
		HealthThreshold = default.Health*0.3;
		if( Health < HealthThreshold )
		{
			if( !SmokeEffect )
			{
				SmokeEffect = ParticleSalamander(class'ParticleGenerator'.static.CreateNew( Self, SmokeEffectTemplate, Location ));
				SmokeEffect.SetBase( Self );
				SmokeEffect.TurnOn();
			}
			SmokeEffect.Volume = Blend( SmokeVolumeMin, SmokeVolumeMax, ABlend( HealthThreshold, 0, Health ) );
			SmokeEffect.MinVolume = SmokeEffect.Volume * 0.2;
		}
	}

//Removed squealing tires because it was lame

	//since we can't do damage within KImpact, must do it here
	if (level.netmode != NM_Client && PendingCollisionDamage != 0)
	{
		if (CollisionTarget? && CollisionTarget.ConglomerateID?)
		{
			takedamage ( PendingCollisionDamage, pawn(CollisionTarget.ConglomerateID), location, -velocity, class'Crushed' );
			CollisionTarget.takedamage ( PendingCollisionDamage, self, CollisionTarget.location, velocity, class'Crushed' );
		} else 
		{
			takedamage ( PendingCollisionDamage/2, none, location, -velocity, class'Crushed' );
		}
	PendingCollisionDamage=0;
	}

	if (!bIsInverted && TakingInvertedDamage)
		TakingInvertedDamage=false;

	if (Role == ROLE_Authority && bIsInverted && abs(forwardvel) < 100)
	{
		InvertedDamageDelta += Delta;
		if (InvertedDamageDelta >= 1 && TakingInvertedDamage)
		{
			takedamage ( InvertedDamagePerSec , Self, Location, vect(0,0,0), class'Crushed' );
			InvertedDamageDelta -= 1;
		} else if (InvertedDamageDelta >=4)
		{
			TakingInvertedDamage=true;
			InvertedDamageDelta=0;
		}
	}

	for (i=0; Turrets[i] != None; i++)
		{
			if (Role < ROLE_Authority && MyPassengerNumber == i)
			{
				if (OldTurretRotation[MyPassengerNumber] != MyTurretRotation)
				{
					Turrets[MyPassengerNumber].RotateTurret(MyTurretRotation, OldTurretRotation[MyPassengerNumber]);
					OldTurretRotation[MyPassengerNumber] = MyTurretRotation;
				}
			} else if (OldTurretRotation[i] != TurretRotation[i]) 
			{
				Turrets[i].RotateTurret(TurretRotation[i], OldTurretRotation[i]);
				OldTurretRotation[i] = TurretRotation[i];
			}
		}

}

//-----------------------------------------------------------------------------
// Team energy usage

function NotifyTeamEnergyStatus( bool bEnabled )	// actor has been shutdown or activated because of team energy
{
	OnLine=bEnabled;
}

function Fire( optional float F )
{
    if( Turrets[0].Barrel? )
	{
		if (Turrets[0].Barrel.Weapon?)
			Turrets[0].Barrel.Weapon.Fire();
		else if (Turrets[0].Barrel.MyWeapon?)
			Turrets[0].Barrel.MyWeapon.Fire();
	}
}

// Special calc-view for vehicles
simulated function bool SpecialCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local PlayerController pc;

	if (Level.NetMode != NM_DedicatedServer)
	{
	pc = PlayerController  (Controller);
		if(pc == None )
		{
			DMTNS("Controller for vehicle is set to "$Controller$" and is not a PlayerController");
			return false;
		}

		if (!pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.bArticulated)
		{
			CameraRotation=pc.DrivenVehicle.Turrets[pc.PassengerNumber].Rotation;
			CameraRotation.Yaw+=32767;
			CameraRotation.Pitch= -(pc.DrivenVehicle.Turrets[pc.PassengerNumber].Rotation.Pitch);
			CameraRotation.Roll= -(pc.DrivenVehicle.Turrets[pc.PassengerNumber].Rotation.Roll);
			if (!pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.bRelativeRotation)
			{
				CameraLocation=Location+ ((TurretClass[pc.PassengerNumber].default.TurretOffset+TurretClass[pc.PassengerNumber].default.CameraOffset) >> Rotation);
			}
			else
				CameraLocation = pc.DrivenVehicle.Turrets[pc.PassengerNumber].Location + (pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.CameraOffset >> pc.DrivenVehicle.Turrets[pc.PassengerNumber].Rotation);

		} else
		{
			CameraRotation=pc.DrivenVehicle.Turrets[pc.PassengerNumber].barrel.Rotation;
			CameraRotation.Yaw+=32767;
			CameraRotation.Pitch= -(pc.DrivenVehicle.Turrets[pc.PassengerNumber].barrel.Rotation.Pitch);
			CameraRotation.Roll= -(pc.DrivenVehicle.Turrets[pc.PassengerNumber].barrel.Rotation.Roll);
		
			if (!pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.bRelativeRotation)
			{
				CameraLocation=Location+ ((TurretClass[pc.PassengerNumber].default.TurretOffset+TurretClass[pc.PassengerNumber].default.CameraOffset) >> Rotation);
			}
			else
				CameraLocation = pc.DrivenVehicle.Turrets[pc.PassengerNumber].barrel.Location + (pc.DrivenVehicle.TurretClass[pc.PassengerNumber].default.CameraOffset >> pc.DrivenVehicle.Turrets[pc.PassengerNumber].barrel.Rotation);
		}
		CameraLocation = CameraLocation + pc.ShakeOffset;
		return true;
	} else
	{
	return Super.SpecialCalcView(ViewActor,CameraLocation,CameraRotation );
	}
}

function PossessedBy(Controller C)
{
	Controller = C;
	NetPriority = 3;

	if ( C.PlayerReplicationInfo != None )
	{
		PlayerReplicationInfo = C.PlayerReplicationInfo;
		OwnerName = PlayerReplicationInfo.PlayerName;
	}
	if ( C.IsA('PlayerController') )
	{
		if ( Level.NetMode != NM_Standalone )
			RemoteRole = ROLE_AutonomousProxy;
			Turrets[0].RemoteRole = ROLE_AutonomousProxy;
			Turrets[0].SetOwner(C);
		BecomeViewTarget();
	}
	else
		RemoteRole = Default.RemoteRole;

	SetOwner(Controller);	// for network replication
	Eyeheight = BaseEyeHeight;
	ChangeAnimation();
}

function UnPossessed()
{	
	PlayerReplicationInfo = None;
	SetOwner(None);
	Turrets[0].SetOwner(Self);
	Controller = None;
}

simulated function ClientKDriverEnter(PlayerController pc, int PassengerNum, Pawn p)
{
	Super.ClientKDriverEnter(pc,PassengerNum,p);
	if (DriverViewMesh?)
		{
			MyCab.bHidden=true;
			BaseMesh=StaticMesh;
			SetStaticMesh(DriverViewMesh);
		}
}

simulated function ClientKDriverLeave(PlayerController pc, Rotator exitLookDir)
{
	Super.ClientKDriverLeave(pc, exitLookDir);
	if (DriverViewMesh?)
		{
		MyCab.bHidden=false;
		SetStaticMesh(BaseMesh);
		}
}

defaultproperties
{
	bUsable=false 
	InvertedDamagePerSec=100
	VehicleExplosionType=ParticleSalamander'Vehicle_Explosion_FX.ParticleSalamander0'
	ExplosionDamage=800
	HitThreshold=400
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorBody_TD_001'
    Begin Object Class=KarmaParamsRBFull Name=KParams0
        KInertiaTensor(0)=20
        KInertiaTensor(1)=0
        KInertiaTensor(2)=0
        KInertiaTensor(3)=30
        KInertiaTensor(4)=0
        KInertiaTensor(5)=48
		KCOMOffset=(X=0.8,Y=0.0,Z=-0.7)
		KStartEnabled=True
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

	DrawScale=1

	FrontTireClass=class'XMPVehicleTire'
	RearTireClass=class'XMPVehicleTire'

	WheelFrontAlong=-126.01
	WheelFrontAcross=-96.38
	WheelRearAlong=120.16
	WheelRearAcross=-96.38
	WheelVert=-38.74
	// Driver positions
	
	ExitPositions(0)=(X=0,Y=200,Z=100)
	ExitPositions(1)=(X=0,Y=-200,Z=100)
	ExitPositions(2)=(X=350,Y=0,Z=100)
	ExitPositions(3)=(X=-350,Y=0,Z=100)

    OutputBrake=1
    Steering=0
    Throttle=0
    Gear=1
	StopThreshold=40

    MaxSteerAngle=3400
    MaxBrakeTorque=55

	TorqueCurve=(Points=((InVal=0,OutVal=270),(InVal=200000,OutVal=270),(InVal=300000,OutVal=0)))
    TorqueSplit=0.5
    SteerPropGap=500
    SteerTorque=15000
    SteerSpeed=20000

    TireMass=0.5    
    ChassisMass=8

    TireRollFriction=1.5
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

	AirSpeed = 5000

	Health=900
    
    UntilNextImpact=0
	TurretClass(0)=class'XMPVehicleTurret'

	// Sounds
	EnginePitchScale=1655350
	SquealVelThresh=15
	SoundRadius=128
	XMPVehicleStartSound=Sound'VehiclesA.Raptor.RaptorStartup'
	XMPVehicleIdleSound=Sound'VehiclesA.Raptor.RaptorIdle'
	XMPVehicleSquealSound=Sound'VehiclesA.Generic.Skid'
	//XMPVehicleHitSound=Sound'VehiclesA.Generic.Crash'
	XMPVehicleHitSound=Sound'u2ambient2a.atlantiscrash.atlantis_debris_crash_2'
	//bAlwaysRelevant=true
	DamageFilterClass=class'DamageFilterVehicles'
	SmokeEffectTemplate=ParticleSalamander'Vehicle_Damage_FX.ParticleSalamander3'
	SmokeVolumeMin=1
	SmokeVolumeMax=40
	bNetNotify=true
	FrontLeftDustClass=ParticleSalamander'Vehicle_WheelDust_FX.ParticleSalamander0'
	FrontRightDustClass=ParticleSalamander'Vehicle_WheelDust_FX.ParticleSalamander0'
	RearLeftDustClass=ParticleSalamander'Vehicle_WheelDust_FX.ParticleSalamander0'
	RearRightDustClass=ParticleSalamander'Vehicle_WheelDust_FX.ParticleSalamander0'
	VelocityThreshold=100
	DustVolumeMax=90
}

