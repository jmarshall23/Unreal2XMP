// Generic 'Stationary Turret' base class that can be controlled by a Pawn.

class StationaryTurret extends Pawn
    abstract;

// generic controls (set by controller, used by concrete derived classes)
var (StationaryTurret) float		Steering; // between -1 and 1
var (StationaryTurret) float		Throttle; // between -1 and 1
var				Pawn		Driver;
var				Controller	DriverController;
var				Rotator		TurretRotation;
var				Rotator		MyTurretRotation;
var (StationaryTurret) array<vector>	ExitPositions;		// Positions (rel to vehicle) to try putting the player when exiting.
// Effect spawned when vehicle is destroyed
var (StationaryTurret) class<Actor>	DestroyEffectClass;
var				Rotator		initialRotation;
var				Vector		initialLocation;

var		vector					TurretOffset;	
var	(StationaryTurret)	Rotator					RotationCenter;
var	(StationaryTurret)	int						PitchConstraint;
var	(StationaryTurret)	int						YawConstraint;
var		int						RateOfFire;
var		class<Actor>			ProjectileClass;		
var		vector					CameraOffset;
var		vector					Barrel1FiringOffset;
var		vector					Barrel2FiringOffset;
var		Rotator					OldRotation;
var		class<XMPVehicleBarrel>	GunBarrelClass;
var		XMPVehicleBarrel		Barrel;
var	(StationaryTurret)	int						MaxRotation;
var						bool					bRelativeRotation;
var (StationaryTurret)	bool	bProxyControlled;
var (StationaryTurret)	bool		bExternalView;
var						bool     bGetOut;
var						bool					bArticulated;
// Weapon system
var						bool	bTurretIsFiring, bTurretIsAltFiring;

var						bool	OnLine;

var						float	LastOccupiedTime;
var						int		LastOccupiedTeam;

var		float					LastTimeRotated;
var		sound					RotateStartSound;
var		sound					RotateSound;

replication
{
	reliable if(Role==ROLE_Authority)
		ClientKDriverEnter, ClientKDriverLeave;//, Resurrect, FeignDeath;
	
	reliable if(Role==ROLE_Authority)
		TurretRotation, Barrel, Throttle, OnLine;

	reliable if (Role==ROLE_Authority && bNetInitial)
		initialRotation,RotationCenter,PitchConstraint,YawConstraint,InitialLocation;
						
	reliable if(Role < ROLE_Authority)
		TurretFire, TurretCeaseFire;
}

//-----------------------------------------------------------------------------
// Team energy usage

function name GetWeaponBoneFor(Inventory I)
{
	return '';
}

function NotifyTeamEnergyStatus( bool bEnabled )	// actor has been shutdown or activated because of team energy
{
	OnLine=bEnabled;
}

function OnUse (Actor User)
{
	TryToDrive(Controller(User).Pawn);
}

function Fire( optional float F )
{
    if( Barrel.MyWeapon!=None )
        Barrel.Fire();
}

function SetTeam( int NewTeam )
{
	// cache the last occupied time / team
	if( GetTeam() != 255 )
	{
		LastOccupiedTeam = GetTeam();
		LastOccupiedTime = Level.TimeSeconds;
	}

	Super.SetTeam( NewTeam );
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );

	if (Barrel != None)
		class'UtilGame'.static.SetTeamSkin( Barrel, NewTeam );
}


simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if (bPendingDelete)
		return;
	if (Role == ROLE_Authority)
	{
		InitialLocation=Location;
		InitialRotation=Rotation;
	}
    ConglomerateID=self;
	if (Level.NetMode != NM_Client)
	{	
		Barrel = spawn(GunBarrelClass, self,, Location + (Gunbarrelclass.default.BarrelOffset >> Rotation));
		Barrel.SetBase(Self);
		Barrel.ConglomerateID=self;
		Barrel.SetOwner(self);
		Barrel.SetCollision(true,false,true);
		Barrel.Instigator=Self;
		Barrel.SetNetDirty(true);
		self.SetNetDirty(true);
		self.NetDependents.Insert(0,1);
		self.NetDependents[0] = Barrel;
		Barrel.NetDependents.Insert(0,1);
		Barrel.NetDependents[0] = self;
		Barrel.bAlwaysRelevant=true;
		TurretRotation=RotationCenter;
		RotateTurret (TurretRotation);
	}

}

// Really simple at the moment!
function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	local int ActualDamage;
	local Pawn PCPawn;
	
	//do reducedamage so that team-based damage filtering gets utilized
	actualDamage = Level.Game.ReduceDamage(Damage, self, instigatedBy, HitLocation, Momentum, DamageType);
	CalcHitIndication( actualDamage, HitLocation, Momentum );
	Health -= ActualDamage;
	if(Health <= 0)
	{
		if (Driver != None)
		{
			PCPawn=Driver;
			KDriverLeave(true);
			PCPawn.TakeDamage(3000, instigatedBy , PCPawn.Location , Momentum * 3, damageType );
		}
 //We'll never actually destroy the turret through damage
//		Destroy();
		FeignDeath();
	}

}

simulated function FeignDeath()
{
//Stub
}

simulated function Resurrect()
{
//Stub
}


simulated function RotateTurret( Rotator TurretRot )
{
	local rotator TurretBaseRotation;
	local rotator TurretGunRotation;
			
	if (!bArticulated )
	{
		SetRotation(TurretRot + initialRotation);
	} else 
	{
		TurretBaseRotation=TurretRot;
		TurretBaseRotation.Pitch=0;
		TurretBaseRotation.Roll=0;
		TurretGunRotation=TurretRot;
		TurretGunRotation.Roll=0;
		TurretGunRotation.Yaw=0;
		SetRotation(TurretBaseRotation + initialRotation);
		Barrel.SetRelativeRotation(TurretGunRotation);
		SetNetDirty(true);
	}
	//What we really need here is a start sound and a loopable ambient sound with an endsound attached to its end (which ends the sound)
	//We check evern n seconds (where n = the length of the looping portion) and restart the looping portion if the player has moved his mouse
/*
	if (level.NetMode != NM_DedicatedServer)
	{
		if (LastTimeRotated < (level.timeseconds-1))
			//it's been a while since we last rotated so we'll play the startrotating sound
		{
			if (RotateStartSound?)
				PlaySound(RotateStartSound, SLOT_Pain, 128,,128 );
		} else if (LastTimeRotated < (level.timeseconds - 0.1))
			//play the rotating sound
		{
			if (RotateSound?)
			PlaySound(RotateSound, SLOT_Pain, 128,,128 );
		}
		//otherwise we're still playing the last sound
		LastTimeRotated = level.timeseconds;
	}
*/
}

// Turrets dont get telefragged.
event EncroachedBy( actor Other )
{
	Log("StationaryTurret("$self$") Encroached By: "$Other$".");
}

// Do some server-side vehicle firing stuff
function TurretFire(bool bWasAltFire)
{
	if(bWasAltFire)
		bTurretIsAltFiring = true;
	else
		bTurretIsFiring = true;
}

function TurretCeaseFire(bool bWasAltFire)
{
	if(bWasAltFire)
		bTurretIsAltFiring = false;
	else
		bTurretIsFiring = false;
}


// The pawn Driver has tried to take control of this Turret
function TryToDrive(Pawn p)
{
	local Controller C;
	C = p.Controller;

    if ( (Driver == None) && (C != None) && C.bIsPlayer && !C.IsInState('PlayerTurreting') && p.IsHumanControlled() )
	{        
		KDriverEnter(p);
    }
}

// Events called on driver entering/leaving vehicle

simulated function ClientKDriverEnter(PlayerController pc, Pawn P)
{
	p.GotoState('Passenger');
	p.Weapon.PutDown();
	pc.bBehindView = true;
	pc.bFixedCamera = false;
	pc.bFreeCamera = true;
    pc.TempTurretRotation=TurretRotation;
	pc.setbase(barrel);
	Barrel.Controller=pc;
	Barrel.SetOwner(pc);
	Controller=pc;
	if (Barrel.Myweapon?)
	{	
		if( Barrel.Myweapon.CrossHair? )
			pc.SendEvent(Barrel.Myweapon.CrossHair);
	Barrel.MyWeapon.Instigator = Barrel;
	if (Barrel.MyWeapon.ThirdPersonActor?)
		Barrel.MyWeapon.ThirdPersonActor.Instigator = Barrel;
	}
	if (Role<ROLE_Authority)
	{
		Barrel.Weapon=Barrel.MyWeapon;
		if (Barrel.MyAmmoType?)
		{
			Barrel.Weapon.AmmoType=Barrel.MyAmmoType;
			Barrel.Weapon.AmmoTypes[0]=Barrel.MyAmmoType;
		}
	}
}

function KDriverEnter(Pawn p)
{
	local PlayerController pc;
	local Inventory Inv;

	pc = PlayerController(p.Controller);
	DriverController = pc;
	Driver = p;
	Self.SetTeam(pc.GetTeam());
	Barrel.SetTeam(pc.GetTeam());
	// Move the driver into position, and attach to car.
	Driver.SetCollision(false, false, false);
	//Driver.bCollideWorld = false;
	Driver.bPhysicsAnimUpdate = false;
	Driver.Velocity = vect(0,0,0);
	Driver.SetPhysics(PHYS_None);
	Driver.bHidden= True;
	Driver.GotoState('Passenger');
	Driver.weapon.putdown();
	// Disconnect PlayerController from PlayerPawn and connect to StationaryTurret.
	pc.Unpossess();
	Driver.SetOwner(pc); // This keeps the driver relevant.
	//pc.SetRotation(CameraRotator);
	//pc.setLocation(TurretCamera.Location);
	pc.Possess(self);
	pc.ClientSetViewTarget(barrel);
	SetPhysics(PHYS_None);
	ClientKDriverEnter(pc,p);
	pc.TempTurretRotation=TurretRotation;

	for( Inv = P.Inventory; Inv != None; Inv = Inv.Inventory )
	{
		if (Inv.IsA('Artifact'))
		{
			Inv.DetachFromPawn(p);
			Inv.AttachToPawn(self);
		}
	}	

	if (Barrel != None && !bProxyControlled)
		Barrel.bUsable=false;
	if (Barrel?)
		pc.setbase(barrel);
	if (Barrel? && pc.PlayerReplicationInfo != None )
	{
		Barrel.PlayerReplicationInfo = pc.PlayerReplicationInfo;
		Barrel.OwnerName = Barrel.PlayerReplicationInfo.PlayerName;
		Barrel.Controller=pc;
		Barrel.SetOwner(pc);
	}
	if (Barrel? && Barrel.Weapon?)
	{
		Barrel.Weapon.SetNetDirty(true);
	}
    pc.GotoState('PlayerTurreting');
	pc.ClientGotoState('PlayerTurreting', 'Begin');
}

simulated function ClientKDriverLeave(PlayerController pc, Pawn P)
{
	pc.bBehindView = false;
	pc.bFixedCamera = true;
	pc.bFreeCamera = false;
	pc.setbase(none);
	pc.SetRotation(P.Rotation);
	Barrel.Controller=None;
	Barrel.SetOwner(self);
	P.GotoState('');
}

// Called from the PlayerController when player wants to get out.
function bool KDriverLeave(bool bForceLeave)
{
	local PlayerController pc;
	local Inventory Inv;

	// Do nothing if we're not being driven
	if(Driver == None)
		return false;

	Driver.bCollideWorld = true;
	Driver.SetCollision(true, true, true);
	pc = PlayerController(Controller);

	ClientKDriverLeave(pc, Driver);

	// Reconnect PlayerController to Driver.
	pc.Unpossess();
	if (Barrel.PlayerReplicationInfo != None)
	{
		Barrel.PlayerReplicationInfo=None;
		if (self.PlayerReplicationInfo != None)
			self.PlayerReplicationInfo = None;
	}
	pc.Possess(Driver);

	for( Inv = Driver.Inventory; Inv != None; Inv = Inv.Inventory )
	{
		if (Inv.IsA('Artifact'))
		{
			Inv.DetachFromPawn(self);
			Inv.AttachToPawn(Driver);
		}
	}


	Driver.GotoState('');
	//Driver.weapon.bringup();
	pc.ClientSetViewTarget(Driver); // Set playercontroller to view the person that got out

	Controller = None;

	Driver.PlayWaiting();
	Driver.bPhysicsAnimUpdate = Driver.Default.bPhysicsAnimUpdate;

    Driver.Acceleration = vect(0, 0, 24000);
	Driver.SetPhysics(PHYS_Falling);
	Driver.SetBase(None);
	Driver.bHidden = False;

	// Car now has no driver
	Driver = None;

	// Put brakes on before you get out :)
    Throttle=0;
    Steering=0;
    
	if (Barrel != None && !bProxyControlled)
		Barrel.bUsable=true;
	Barrel.Controller=None;
	Barrel.SetOwner(self);
	Self.SetTeam(255);
	Barrel.SetTeam(255);
	// Stop firing when you get out!
	bTurretIsFiring = false;
	bTurretIsAltFiring = false;

    return true;
}

// Special calc-view for turrets
simulated function bool SpecialCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local PlayerController pc;

	if (Level.NetMode != NM_DedicatedServer && PlayerController(Controller)?)
	{
		pc = PlayerController  (Controller);
		if(pc == None )
			return false;
		CameraRotation = Barrel.Rotation;
		CameraRotation.Yaw+=32767;
		CameraRotation.Pitch= -(Barrel.Rotation.Pitch);
		CameraRotation.Roll= -(Barrel.Rotation.Roll);
		CameraLocation = Barrel.Location + ((CameraOffset - Barrel.BarrelOffset) >> Barrel.Rotation);
		CameraLocation = CameraLocation + pc.ShakeOffset;
		return true;
	} 
}

simulated function Destroyed()
{
	local PlayerController pc;
	local Pawn PCPawn;
	// If there was a driver in the turret, destroy him too
	if ( Controller != None )
	{
		pc = PlayerController(Controller);

		if( Controller.bIsPlayer && pc != None )
		{
			if (Driver != None) 
				PCPawn=Driver;
			KDriverLeave(true); // Just to reset HUD etc.
			PCPawn.TakeDamage(3000,None ,PCPawn.Location , (Self.Location - PCPawn.Location) * 2, class'Crushed' );
		}
		else
			Controller.Destroy();
	}

	// Trigger any effects for destruction
	if(DestroyEffectClass != None)
		spawn(DestroyEffectClass, , , Location, Rotation);

	if(Level.NetMode != NM_Client)
			Barrel.Destroy();

	Super.Destroyed();
}

// Just to intercept 'getting out' request.
simulated event Tick(float deltaSeconds)
{
	local bool gotOut;

	if(bGetOut && ROLE==Role_Authority)
	{
		gotOut = KDriverLeave(false);
		if(!gotOut )
		{
			Log("Couldn't Leave - staying in!");
		}
	}
	bGetOut = false;

	if (Level.NetMode == NM_Client || Driver != None)
	{	
		if (Level.NetMode == NM_Client)
		{
			if (MyTurretRotation != OldRotation)
			{
				RotateTurret(MyTurretRotation);
				OldRotation = MyTurretRotation;
			}
		}
		else if (TurretRotation != OldRotation)
		{
			RotateTurret(TurretRotation);
			OldRotation = TurretRotation;
		}	
	}

	if (Role==ROLE_Authority && bTurretIsFiring)
	{
		Barrel.Myweapon.fire();
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
			Barrel.RemoteRole = ROLE_AutonomousProxy;
			Barrel.SetOwner(C);
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
	Barrel.SetOwner(Self);
	Controller = None;
}

simulated event PostNetBeginPlay()
{
	super.PostNetBeginPlay();
	if (Barrel?)
	{
		if (Barrel.ConglomerateID != Self)
			Barrel.ConglomerateID = Self;
		if (Barrel.Base != Self)
			Barrel.SetBase(Self);
		if (Barrel.RelativeLocation != Barrel.BarrelOffset)
			Barrel.SetRelativeLocation(Barrel.BarrelOffset);
		if (Barrel?)
		RotateTurret(TurretRotation);
	}
}

simulated event postnetreceive()
{
	super.postnetreceive();
	if (bNetInitial)
		RotateTurret(TurretRotation);
}

defaultproperties
{
	bCanPickupInventory=false
	bUseEnabled=false
	bUsable=true 
	Steering=0
    Throttle=0
	bExternalView=false;
	ExitPositions(0)=(X=0,Y=0,Z=0)
	bProxyControlled=false
    Physics=PHYS_None
	bEdShouldSnap=True
	bStatic=False
	bShadowCast=False
	bCollideActors=True
	bCollideWorld=True
    bProjTarget=True
	bBlockActors=True
	bBlockNonZeroExtentTraces=True
	bBlockZeroExtentTraces=True
	bBlockPlayers=True
	bWorldGeometry=False
	bBlockKarma=True
    CollisionHeight=+000001.000000
	CollisionRadius=+000001.000000
	bAcceptsProjectors=True
	bCanBeBaseForPawns=True
	bAlwaysRelevant=True
	RemoteRole=ROLE_SimulatedProxy
	bNetInitialRotation=True
	bSpecialCalcView=True
	//bSpecialHUD=true
	MaxRotation=12500
	ControllerClass=None
	bNetNotify=true
	bIgnoreEncroachers=true
	bUnlit=true
	bOwnerNoSee=false
	SoundRadius=128
	SoundVolume=192
}