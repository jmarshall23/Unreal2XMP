// Generic 'Karma Vehicle' base class that can be controlled by a Pawn.
#exec OBJ LOAD File=..\Particles\Vehicle_Explosion_FX.u
class KVehicle extends Pawn
    native
    abstract;

cpptext
{
#ifdef WITH_KARMA
	virtual void PostNetReceive();
    virtual void PostEditChange();
	virtual void setPhysics(BYTE NewPhysics, AActor *NewFloor, FVector NewFloorV);
	virtual void TickSimulated( FLOAT DeltaSeconds );
	virtual void TickAuthoritative( FLOAT DeltaSeconds );
#endif

}

//moved wheels up into KVehicle so that I can use their information while doing karma collision
var KTire frontLeft, frontRight, rearLeft, rearRight;

// generic controls (set by controller, used by concrete derived classes)
var (KVehicle)	float						Steering; // between -1 and 1
var (KVehicle)	float						Throttle; // between -1 and 1
var				bool						bThrottling;
var				bool						bOldThrottling;
var				StaticMesh					BaseMesh;
var				StaticMesh					DriverViewMesh;
const										HardMaxPassengers=10;
var				int							MaxPassengers;
var				Pawn						PassengerPawns[HardMaxPassengers];
var				Controller					PassengerControllers[HardMaxPassengers];
var				PlayerReplicationInfo		PassengerPRIs[HardMaxPassengers];
var				XMPVehicleTurret			Turrets[HardMaxPassengers];
var				int							NumPassengers;
var				Rotator						TurretRotation[HardMaxPassengers];
var				Rotator						InitialTurretRotation[HardMaxPassengers];
var				Rotator						MyTurretRotation;
var				class<XMPVehicleTurret>		TurretClass[HardMaxPassengers];
var				float						CollisionDamageMultiplier;
var				float						SameTeamCollisionDamageMultiplier;
var (KVehicle)	array<vector>				ExitPositions;		// Positions (rel to vehicle) to try putting the player when exiting.
var				int							MyPassengerNumber;

// Simple 'driving-in-rings' logic.
var (KVehicle)	bool						bAutoDrive;
var (KVehicle)	bool						bExternalView;

var				byte						bGetOut[HardMaxPassengers];

// The factory that created this vehicle.
var				KVehicleFactory				ParentFactory;

// Weapon system
var				byte						bVehicleIsFiring[HardMaxPassengers], bVehicleIsAltFiring[HardMaxPassengers];
var				class<DamageType>			MyDamageType;
const										FilterFrames = 5;
var				vector						CameraHistory[FilterFrames];
var				vector						SprintImpulseOffset;
var				float						Stamina;
var				float						StaminaMax;
var				float						StaminaRechargeRate;
var				int							NextHistorySlot;
var				bool						bHistoryWarmup;

var				bool						OnLine;

var				float						LastOccupiedTime;
var				int							LastOccupiedTeam;

// Useful function for plotting data to real-time graph on screen.
native final function GraphData(string DataName, float DataValue);

replication
{
	reliable if(Role==ROLE_Authority)
		ClientKDriverEnter, ClientKDriverLeave;
	reliable if(Role==ROLE_Authority)
		//TurretRotation, Turrets, PassengerPawns, PassengerControllers, touchme;
		InitialTurretRotation, TurretRotation, Turrets, PassengerPRIs, Online;
	reliable if(Role < ROLE_Authority)
		VehicleFire, VehicleCeaseFire;
}

function name GetWeaponBoneFor(Inventory I)
{
	return '';
}

function DoSprint()
{
//STUB
}

simulated function FaceRotation( rotator NewRotation, float DeltaTime )
{
	// Vehicles ignore 'face rotation'.
}

function SetTeam( int NewTeam )
{
	local int i;

	// cache the last occupied time / team
	if( GetTeam() != 255 )
	{
		LastOccupiedTeam = GetTeam();
		LastOccupiedTime = Level.TimeSeconds;
	}

	Super.SetTeam( NewTeam );
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
	for (i=0; i< MaxPassengers; i++)
	{
		if (Turrets[i] != None)
		{
			if (Turrets[i].DrawType != DT_None)
				class'UtilGame'.static.SetTeamSkin( Turrets[i], NewTeam );
			if (Turrets[i].Barrel != None)
				class'UtilGame'.static.SetTeamSkin( Turrets[i].Barrel, NewTeam );
		}
	}
}

// Vehicles dont get telefragged.
event EncroachedBy( actor Other )
{
}

// This will get called if we couldn't move a pawn out of the way. 
function Smack( actor Other )
{

	local vector MomentumTransfer;
	local Pawn Instigator;
	local int ImpactSpeed;
	local int Damage;

	if ( Pawn(Other) != None )
	{
		if ( Pawn(Other).Health > 0 && Role == ROLE_Authority)
		{
			Instigator = Self;
			//if (PassengerPawns[0] != None)
			//	Instigator = PassengerPawns[0];
			ImpactSpeed = clamp(VSize(velocity),400,1200);
			MomentumTransfer = normal(Other.location - location);
			MomentumTransfer *= (ImpactSpeed * 80);
			MomentumTransfer.Z+=32000;
			Damage = ((VSize(velocity) / 20 )* CollisionDamageMultiplier);
/*			Damage = Level.Game.ReduceDamage(
				((VSize(velocity) / 10 )* CollisionDamageMultiplier)
				, Self
				, Instigator
				, Location
				, MomentumTransfer
				, class'Crushed');*/

			Other.TakeDamage(
				  Damage
				, Instigator
				, Other.Location
				, MomentumTransfer
				, class'Crushed');

		}
	}
}

// You got some new info from the server (ie. VehicleState has some new info).
event VehicleStateReceived();

// Called when a parameter of the overall articulated actor has changed (like PostEditChange)
// The script must then call KUpdateConstraintParams or Actor Karma mutators as appropriate.
simulated event KVehicleUpdateParams();

// Do some server-side vehicle firing stuff
function VehicleFire(int PassengerNumber, bool bWasAltFire)
{
	if(bWasAltFire)
		bVehicleIsAltFiring[PassengerNumber] = 1;
	else
		bVehicleIsFiring[PassengerNumber] = 1;
}

function VehicleCeaseFire(int PassengerNumber, bool bWasAltFire)
{
	if(bWasAltFire)
		bVehicleIsAltFiring[PassengerNumber] = 0;
	else
		bVehicleIsFiring[PassengerNumber] = 0;
}

// The pawn Driver has tried to take control of this vehicle
function TryToDrive(Pawn p, int PassengerLocation)
{
	local Controller C;
	local bool SameTeam;
	C = p.Controller;
	
	//iif someone is already occupying the chosen location, don't allow to drive (this should never happen, but just in case)
	if (PassengerControllers[PassengerLocation] != None) return;
	
	//only allow to drive if vehicle's team is same as player's or if vehicle is neutral
	if ((Self.GetTeam() == C.GetTeam()) || Self.GetTeam() == 255)
		SameTeam = true;
	else
		SameTeam = false;

    if (C.bIsPlayer && p.IsHumanControlled() && SameTeam)
	{        
		KDriverEnter(p, PassengerLocation);
    }
}

function SetUsability (int PassengerNum)
{
//STUB!!!
}

function UnsetUsability (int PassengerNum)
{
//More STUB!!!
}


// Events called on driver entering/leaving vehicle

simulated function ClientKDriverEnter(PlayerController pc, int PassengerNum, Pawn p)
{
	local rotator StartingRotation;
	bThrottling = false;
	bOldThrottling = false;
	pc.DrivenVehicle=Self;
	pc.bBehindView = true;
	pc.bFixedCamera = false;
	pc.bFreeCamera = true;
	pc.PassengerNumber=PassengerNum;
    pc.TempTurretRotation=TurretRotation[PassengerNum];
	P.weapon.putdown();
	if (!PlayerController(Controller)? && PassengerNum==0)
	{
		Controller=pc;
	}
	MyTurretRotation=TurretRotation[PassengerNum];
	if (PassengerNum==0)
	{
		if (Turrets[PassengerNum]? && Turrets[PassengerNum].Barrel?)
		{
			Turrets[PassengerNum].Barrel.Controller=pc;
			Turrets[PassengerNum].Barrel.SetOwner(pc);
			if (!Turrets[PassengerNum].Barrel.Weapon? && Turrets[PassengerNum].Barrel.Myweapon?)
				Turrets[PassengerNum].Barrel.Weapon = Turrets[PassengerNum].Barrel.Myweapon;
			if (Turrets[PassengerNum].Barrel.Weapon?)
			{	
				if( Turrets[PassengerNum].Barrel.Weapon.CrossHair? )
					pc.SendEvent(Turrets[PassengerNum].Barrel.Weapon.CrossHair);
				Turrets[PassengerNum].Barrel.Weapon.Instigator = Turrets[PassengerNum].Barrel;
				if (Turrets[PassengerNum].Barrel.Weapon.ThirdPersonActor?)
					Turrets[PassengerNum].Barrel.Weapon.ThirdPersonActor.Instigator = Turrets[PassengerNum].Barrel;
			}

			if (Role<ROLE_Authority && Turrets[PassengerNum].Barrel.MyWeapon?)
			{
				Turrets[PassengerNum].Barrel.Weapon=Turrets[PassengerNum].Barrel.MyWeapon;
				Turrets[PassengerNum].Barrel.Weapon.AmmoType=Turrets[PassengerNum].Barrel.MyAmmoType;
				Turrets[PassengerNum].Barrel.Weapon.AmmoTypes[0]=Turrets[PassengerNum].Barrel.MyAmmoType;
			}
			Turrets[PassengerNum].RotateTurret(TurretRotation[PassengerNum]);
			StartingRotation = Turrets[PassengerNum].Barrel.Rotation;
		} else
		{
			Turrets[PassengerNum].RotateTurret(TurretRotation[PassengerNum]);
			StartingRotation = Turrets[PassengerNum].Rotation;
		}
		StartingRotation.Yaw += 32768;
		StartingRotation.Pitch *=-1;
		StartingRotation.Roll *= -1;
		pc.SetRotation(StartingRotation);
	}
	//TurretRotation[PassengerNum]=InitialTurretRotation[PassengerNum];
	MyPassengerNumber = PassengerNum;
	if (PassengerNum == 0)
	{
		Stamina=100;
	}
	if (pc.bSprint?)
		pc.bSprint = 0;
}

function KDriverEnter(Pawn p, int PassengerNum)
{
	local PlayerController pc;
	local Inventory Inv;
	local rotator StartingRotation;

	PassengerPawns[PassengerNum] = p;
	// Make player's pawn invisible and kill its collision, put down its weapon
	PassengerPawns[PassengerNum].SetCollision(false, false, false);
	//PassengerPawns[PassengerNum].bCollideWorld = false;
	PassengerPawns[PassengerNum].bPhysicsAnimUpdate = false;
	PassengerPawns[PassengerNum].Velocity = vect(0,0,0);
	PassengerPawns[PassengerNum].SetPhysics(PHYS_None);
	PassengerPawns[PassengerNum].bHidden= True;
	PassengerPawns[PassengerNum].weapon.putdown();
	PassengerPawns[PassengerNum].bCanBeDamaged = false;
	PassengerPawns[PassengerNum].GotoState('Passenger');
	
	pc = PlayerController(p.Controller);
	pc.DrivenVehicle=Self;
	pc.PassengerNumber=PassengerNum;
	PassengerControllers[PassengerNum]=pc;
	PassengerPRIs[PassengerNum]=pc.PlayerReplicationInfo;
	if (NumPassengers == 0)
		Self.SetTeam(pc.GetTeam());
	NumPassengers++;

	pc.EntryTime=Level.TimeSeconds;
	// Disconnect PlayerController from PlayerPawn and connect to KVehicle or turret barrel (pawn)
	pc.Unpossess();
	PassengerPawns[PassengerNum].SetOwner(pc); // This keeps the driver relevant.
	if (Turrets[pc.PassengerNumber].bArticulated)
	{
		if (Turrets[pc.PassengerNumber].bRelativeRotation)
		{
			pc.setLocation(Turrets[pc.PassengerNumber].barrel.Location);
			pc.setbase(Turrets[pc.PassengerNumber].barrel);
			//pc.SetRotation(Turrets[pc.PassengerNumber].barrel.Rotation);
			pc.ClientSetViewTarget(Turrets[pc.PassengerNumber].barrel);
		}
		else
		{
			pc.setLocation(Turrets[pc.PassengerNumber].barrel.Location);
			pc.setbase(Turrets[pc.PassengerNumber]);
			//pc.SetRotation(Turrets[pc.PassengerNumber].barrel.Rotation);
			pc.ClientSetViewTarget(Turrets[pc.PassengerNumber].barrel);
		}
	} else
	{
		if (Turrets[pc.PassengerNumber].bRelativeRotation)
		{
			pc.setbase(self);
			//pc.SetRotation(Turrets[pc.PassengerNumber].Rotation);
			pc.ClientSetViewTarget(Turrets[pc.PassengerNumber]);
		}
		else
		{
			pc.setbase(self);
			//pc.SetRotation(Turrets[pc.PassengerNumber].Rotation);
			pc.ClientSetViewTarget(self);
		}
	}
	//pc.TempTurretRotation=InitialTurretRotation[PassengerNum];
	if (PassengerNum == 0)
	{
		pc.Possess(self);
		if (Turrets[0].Barrel?)
			Turrets[0].Barrel.Controller=pc;
	}
	else
	{
		pc.Possess(Turrets[PassengerNum].barrel);
	}

	for( Inv = P.Inventory; Inv != None; Inv = Inv.Inventory )
	{
		if (Inv.IsA('Artifact'))
		{
			if (PassengerNum == 0)
			{
				Inv.DetachFromPawn(p);
				Inv.AttachToPawn(self);
			} else
			{
				Inv.DetachFromPawn(p);
				Inv.AttachToPawn(Turrets[PassengerNum].Barrel);
			}
			Inv.SetRelativeLocation(vect(0,0,50));
		}
	}	

	Turrets[PassengerNum].SetCollision(true,false,false);
	if (Turrets[PassengerNum].Barrel?)
		Turrets[PassengerNum].Barrel.SetCollision(true,false,false);

	if (PassengerNum == 0 && Turrets[PassengerNum].Barrel? && pc.PlayerReplicationInfo != None )
	{
		Turrets[PassengerNum].Barrel.PlayerReplicationInfo = pc.PlayerReplicationInfo;
		Turrets[PassengerNum].Barrel.OwnerName = Turrets[PassengerNum].Barrel.PlayerReplicationInfo.PlayerName;
		Turrets[PassengerNum].Barrel.Controller=pc;
		Turrets[PassengerNum].Barrel.SetOwner(pc);
	}

	if (Turrets[PassengerNum].Barrel? && Turrets[PassengerNum].Barrel.Weapon?)
		Turrets[PassengerNum].Barrel.Weapon.SetNetDirty(true);
	if (Turrets[PassengerNum].Barrel?)
	{
		StartingRotation = Turrets[PassengerNum].Barrel.Rotation;
	} else
	{
		StartingRotation = Turrets[PassengerNum].Rotation;
	}
	StartingRotation.Yaw += 32768;
	StartingRotation.Pitch *=-1;
	StartingRotation.Roll *= -1;
	pc.SetRotation(StartingRotation);
	// Change controller state to driver
    pc.GotoState('PlayerDriving');
	pc.ClientGotoState('PlayerDriving', 'Begin');
		
	SetUsability(PassengerNum);
    if (PassengerNum == 0)
	{
		Stamina=100;
		if (pc.bSprint?)
			pc.bSprint = 0;
		ClientKDriverEnter(pc, PassengerNum, p);
	}
	else
		Turrets[PassengerNum].Barrel.ClientKDriverEnter(pc, PassengerNum, p);

	pc.ClientSendEvent(12);	//ShowPassengers
}

simulated function ClientKDriverLeave(PlayerController pc, Rotator exitLookDir)
{
	pc.bBehindView = false;
	pc.bFixedCamera = true;
	pc.bFreeCamera = false;
	pc.DrivenVehicle=None;
	pc.SetRotation(exitLookDir);
	pc.setbase(None);
	if (pc.PassengerNumber == 0)
		Controller = None;
	if (pc.PassengerNumber == 0)
	{
		Controller=None;
		if (Turrets[pc.PassengerNumber]? && Turrets[pc.PassengerNumber].Barrel?)
		{
			Turrets[pc.PassengerNumber].Barrel.Controller=None;
			Turrets[pc.PassengerNumber].Barrel.SetOwner(Turrets[pc.PassengerNumber]);
		}
	}
	pc.PassengerNumber=-1;
	MyPassengerNumber = -1;

}

// Called from the PlayerController when player wants to get out.
function bool KDriverLeave(int PassengerNumber, bool bForceLeave)
{
	local Inventory Inv;
	local PlayerController pc;
	local int i;
	local bool havePlaced,bIsInverted;
	local vector HitLocation, HitNormal, tryPlace, highLocation, worldup;
	local Rotator exitLookDir;

    //log("KVehicle KDriverLeave");

	// Do nothing if we're not being driven
	if(PassengerPawns[PassengerNumber] == None)
		return false;

	// Before we can exit, we need to find a place to put the driver.
	// Iterate over array of possible exit locations.
	
	PassengerPawns[PassengerNumber].bCollideWorld = true;
	PassengerPawns[PassengerNumber].SetCollision(true, true, true);
	
	havePlaced = false;
	worldUp = vect(0, 0, 1) >> Rotation;
	bIsInverted = worldUp.Z < 0.2;
	for(i=0; i < ExitPositions.Length && havePlaced == false; i++)
	{
		//Log("Trying Exit:"$i);
		if (bIsInverted)
		{
			highLocation = ExitPositions[i];
			highLocation.Z *= -1;
			tryPlace = Location + (highLocation >> Rotation);
		} else
			tryPlace = Location + (ExitPositions[i] >> Rotation);

		// First, do a line check (stops us passing through things on exit).
		if( Trace(HitLocation, HitNormal, tryPlace, Location, false) != None )
			continue;
			
		// Then see if we can place the player there.
//		if (!level.FarMoveActor(self, tryPlace, 1))
		if( !PassengerPawns[PassengerNumber].TestLocation(tryPlace) )
			continue;
		
		havePlaced = true;
		
		//Log("SUCCESS!");		
	}

	// If we could not find a place to put the driver, leave driver inside as before.
	if(!havePlaced && !bForceLeave)
	{
		Log("Could not place driver.");
	
		PassengerPawns[PassengerNumber].bCollideWorld = false;
		PassengerPawns[PassengerNumber].SetCollision(false, false, false);
		PassengerPawns[PassengerNumber].bHidden= True;
		return false;
	}

	InitialTurretRotation[PassengerNumber]=TurretRotation[PassengerNumber];
	pc = PlayerController(PassengerControllers[PassengerNumber]);
	// Reconnect PlayerController to Driver.
	PassengerControllers[PassengerNumber] =  None;
	PassengerPRIs[PassengerNumber] = None;
	if (Turrets[PassengerNumber].Barrel?)
		exitLookDir=Turrets[PassengerNumber].Barrel.Rotation;
	else
		exitLookDir=Turrets[PassengerNumber].Rotation;
	exitLookDir = Rotator(-Vector(exitLookDir));
	exitLookDir.Roll=0;
	
	if (PassengerNumber == 0)
		ClientKDriverLeave(pc, exitLookDir);	
	else
		Turrets[PassengerNumber].Barrel.ClientKDriverLeave(pc, exitLookDir);
	
	pc.ClientSendEvent(13);	//HidePassengers
	pc.Unpossess();
	if (PassengerNumber == 0 && Turrets[PassengerNumber].Barrel? && Turrets[PassengerNumber].Barrel.PlayerReplicationInfo != None)
	{
		Turrets[PassengerNumber].Barrel.PlayerReplicationInfo=None;
		if (self.PlayerReplicationInfo != None)
			self.PlayerReplicationInfo = None;
	}
	pc.Possess(PassengerPawns[PassengerNumber]);	
	PassengerPawns[PassengerNumber].SetLocation(tryPlace);
	PassengerPawns[PassengerNumber].SetRotation(exitLookDir);
	PassengerPawns[PassengerNumber].bPhysicsAnimUpdate = PassengerPawns[PassengerNumber].Default.bPhysicsAnimUpdate;
	PassengerPawns[PassengerNumber].bHidden= False;
    PassengerPawns[PassengerNumber].Acceleration = vect(0, 0, 24000);
	PassengerPawns[PassengerNumber].SetPhysics(PHYS_Falling);
//	PassengerPawns[PassengerNumber].SetBase(None);
	PassengerPawns[PassengerNumber].bCanBeDamaged = true;
	PassengerPawns[PassengerNumber].GotoState('');
	pc.SetBase(None);
	pc.DrivenVehicle=None;	
	pc.PassengerNumber=-1;
	pc.SetRotation(exitLookDir);
	pc.ClientSetViewTarget(PassengerPawns[PassengerNumber]); // Set playercontroller to view the person that got out
	
	if (PassengerNumber == 0)
	{
		if (Turrets[0].Barrel?)
			Turrets[0].Barrel.Controller=None;
		Controller = None;	
	}
	else
		Turrets[PassengerNumber].Barrel.Controller = None;
	
	
	UnsetUsability(PassengerNumber);


	// Put brakes on before you get out :)
	if (PassengerNumber == 0)
	{
		Throttle=0;
		Steering=0;
	}
	for( Inv = PassengerPawns[PassengerNumber].Inventory; Inv != None; Inv = Inv.Inventory )
	{
		if (Inv.IsA('Artifact'))
		{
			if (PassengerNumber == 0)
			{
				Inv.DetachFromPawn(self);
				Inv.AttachToPawn(PassengerPawns[PassengerNumber]);
			} else
			{
				Inv.DetachFromPawn(Turrets[PassengerNumber].Barrel);
				Inv.AttachToPawn(PassengerPawns[PassengerNumber]);
			}
			Inv.SetRelativeLocation(vect(0,0,0));
		}
	}
	PassengerPawns[PassengerNumber] = None;

	if (PassengerNumber ==0 && Turrets[0]?)
	{
		//if driver position, reset driver cam to center
		TurretRotation[0] = Turrets[0].RotationCenter;
		Turrets[0].RotateTurret(TurretRotation[0]);
	}
	// Stop firing when you get out!
	bVehicleIsFiring[PassengerNumber] = 0;
	bVehicleIsAltFiring[PassengerNumber] = 0;
	NumPassengers--;
	if (NumPassengers == 0)
		Self.SetTeam(255);
    return true;
}




// Special calc-view for vehicles
simulated function bool SpecialCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local vector CamLookAt, HitLocation, HitNormal;
	local PlayerController pc;
	local int i, averageOver;

	pc = PlayerController(Controller);

	// Only do this mode we have a playercontroller viewing this vehicle
	if(pc == None)
		return false;

	ViewActor = self;
	CamLookAt = Location + (vect(-100, 0, 100) >> Rotation); 

	//////////////////////////////////////////////////////
	// Smooth lookat position over a few frames.
	CameraHistory[NextHistorySlot] = CamLookAt;
	NextHistorySlot++;

	if(bHistoryWarmup)
		averageOver = NextHistorySlot;
	else
		averageOver = FilterFrames;

	CamLookAt = vect(0, 0, 0);
	for(i=0; i<averageOver; i++)
		CamLookAt += CameraHistory[i];

	CamLookAt /= float(averageOver);

	if(NextHistorySlot == FilterFrames)
	{
		NextHistorySlot = 0;
		bHistoryWarmup=false;
	}
	//////////////////////////////////////////////////////

	CameraLocation = CamLookAt + (vect(-600, 0, 0) >> CameraRotation);

	if( Trace( HitLocation, HitNormal, CameraLocation, CamLookAt, false, vect(10, 10, 10) ) != None )
	{
		CameraLocation = HitLocation;
	}

	return true;

}


simulated function Destroyed()
{
    local int i;
	local PlayerController pc;
	local Pawn PCPawn;
	if (Level.NetMode != NM_Client)
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
					PCPawn.TakeDamage(3000,None ,PCPawn.Location , (Self.Location - PCPawn.Location) * 2, class'Crushed' );
			}
		}

		// Decrease number of cars active out of parent factory
		if(ParentFactory != None)
		{
			ParentFactory.VehicleCount--;
			ParentFactory.Enable( 'Tick' );
		}

	}
	Super.Destroyed();
}

// Just to intercept 'getting out' request.
simulated event Tick(float deltaSeconds)
{
	local bool gotOut;
	local int PassengerNumber;

	super.tick(DeltaSeconds);

	if( Stamina < StaminaMax )
		{
			Stamina += StaminaRechargeRate * DeltaSeconds;
			if( Stamina > StaminaMax ) Stamina = StaminaMax;
		}
	for (PassengerNumber=0; PassengerNumber<MaxPassengers; PassengerNumber++)
	{
		if(bGetOut[PassengerNumber]==1)
		{
			gotOut = KDriverLeave(PassengerNumber, false);
			if(!gotOut )
			{
				Log("Couldn't Leave - staying in!");
			}
		}
		bGetOut[PassengerNumber] = 0;
	}

}

function fire (optional float F)
{
//STUB
}

function interface bool GetPassengerEnabled0()		{ return PassengerPRIs[0]?; }
function interface bool GetPassengerEnabled1()		{ return PassengerPRIs[1]?; }
function interface bool GetPassengerEnabled2()		{ return PassengerPRIs[2]?; }

function interface string GetPassengerName0()		{ return PassengerPRIs[0].PlayerName; }
function interface string GetPassengerName1()		{ return PassengerPRIs[1].PlayerName; }
function interface string GetPassengerName2()		{ return PassengerPRIs[2].PlayerName; }

function interface int GetPassengerClassIndex0()	{ return PassengerPRIs[0].PawnClass.default.IconIndex; }
function interface int GetPassengerClassIndex1()	{ return PassengerPRIs[1].PawnClass.default.IconIndex; }
function interface int GetPassengerClassIndex2()	{ return PassengerPRIs[2].PawnClass.default.IconIndex; }

function interface int GetPassengerIconIndex0()		{ return 0; }
function interface int GetPassengerIconIndex1()		{ return 1; }
function interface int GetPassengerIconIndex2()		{ return 2; }

// Includes properties from KActor
defaultproperties
{
	bCanPickupInventory=false
	bUseEnabled=false
	MaxPassengers=10
    Steering=0
    Throttle=0
	bExternalView=false;
	ExitPositions(0)=(X=0,Y=0,Z=0)

	NumPassengers=0

	bHistoryWarmup = true;

	CollisionDamageMultiplier = 1
	SameTeamCollisionDamageMultiplier = 0.5

    Physics=PHYS_Karma
	bEdShouldSnap=True
	bStatic=False
	bShadowCast=False
	bCollideActors=True
	bCollideWorld=False
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
	//bAlwaysRelevant=True
	RemoteRole=ROLE_SimulatedProxy
	bNetInitialRotation=True
	bSpecialCalcView=True
	//bSpecialHUD=true
	ControllerClass=None
	MyDamageType=class'DamageType'
	SprintImpulseOffset=(X=0,Y=0,Z=0)
	StaminaMax=100
	StaminaRechargeRate=7.0
	MyPassengerNumber=-1
	bOwnerNoSee=false
	//bDestroyInPainVolume=true
}