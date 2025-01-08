class SVehicle extends Pawn
	native
	abstract;

cpptext
{
#ifdef WITH_KARMA
	// Actor interface.
	virtual void PostBeginPlay();
	virtual void Destroy();
	virtual void PostNetReceive();
    virtual void PostEditChange();
	virtual void setPhysics(BYTE NewPhysics, AActor *NewFloor, FVector NewFloorV);
	virtual void TickSimulated( FLOAT DeltaSeconds );
	virtual void TickAuthoritative( FLOAT DeltaSeconds );
	virtual void physKarma(FLOAT DeltaTime);
	virtual void preContactUpdate();
	virtual void preKarmaStep(FLOAT DeltaTime);

	// SVehicle interface.
	virtual void UpdateVehicle(FLOAT DeltaTime);
#endif

}

var (SVehicle) editinline export	array<SVehicleWheel>		Wheels; // Wheel data

// generic controls (set by controller, used by concrete derived classes)
var (SVehicle) float    Steering; // between -1 and 1
var (SVehicle) float    Throttle; // between -1 and 1
var (SVehicle) float	Rise;	  // between -1 and 1

var			   Pawn				Driver;

var (SVehicle) array<vector>	ExitPositions;		// Positions (rel to vehicle) to try putting the player when exiting.
var (SVehicle) array<vector>	EntryPositions;		// Positions (rel to vehicle) to create triggers for entry.

var (SVehicle) vector	DrivePos;		// Position (rel to vehicle) to put player while driving.
var (SVehicle) rotator	DriveRot;		// Rotation (rel to vehicle) to put driver while driving.
var (SVehicle) name		DriveAnim;		// Animation to play while driving.

//// CAMERAS ////
var (SVehicle) bool		bDrawMeshInFP;		// Whether to draw the vehicle mesh when in 1st person mode.
var	(SVehicle) bool		bDrawDriverInTP;	// Whether to draw the driver when in 3rd person mode.
var (SVehicle) bool		bZeroPCRotOnEntry;	// If true, set camera rotation to zero on entering vehicle. If false, set it to the vehicle rotation.

var (SVehicle) vector   FPCamPos;	// Position of camera when driving first person.

var (SVehicle) vector   TPCamLookat;
var (SVehicle) float    TPCamDistance;

var (SVehicle) int		MaxViewYaw; // Maximum amount you can look left and right
var (SVehicle) int		MaxViewPitch; // Maximum amount you can look up and down 

//// PHYSICS ////
var (SVehicle) float	VehicleMass;

//// EFFECTS ////

// Effect spawned when vehicle is destroyed
var (SVehicle) class<Actor>	DestroyEffectClass;

// Created in PostNetBeginPlay and destroyed when vehicle is.
var			   array<SVehicleTrigger>	EntryTriggers;

var			   bool     bGetOut;

// The factory that created this vehicle.
var			   SVehicleFactory	ParentFactory;

// Shadow projector
var			   ShadowProjector	VehicleShadow;

// Useful function for plotting data to real-time graph on screen.
native final function GraphData(string DataName, float DataValue);

replication
{
	reliable if(Role==ROLE_Authority)
		ClientKDriverEnter, ClientKDriverLeave;
}

// Really simple at the moment!
function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	local PlayerController pc;

	Health -= Damage;

	//Log("Ouch! "$Health);

	// The vehicle is dead!
	if(Health <= 0)
	{
		if ( Controller != None )
		{
			pc = PlayerController(Controller);

			if( Controller.bIsPlayer && pc != None )
			{
				ClientKDriverLeave(pc); // Just to reset HUD etc.

				// THIS WAS HOW IT WAS IN UT2003, DONT KNOW ABOUT WARFARE...
				// pc.PawnDied(self); // This should unpossess the controller and let the player respawn
			}
			else
				Controller.Destroy();
		}

		Destroy(); // Destroy the vehicle itself (see Destroyed below)
	}

    //KAddImpulse(momentum, hitlocation);
}

// Vehicles dont get telefragged.
event EncroachedBy( actor Other )
{
	Log("SVehicle("$self$") Encroached By: "$Other$".");
}

simulated function FaceRotation( rotator NewRotation, float DeltaTime )
{
	// Vehicles ignore 'face rotation'.
}

// You got some new info from the server (ie. VehicleState has some new info).
event VehicleStateReceived();

// Do script car model stuff here - but DONT create/destroy anything.
native event UpdateVehicle( float DeltaTime );

// Do any general vehicle set-up when it gets spawned.
simulated function PostNetBeginPlay()
{
	local vector RotX, RotY, RotZ;
	local int i;
	local SVehicleTrigger NewTrigger;

    Super.PostNetBeginPlay();

	if(Level.NetMode != NM_Client)
	{
	    GetAxes(Rotation,RotX,RotY,RotZ);

		EntryTriggers.Length = EntryPositions.Length;

		for(i=0; i<EntryPositions.Length; i++)
		{
			// Create triggers for gettting into the hoverbike - only on the server
			NewTrigger = spawn(class'SVehicleTrigger', self,, Location + EntryPositions[i].X * RotX + EntryPositions[i].Y * RotY + EntryPositions[i].Z * RotZ);
			NewTrigger.SetBase(self);
			NewTrigger.SetCollision(true, false, false);

			EntryTriggers[i] = NewTrigger;
		}
	}

	// Make sure params are up to date.
	SVehicleUpdateParams();
}

// Called when a parameter of the overall articulated actor has changed (like PostEditChange)
// The script must then call KUpdateConstraintParams or Actor Karma mutators as appropriate.
simulated event SVehicleUpdateParams()
{
	KSetMass(VehicleMass);
}

// The pawn Driver has tried to take control of this vehicle
function TryToDrive(Pawn p)
{
	local Controller C;
	C = p.Controller;

    if ( (Driver == None) && (C != None) && C.bIsPlayer && !C.IsInState('PlayerDriving') && p.IsHumanControlled() )
	{        
		KDriverEnter(p);
    }
}

// Events called on driver entering/leaving vehicle

simulated function ClientKDriverEnter(PlayerController pc)
{
	//log("Enter: "$pc.Pawn);

	//pc.myHUD.bCrosshairShow = false;
	//pc.myHUD.bShowWeaponInfo = false;
	//pc.myHUD.bShowPersonalInfo = false;
	//pc.myHUD.bShowPoints = false;

	pc.bBehindView = true;
	pc.bFixedCamera = false;
	pc.bFreeCamera = true;

	// Set rotation of camera when getting into vehicle based on bZeroPCRotOnEntry
	if(	bZeroPCRotOnEntry )
		pc.SetRotation( rot(0, 0, 0) );
	else
		pc.SetRotation( rotator( vect(1, 0, 0) >> Rotation ) );
}

function KDriverEnter(Pawn p)
{
	local PlayerController pc;
	local vector AttachPos;

    //log("SVehicle KDriverEnter");

	// Set pawns current controller to control the vehicle pawn instead
	Driver = p;

	// Move the driver into position, and attach to car.
	Driver.SetCollision(false, false, false);
	Driver.bCollideWorld = false;
	Driver.bPhysicsAnimUpdate = false;
	Driver.Velocity = vect(0,0,0);
//	Driver.SetPhysics(PHYS_None);

	AttachPos = Location + (DrivePos >> Rotation);
	Driver.SetLocation(AttachPos);

	Driver.SetPhysics(PHYS_None);

	Driver.bHardAttach = true;
	Driver.SetBase(None);
	Driver.SetBase(self);
	Driver.SetRelativeRotation(DriveRot);
	Driver.SetPhysics(PHYS_None);

	pc = PlayerController(p.Controller);
	//pc.ClientSetBehindView(true);
	//pc.ClientSetFixedCamera(false);

	// Disconnect PlyaerController from Driver and connect to SVehicle.
	pc.Unpossess();
	Driver.SetOwner(self); // This keeps the driver relevant.
	pc.Possess(self);

	pc.ClientSetViewTarget(self); // Set playercontroller to view the vehicle

	// Change controller state to driver
    pc.GotoState('PlayerDriving');

	ClientKDriverEnter(pc);
}

simulated function ClientKDriverLeave(PlayerController pc)
{
	//local vector exitLookDir;

	//log("Leave: "$pc.Pawn);

	//pc.bBehindView = false;
	pc.bFixedCamera = true;
	pc.bFreeCamera = false;

	// Stop messing with bOwnerNoSee
	Driver.bOwnerNoSee = Driver.default.bOwnerNoSee;

	// This removes any 'roll' from the look direction.
	//exitLookDir = Vector(pc.Rotation);
	//pc.SetRotation(Rotator(exitLookDir));

    //pc.myHUD.bCrosshairShow = pc.myHUD.default.bCrosshairShow;
	//pc.myHUD.bShowWeaponInfo = pc.myHUD.default.bShowWeaponInfo;
	//pc.myHUD.bShowPersonalInfo = pc.myHUD.default.bShowPersonalInfo;
	//pc.myHUD.bShowPoints = pc.myHUD.default.bShowPoints;
}

// Called from the PlayerController when player wants to get out.
function bool KDriverLeave(bool bForceLeave)
{
	local PlayerController pc;
	local int i;
	local bool havePlaced;
	local vector HitLocation, HitNormal, tryPlace;

    //log("SVehicle KDriverLeave");

	// Do nothing if we're not being driven
	if(Driver == None)
		return false;

	// Before we can exit, we need to find a place to put the driver.
	// Iterate over array of possible exit locations.
	
	Driver.bHardAttach = false;
	Driver.bCollideWorld = true;
	Driver.SetCollision(true, true, true);
	
	havePlaced = false;
	for(i=0; i < ExitPositions.Length && havePlaced == false; i++)
	{
		//Log("Trying Exit:"$i);
	
		tryPlace = Location + (ExitPositions[i] >> Rotation);
	
		// First, do a line check (stops us passing through things on exit).
		if( Trace(HitLocation, HitNormal, tryPlace, Location, false) != None )
			continue;
			
		// Then see if we can place the player there.
		if( !Driver.SetLocation(tryPlace) )
			continue;
		
		havePlaced = true;
		
		//Log("SUCCESS!");		
	}

	// If we could not find a place to put the driver, leave driver inside as before.
	if(!havePlaced && !bForceLeave)
	{
		Log("Could not place driver.");
	
		Driver.bHardAttach = true;
		Driver.bCollideWorld = false;
		Driver.SetCollision(false, false, false);
	
		return false;
	}

	pc = PlayerController(Controller);

	//Log("Pre ClientKDriverLeave");
	ClientKDriverLeave(pc);
	//Log("Post ClientKDriverLeave");

	// Reconnect PlayerController to Driver.
	pc.Unpossess();
	Driver.SetOwner(pc);
	pc.Possess(Driver);

	pc.ClientSetViewTarget(Driver); // Set playercontroller to view the persone that got out

	Controller = None;

	Driver.PlayWaiting();
	Driver.bPhysicsAnimUpdate = Driver.Default.bPhysicsAnimUpdate;

	// Do stuff on client
	//pc.ClientSetBehindView(false);
	//pc.ClientSetFixedCamera(true);

    Driver.Acceleration = vect(0, 0, 24000);
	Driver.SetPhysics(PHYS_Falling);
	Driver.SetBase(None);

	// Car now has no driver
	Driver = None;

	// Put brakes on before you get out :)
    Throttle=0;
    Steering=0;
	Rise=0;
    
    return true;
}

simulated function Destroyed()
{
	local int i;

	//Log("SVehicle Destroyed");

	// Destroy the triggers used for getting in.
	for(i=0; i<EntryTriggers.Length; i++)
	{
		if(EntryTriggers[i] != None)
			EntryTriggers[i].Destroy();
	}

	// If there was a driver in the vehicle, destroy him too
	if(Driver != None)
		Driver.Destroy();

	// Decrease number of cars active out of parent factory
	if(ParentFactory != None)
		ParentFactory.VehicleCount--;

	// Trigger any effects for destruction
	if(DestroyEffectClass != None)
		spawn(DestroyEffectClass, , , Location, Rotation);

	// Destroy shadow projector
    if (VehicleShadow != None) 
		VehicleShadow.Destroy();

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
}

// This will get called if we couldn't move a pawn out of the way. 
function bool EncroachingOn( actor Other )
{
	if ( Other == None )
		return false;

	// If its a non-vehicle pawn, do lots of damage.
	if( Pawn(Other) != None && SVehicle(Other) == None )
	{
		Other.TakeDamage(10000, None, Other.Location, vect(0,0,0), class'Crushed');
		return false;
	}
}

// Glue a shadow projector on
simulated function PostBeginPlay()
{
    Super.PostBeginPlay();

    if (Level.NetMode != NM_DedicatedServer)
    {
        VehicleShadow = Spawn(class'ShadowProjector', self, '', Location);
        VehicleShadow.ShadowActor = self;
        VehicleShadow.bBlobShadow = false;
        VehicleShadow.LightDirection = Normal(vect(1,1,6));
        VehicleShadow.LightDistance = 1200;
        VehicleShadow.MaxTraceDistance = 350;
        VehicleShadow.InitShadow();
    }
}

// Special calc-view for vehicles
simulated function bool SpecialCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local vector CamLookAt, HitLocation, HitNormal, OffsetVector;
	local PlayerController pc;
	local quat CarQuat, LookQuat, ResultQuat;

	pc = PlayerController(Controller);

	// Only do this mode we have a playercontroller viewing this vehicle
	if(pc == None || pc.ViewTarget != self)
		return false;

	if(pc.bBehindView) ///// THIRD PERSON ///////
	{
		ViewActor = self;
		CamLookAt = Location + (TPCamLookat >> Rotation); 

		OffsetVector = vect(0, 0, 0);
		OffsetVector.X = -1.0 * TPCamDistance;

		CameraLocation = CamLookAt + (OffsetVector >> CameraRotation);

		if( Trace( HitLocation, HitNormal, CameraLocation, CamLookAt, false, vect(10, 10, 10) ) != None )
		{
			CameraLocation = HitLocation;
		}

		bOwnerNoSee = false;

		if(bDrawDriverInTP)
			Driver.bOwnerNoSee = false;
		else
			Driver.bOwnerNoSee = true;
	}
	else ////// FIRST PERSON //////
	{
		ViewActor = self;

		// Camera position is locked to car
		CameraLocation = Location + (FPCamPos >> Rotation);

		//Log(CameraRotation);
		CarQuat = QuatFromRotator(Rotation);

		// Limit where you can look while driving.

		Normalize(CameraRotation); // Puts each element between +/- 32767

		if(CameraRotation.Yaw > MaxViewYaw)
			CameraRotation.Yaw = MaxViewYaw;
		else if(CameraRotation.Yaw < -MaxViewYaw)
			CameraRotation.Yaw = -MaxViewYaw;

		//if(CameraRotation.Pitch > MaxViewPitch)
		//	CameraRotation.Pitch = MaxViewPitch;
		//else if(CameraRotation.Pitch < -MaxViewPitch)
		//	CameraRotation.Pitch = -MaxViewPitch;

		//pc.DesiredRotation = CameraRotation;
		//pc.SetRotation(CameraRotation);

		LookQuat = QuatFromRotator(CameraRotation);
		ResultQuat = QuatProduct(LookQuat, CarQuat);
		CameraRotation = QuatToRotator(ResultQuat);

		if(bDrawMeshInFP)
			bOwnerNoSee = false;
		else
			bOwnerNoSee = true;

		Driver.bOwnerNoSee = true; // In first person, dont draw the driver
	}

	return true;
}

// Includes properties from KActor
defaultproperties
{
    Steering=0
    Throttle=0
	VehicleMass=1.0

	ExitPositions(0)=(X=0,Y=0,Z=0)

	DrivePos=(X=0,Y=0,Z=0)
	bZeroPCRotOnEntry=true

	TPCamLookat=(X=-100,Y=0,Z=100)
	TPCamDistance=600

	MaxViewYaw=16000
	MaxViewPitch=16000

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
	bAlwaysRelevant=True
	RemoteRole=ROLE_SimulatedProxy
	bNetInitialRotation=True
	bSpecialCalcView=True
	//bSpecialHUD=true
}