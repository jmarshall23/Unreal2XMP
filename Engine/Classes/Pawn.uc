//=============================================================================
// Pawn, the base class of all actors that can be controlled by players or AI.
//
// Pawns are the physical representations of players and creatures in a level.  
// Pawns have a mesh, collision, and physics.  Pawns can take damage, make sounds, 
// and hold weapons and other inventory.  In short, they are responsible for all 
// physical interaction between the player or AI and the world.
//
// This is a built-in Unreal class and it shouldn't be modified.
//=============================================================================
class Pawn extends Actor 
	native
	placeable
	config(user)
	nativereplication;

#exec Texture Import File=Textures\Pawn.pcx Name=S_Pawn Mips=Off MASKED=1

//-----------------------------------------------------------------------------
// Pawn variables.

var Controller Controller;

// cache net relevancy test
var float NetRelevancyTime;
var playerController LastRealViewer;
var actor LastViewer;

// Physics related flags.
var bool		bJustLanded;		// used by eyeheight adjustment
var bool		bUpAndOut;			// used by swimming 
var bool		bIsWalking;			// currently walking (can't jump, affects animations)
var bool		bWarping;			// Set when travelling through warpzone (so shouldn't telefrag)
var bool		bWantsToCrouch;		// if true crouched (physics will automatically reduce collision height to CrouchHeight)
var const bool	bIsCrouched;		// set by physics to specify that pawn is currently crouched
var const bool	bTryToUncrouch;		// when auto-crouch during movement, continually try to uncrouch
var() bool		bCanCrouch;			// if true, this pawn is capable of crouching
var bool		bCrawler;			// crawling - pitch and roll based on surface pawn is on
var const bool	bReducedSpeed;		// used by movement natives
var bool		bJumpCapable;
var	bool		bCanJump;			// movement capabilities - used by AI
var	bool 		bCanWalk;
var	bool		bCanSwim;
var	bool		bCanFly;
var	bool		bCanClimbLadders;
var	bool		bCanStrafe;
var	bool		bAvoidLedges;		// don't get too close to ledges
var	bool		bStopAtLedges;		// if bAvoidLedges and bStopAtLedges, Pawn doesn't try to walk along the edge at all
var	bool		bNoJumpAdjust;		// set to tell controller not to modify velocity of a jump/fall	
var	bool		bCountJumps;		// if true, inventory wants message whenever this pawn jumps
var const bool	bSimulateGravity;	// simulate gravity for this pawn on network clients when predicting position (true if pawn is walking or falling)
var	bool		bUpdateEyeheight;	// if true, UpdateEyeheight will get called every tick
var	bool		bIgnoreForces;		// if true, not affected by external forces
var const bool	bNoVelocityUpdate;	// used by C++ physics
var	bool		bCanWalkOffLedges;	// Can still fall off ledges, even when walking (for Player Controlled pawns)
var bool		bSteadyFiring;		// used for third person weapon anims/effects
var bool		bClientCollision;	// used on clients when temporarily turning off collision
var const bool	bSimGravityDisabled;	// used on network clients
var bool		bDirectHitWall;		// always call pawn hitwall directly (no controller notifyhitwall)

// used by dead pawns (for bodies landing and changing collision box)
var		bool	bThumped;		
var		bool	bInvulnerableBody;

// AI related flags
var		bool	bIsFemale;
var		bool	bAutoActivate;			// if true, automatically activate Powerups which have their bAutoActivate==true
var		bool	bCanPickupInventory;	// if true, will pickup inventory when touching pickup actors
var		bool	bUpdatingDisplay;		// to avoid infinite recursion through inventory setdisplay
var		bool	bAmbientCreature;		// AIs will ignore me
var(AI) bool	bLOSHearing;			// can hear sounds from line-of-sight sources (which are close enough to hear)
										// bLOSHearing=true is like UT/Unreal hearing
var(AI) bool	bSameZoneHearing;		// can hear any sound in same zone (if close enough to hear)
var(AI) bool	bAdjacentZoneHearing;	// can hear any sound in adjacent zone (if close enough to hear)
var(AI) bool	bMuffledHearing;		// can hear sounds through walls (but muffled - sound distance increased to double plus 4x the distance through walls
var(AI) bool	bAroundCornerHearing;	// Hear sounds around one corner (slightly more expensive, and bLOSHearing must also be true)
//OLD (mdf) var(AI) bool	bDontPossess;			// if true, Pawn won't be possessed at game start
var		bool	bAutoFire;				// used for third person weapon anims/effects
var		bool	bRollToDesired;			// Update roll when turning to desired rotation (normally false)
var		bool	bIgnorePlayFiring;		// if true, ignore the next PlayFiring() call (used by AnimNotify_FireWeapon)

var		bool	bCachedRelevant;		// network relevancy caching flag
var		bool	bUseCompressedPosition;	// use compressed position in networking - true unless want to replicate roll, or very high velocities
var		globalconfig bool bWeaponBob;
var		bool    bSpecialCalcView;		// If true, the Controller controlling this pawn will call 'SpecialCalcView' to find camera pos.
var		bool	bNoWeaponFiring;
var		bool	bIsTyping; 
var		bool	bCanBeRevived;			// If true, pawn can be brought back to life.

var(Advanced) bool bUseEnabled;			// If true, use functionality is enabled

var		byte	FlashCount;				// used for third person weapon anims/effects

var		byte	DamageIndicatorAngle;	// use for hit indicator on hud

var		int		IconIndex;				// Use for differentiating classes on hud / scoreboard / etc.

// AI basics.
var 	byte	Visibility;			//How visible is the pawn? 0=invisible, 128=normal, 255=highly visible 
var		float	DesiredSpeed;
var		float	MaxDesiredSpeed;
var(AI) name	AIScriptTag;		// tag of AIScript which should be associated with this pawn
var(AI) float	HearingThreshold;	// max distance at which a makenoise(1.0) loudness sound can be heard
var(AI)	float	Alertness;			// -1 to 1 ->Used within specific states for varying reaction to stimuli 
var(AI)	float	SightRadius;		// Maximum seeing distance.
var(AI)	float	PeripheralVision;	// Cosine of limits of peripheral vision.
var()	float	SkillModifier;			// skill modifier (same scale as game difficulty)	
var const float	AvgPhysicsTime;		// Physics updating time monitoring (for AI monitoring reaching destinations)
var		float	MeleeRange;			// Max range for melee attack (not including collision radii)
var NavigationPoint Anchor;			// current nearest path;
var const NavigationPoint LastAnchor;		// recent nearest path
var		float	FindAnchorFailedTime;	// last time a FindPath() attempt failed to find an anchor.
var		float	LastValidAnchorTime;	// last time a valid anchor was found
var		float	DestinationOffset;	// used to vary destination over NavigationPoints
var		float	NextPathRadius;		// radius of next path in route
var		vector	SerpentineDir;		// serpentine direction
var		float	SerpentineDist;
var		float	SerpentineTime;		// how long to stay straight before strafing again
var const float	UncrouchTime;		// when auto-crouch during movement, continually try to uncrouch once this decrements to zero
var		float	SpawnTime;

// Movement.
var float   GroundSpeed;    // The maximum ground speed.
var float   WaterSpeed;     // The maximum swimming speed.
var float   AirSpeed;		// The maximum flying speed.
var float	LadderSpeed;	// Ladder climbing speed
var float	AccelRate;		// max acceleration rate
var float	JumpZ;      	// vertical acceleration w/ jump
var float   AirControl;		// amount of AirControl available to the pawn
var float	WalkingPct;		// pct. of running speed that walking speed is
var float	CrouchedPct;	// pct. of running speed that crouched walking speed is
var float	MaxFallSpeed;	// max speed pawn can land without taking damage (also limits what paths AI can use)
var vector	ConstantAcceleration;	// acceleration added to pawn when falling

// Player info.
var	string			OwnerName;		// Name of owning player (for save games, coop)
var travel Weapon	Weapon;			// The pawn's current weapon.
var Weapon			PendingWeapon;	// Will become weapon once current weapon is put down
var travel Powerups	SelectedItem;	// currently selected inventory item
var float      		BaseEyeHeight; 	// Base eye height above collision center.
var float        	EyeHeight;     	// Current eye height, adjusted for bobbing and stairs.
var	const vector	Floor;			// Normal of floor pawn is standing on (only used by PHYS_Spider and PHYS_Walking)
var float			SplashTime;		// time of last splash
var float			CrouchHeight;	// CollisionHeight when crouching
var float			CrouchRadius;	// CollisionRadius when crouching
var float			OldZ;			// Old Z Location - used for eyeheight smoothing
var PhysicsVolume	HeadVolume;		// physics volume of head
var travel int      Health;         // Health: 100 = normal maximum
var	float			BreathTime;		// used for getting BreathTimer() messages (for no air, etc.)
var float			UnderWaterTime; // how much time pawn can go without air (in seconds)
var	float			LastPainTime;	// last time pawn played a takehit animation (updated in PlayHit())
var class<DamageType> ReducedDamageType; // which damagetype this creature is protected from (used by AI)
var float			HeadScale;

var array< class<Weapon> > DefaultWeapons;
var array< string > DefaultWeaponNames;

// Sound and noise management
// remember location and position of last noises propagated
var const 	vector 		noise1spot;
var const 	float 		noise1time;
var const	pawn		noise1other;
var const	float		noise1loudness;
var const 	vector 		noise2spot;
var const 	float 		noise2time;
var const	pawn		noise2other;
var const	float		noise2loudness;
var			float		LastPainSound;

// view bob
var				globalconfig float Bob;
var				float				LandBob, AppliedBob;
var				float bobtime;
var				vector			WalkBob;

var float SoundDampening;
var float DamageScaling;

var localized  string MenuName; // Name used for this pawn type in menus (e.g. player selection) 

// shadow decal
var ShadowProjector Shadow;

// blood effect
var class<Effects> BloodEffect;
var class<Effects> LowGoreBlood;

var class<AIController> ControllerClass;	// default class to use when pawn is controlled by AI (can be modified by an AIScript)

var PlayerReplicationInfo PlayerReplicationInfo;

var LadderVolume OnLadder;		// ladder currently being climbed

var name LandMovementState;		// PlayerControllerState to use when moving on land or air
var name WaterMovementState;	// PlayerControllerState to use when moving in water

var PlayerStart LastStartSpot;	// used to avoid spawn camping
var float LastStartTime;

// Animation status
var name AnimAction;			// use for replicating anims 
var name DeathAnim;				// hack to know which death anim to use for proxies (when they become relevant)

// Animation updating by physics FIXME - this should be handled as an animation object
// Note that animation channels 2 through 11 are used for animation updating
var vector TakeHitLocation;		// location of last hit (for playing hit/death anims)
var class<DamageType> HitDamageType;	// damage type of last hit (for playing hit/death anims)
var vector TearOffMomentum;		// momentum to apply when torn off (bTearOff == true)
var bool bPhysicsAnimUpdate;	
var bool bWasCrouched;
var bool bWasWalking;
var bool bWasOnGround;
var bool bInitializeAnimation;
var bool bPlayedDeath;
var EPhysics OldPhysics;
var float OldRotYaw;			// used for determining if pawn is turning
var vector OldAcceleration;
var float BaseMovementRate;		// FIXME - temp - used for scaling movement
var name MovementAnims[4];		// Forward, Back, Left, Right
var name TurnLeftAnim;
var name TurnRightAnim;			// turning anims when standing in place (scaled by turn speed)
var(AnimTweaks) float BlendChangeTime;	// time to blend between movement animations
var float MovementBlendStartTime;	// used for delaying the start of run blending
var float ForwardStrafeBias;	// bias of strafe blending in forward direction
var float BackwardStrafeBias;	// bias of strafe blending in backward direction

var transient CompressedPosition PawnPosition;

var int TeamIndex; // used if no PlayerReplicationInfo

//-----------------------------------------------------------------------------
// animations

var byte ViewPitch;			// jjs - something to replicate so we can see which way remote clients are looking
var name SwimAnims[4];      // 0=forward, 1=backwards, 2=left, 3=right
var name CrouchAnims[4];
var name WalkAnims[4];
var name AirAnims[4];
var name TakeoffAnims[4];
var name LandAnims[4];
var name DoubleJumpAnims[4];
var name DodgeAnims[4];
var name AirStillAnim;
var name TakeoffStillAnim;
var name CrouchTurnRightAnim;
var name CrouchTurnLeftAnim;
var name IdleCrouchAnim;
var name IdleSwimAnim;
var name IdleWeaponAnim;    // WeaponAttachment code will set this one
var name IdleRestAnim;

var name WallDodgeAnims[4];
var name IdleHeavyAnim;
var name IdleRifleAnim;
var name FireHeavyRapidAnim;
var name FireHeavyBurstAnim;
var name FireRifleRapidAnim;
var name FireRifleBurstAnim;
var name FireRootBone;

var array<name> TauntAnims; // Array of names of taunt anim that can be played by this character. First 4 assumed to be orders.
var localized string TauntAnimNames[8]; // Text description

var bool bIsIdle;           // true when standing still on the ground, Physics can be used for determining other states
var bool bWaitForAnim;      // true if the pawn is playing an important non-looping animation (eg. landing/dodge) and doesn't feel like being interrupted
var const bool bReverseRun;
var bool bDoTorsoTwist;
var const bool FootTurning;
var const bool FootStill;

var float DodgeSpeedFactor; // dodge speed moved here so animation knows the diff between a jump and a dodge
var float DodgeSpeedZ;

var const int OldAnimDir;
var const Vector OldVelocity;

var float IdleTime;
var const int FootRot;     // torso twisting/looking stuff
var const int TurnDir;
var name RootBone;
var name HeadBone;
var name SpineBone1;
var name SpineBone2;

var const Pawn nextPawn; //NEW (mdf) chained PawnList

replication
{
	// Variables the server should send to the client.
	reliable if( bNetDirty && (Role==ROLE_Authority) )
        bSimulateGravity, bIsCrouched, bIsWalking, bIsTyping, PlayerReplicationInfo, AnimAction, HitDamageType, TakeHitLocation, HeadScale;
    reliable if( bNetDirty && Health <= 0 && Role==ROLE_Authority )
		DeathAnim;
	reliable if( bTearOff && bNetDirty && (Role==ROLE_Authority) )
		TearOffMomentum;	
	reliable if ( bNetDirty && !bNetOwner && (Role==ROLE_Authority) ) 
		bSteadyFiring;
	reliable if( bNetDirty && bNetOwner && Role==ROLE_Authority )
         Controller,SelectedItem, GroundSpeed, WaterSpeed, AirSpeed, AccelRate, JumpZ, AirControl, DamageIndicatorAngle;
	reliable if( bNetDirty && Role==ROLE_Authority )
         Health, bCanBeRevived;
    unreliable if ( !bNetOwner && Role==ROLE_Authority )
		PawnPosition;

	// replicated functions sent to server by owning client
	reliable if( Role<ROLE_Authority )
		ServerChangedWeapon;
}

native final function AddPawn(); //NEW (mdf)
native final function RemovePawn(); //NEW (mdf)

simulated native function SetTwistLook( int twist, int look ); //NEW (mdf)
simulated event SetHeadScale(float NewScale);
simulated native function int Get4WayDirection( ); //NEW (mdf)

native function bool ReachedDestination(Actor Goal);
native function ForceCrouch();

interface function float GetHealthPct() { return float(Health) / float(default.Health); }
interface function float GetHealth() { return FMax(0,Health); }

interface function float GetDamageIndicatorAngleA(){ return float(DamageIndicatorAngle)*360.0/256.0 + 21.9; }
interface function float GetDamageIndicatorAngleB(){ return float(DamageIndicatorAngle)*360.0/256.0 - 21.9; }

interface function string GetPlayerName() { return PlayerReplicationInfo.PlayerName; }
interface function int GetPlayerClassIndex() { return 3; }

interface function bool IsDead() { return Health <= 0; }
interface function bool IsAlive() { return Health > 0; }

interface function bool LowHealth()	{ return false; }
interface function bool LowEnergy()	{ return false; }
interface function bool LowAmmo()	{ return false; }

interface function bool RecentlyTalked()		{ return PlayerReplicationInfo? && (Level.TimeSeconds - PlayerReplicationInfo.LastTalkTime) < 5.0; }
interface function float RecentlyTalkedPct()	{ return ABlend( PlayerReplicationInfo.LastTalkTime + 5.0, PlayerReplicationInfo.LastTalkTime, Level.TimeSeconds ); }
interface function float RecentlyTalkedAlpha()	{ return FClamp( RecentlyTalkedPct() * 255.0, 0.0, 255.0 ); }

interface function bool FlashbangEnabled()	{ return false; }

interface function bool GetPassengerEnabled0() { return false; }
interface function bool GetPassengerEnabled1() { return false; }
interface function bool GetPassengerEnabled2() { return false; }
interface function bool GetPassengerEnabled3() { return false; }
interface function bool GetPassengerEnabled4() { return false; }
interface function bool GetPassengerEnabled5() { return false; }
interface function bool GetPassengerEnabled6() { return false; }
interface function bool GetPassengerEnabled7() { return false; }

interface function float GetPIDDistancePct() { return 1.0; }

function array< class<Weapon> > MyDefaultWeapons()
{
	local int i;
	if ( DefaultWeapons.length == 0 )
		for ( i=0; i<DefaultWeaponNames.length; i++ )
			if ( DefaultWeaponNames[i] != "" )
				DefaultWeapons[i] = class<Weapon>(DynamicLoadObject(DefaultWeaponNames[i], class'Class'));
	return DefaultWeapons;
}

simulated function Weapon GetDemoRecordingWeapon()
{
	local inventory Inv;
	
	for ( Inv=Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		if ( (Weapon(Inv) != None) && (Inv.ThirdPersonActor != None) )
		{
			Weapon = Weapon(Inv);
			PendingWeapon = Weapon;
//!!MERGE	Weapon.bSpectated = true;
			break;
		}
	}
	return Weapon;
}

/* Reset() 
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	if ( (Controller == None) || Controller.bIsPlayer )
		Destroy();
	else
		Super.Reset();
}

function Fire( optional float F )
{
    if( Weapon!=None )
        Weapon.Fire();
}

function DrawHUD(Canvas Canvas);

// If returns false, do normal calview anyway
function bool SpecialCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation );

simulated function String GetHumanReadableName()
{
	if ( PlayerReplicationInfo != None )
		return PlayerReplicationInfo.PlayerName;
	return MenuName;
}

function PlayTeleportEffect(bool bOut, bool bSound)
{
	MakeNoise(1.0);
}

/* PossessedBy()
 Pawn is possessed by Controller
*/
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
	Controller = None;
}

static function string GetQualifiedClassName( int Team )
{
	return string( default.Class );
}

/* PointOfView()
called by controller when possessing this pawn
false = 1st person, true = 3rd person
*/
simulated function bool PointOfView()
{
	return false;
}

function BecomeViewTarget()
{
	bUpdateEyeHeight = true;
}

function DropToGround()
{
	bCollideWorld = True;
	bInterpolating = false;
	if ( Health > 0 )
	{
		SetCollision(true,true,true);
		SetPhysics(PHYS_Falling);
		AmbientSound = None;
		if ( IsHumanControlled() )
			Controller.GotoState(LandMovementState);
	}
}

function bool CanGrabLadder()
{
	return ( bCanClimbLadders 
			&& (Controller != None)
			&& (Physics != PHYS_Ladder)
			&& ((Physics != Phys_Falling) || (abs(Velocity.Z) <= JumpZ)) );
}

event SetWalking(bool bNewIsWalking)
{
	if ( bNewIsWalking != bIsWalking )
	{
		bIsWalking = bNewIsWalking;
		ChangeAnimation();
	}
}

function SetSprint( bool bEnabled );

function bool CanSplash()
{
	if ( (Level.TimeSeconds - SplashTime > 0.25)
		&& ((Physics == PHYS_Falling) || (Physics == PHYS_Flying))
		&& (Abs(Velocity.Z) > 100) )
	{
		SplashTime = Level.TimeSeconds;
		return true;
	}
	return false;
}

function EndClimbLadder(LadderVolume OldLadder)
{
	if ( Controller != None )
		Controller.EndClimbLadder();
	if ( Physics == PHYS_Ladder )
		SetPhysics(PHYS_Falling);
}

function ClimbLadder(LadderVolume L)
{
	OnLadder = L;
	SetRotation(OnLadder.WallDir);
	SetPhysics(PHYS_Ladder);
	if ( IsHumanControlled() )
		Controller.GotoState('PlayerClimbing');
}

/* DisplayDebug()
list important actor variable on canvas.  Also show the pawn's controller and weapon info
*/
simulated function DisplayDebug(Canvas Canvas, out float YL, out float YPos)
{
	local string T;
	Super.DisplayDebug(Canvas, YL, YPos);

	Canvas.SetDrawColor(255,255,255);

	Canvas.DrawText("Animation Action "$AnimAction$" Health "$Health);
	YPos += YL;
	Canvas.SetPos(4,YPos);
	Canvas.DrawText("Anchor "$Anchor$" Serpentine Dist "$SerpentineDist$" Time "$SerpentineTime);
	YPos += YL;
	Canvas.SetPos(4,YPos);

	T = "Floor "$Floor$" DesiredSpeed "$DesiredSpeed$" Crouched "$bIsCrouched$" Try to uncrouch "$UncrouchTime;
	if ( (OnLadder != None) || (Physics == PHYS_Ladder) )
		T=T$" on ladder "$OnLadder;
	Canvas.DrawText(T);
	YPos += YL;
	Canvas.SetPos(4,YPos);
	Canvas.DrawText("EyeHeight "$Eyeheight$" BaseEyeHeight "$BaseEyeHeight$" Physics Anim "$bPhysicsAnimUpdate);
	YPos += YL;
	Canvas.SetPos(4,YPos);

	if ( Controller == None )
	{
		Canvas.SetDrawColor(255,0,0);
		Canvas.DrawText("NO CONTROLLER");
		YPos += YL;
		Canvas.SetPos(4,YPos);
	}
	else
		Controller.DisplayDebug(Canvas,YL,YPos);

	if ( Weapon == None )
	{
		Canvas.SetDrawColor(0,255,0);
		Canvas.DrawText("NO WEAPON");
		YPos += YL;
		Canvas.SetPos(4,YPos);
	}
	else
		Weapon.DisplayDebug(Canvas,YL,YPos);
}
		 		
//
// Compute offset for drawing an inventory item.
//
simulated function vector CalcDrawOffset(inventory Inv)
{
	local vector DrawOffset;

	if ( Controller == None )
		return (Inv.PlayerViewOffset >> Rotation) + BaseEyeHeight * vect(0,0,1);

	DrawOffset = ((0.9/Weapon.DisplayFOV * 100 * ModifiedPlayerViewOffset(Inv)) >> GetViewRotation() ); 
	if ( !IsLocallyControlled() )
		DrawOffset.Z += BaseEyeHeight;
	else
	{	
		DrawOffset.Z += EyeHeight;
        if( bWeaponBob )
			DrawOffset += WeaponBob(Inv.BobDamping);
		DrawOffset += PlayerController(Controller).ShakeOffset * (0.45 + 0.55 * Inv.BobDamping);
	}
	return DrawOffset;
}

function vector ModifiedPlayerViewOffset(inventory Inv)
{
	return Inv.PlayerViewOffset;
}

function vector WeaponBob(float BobDamping)
{
	Local Vector WBob;

	WBob = BobDamping * WalkBob;
	WBob.Z = (0.45 + 0.55 * BobDamping) * WalkBob.Z;
	return WBob;
}

function CheckBob(float DeltaTime, vector Y)
{
	local float Speed2D;

    if( !bWeaponBob )
    {
		BobTime = 0;
		WalkBob = Vect(0,0,0);
        return;
    }
	Bob = FClamp(Bob, -0.03, 0.03);
	if (Physics == PHYS_Walking )
	{
		Speed2D = VSize(Velocity);
		if ( Speed2D < 10 )
			BobTime += 0.2 * DeltaTime;
		else
			BobTime += DeltaTime * (0.3 + 0.7 * Speed2D/GroundSpeed);
		WalkBob = Y * Bob * Speed2D * sin(8 * BobTime);
		AppliedBob = AppliedBob * (1 - FMin(1, 16 * deltatime));
		WalkBob.Z = AppliedBob;
		if ( Speed2D > 10 )
			WalkBob.Z = WalkBob.Z + 0.75 * Bob * Speed2D * sin(16 * BobTime);
		if ( LandBob > 0.01 )
		{
			AppliedBob += FMin(1, 16 * deltatime) * LandBob;
			LandBob *= (1 - 8*Deltatime);
		}
	}
	else if ( Physics == PHYS_Swimming )
	{
		Speed2D = Sqrt(Velocity.X * Velocity.X + Velocity.Y * Velocity.Y);
		WalkBob = Y * Bob *  0.5 * Speed2D * sin(4.0 * Level.TimeSeconds);
		WalkBob.Z = Bob * 1.5 * Speed2D * sin(8.0 * Level.TimeSeconds);
	}
	else
	{
		BobTime = 0;
		WalkBob = WalkBob * (1 - FMin(1, 8 * deltatime));
	}
}
	
//***************************************
// Interface to Pawn's Controller

// return true if controlled by a Player (AI or human)
simulated function bool IsPlayerPawn()
{
	return ( (Controller != None) && Controller.bIsPlayer );
}

// return true if was controlled by a Player (AI or human)
simulated function bool WasPlayerPawn()
{
	return false;
}

// return true if controlled by a real live human
simulated function bool IsHumanControlled()
{
	return ( PlayerController(Controller) != None );
}

// return true if controlled by local (not network) player
simulated function bool IsLocallyControlled()
{
	if ( Level.NetMode == NM_Standalone )
		return true;
	if ( Controller == None )
		return false;
	if ( PlayerController(Controller) == None )
		return true;

	return ( Viewport(PlayerController(Controller).Player) != None );
}

// return true if viewing this pawn in first person pov. useful for determining what and where to spawn effects
simulated function bool IsFirstPerson()
{
    local PlayerController PC;
 
    PC = PlayerController(Controller);
    return ( PC != None && !PC.bBehindView && Viewport(PC.Player) != None );
}

simulated function vector GetViewLocation()
{
	return Location;
}

simulated function rotator GetViewRotation()
{
	if ( Controller == None )
		return Rotation;
	return Controller.GetViewRotation();
}

simulated function SetViewRotation(rotator NewRotation )
{
	if ( Controller != None )
		Controller.SetRotation(NewRotation);
}

final function bool InGodMode()
{
	return ( (Controller != None) && Controller.bGodMode );
}

function bool NearMoveTarget()
{
	if ( (Controller == None) || (Controller.MoveTarget == None) )
		return false;

	return ReachedDestination(Controller.MoveTarget);
}

simulated final function bool PressingFire()
{
	return ( (Controller != None) && (Controller.bFire != 0) );
}

simulated final function bool PressingAltFire()
{
	return ( (Controller != None) && (Controller.bAltFire != 0) );
}

function bool ShouldAutoSwitch()
{
	return true;
}

function Actor GetMoveTarget()
{	
	if ( Controller == None )
		return None;

	return Controller.MoveTarget;
}

function SetMoveTarget(Actor NewTarget )
{
	if ( Controller != None )
		Controller.MoveTarget = NewTarget;
}

function bool LineOfSightTo(actor Other)
{
	return ( (Controller != None) && Controller.LineOfSightTo(Other) );
} 

simulated final function bool AdjustAim( class<Projectile> ProjectileClass, float ProjSpeed, vector ProjStart, float WeaponFireSpread, bool bLeadTarget, bool bWarnTarget, bool bTossed, bool bTrySplash )
{
	if( Controller == None )
		return true;

	return Controller.AdjustAim( ProjectileClass, ProjSpeed, ProjStart, WeaponFireSpread, bLeadTarget, bWarnTarget, bTossed, bTrySplash );
}

simulated final function rotator GetAimRotation()
{
	if ( Controller == None )
		return Rotation;
	else
		return Controller.GetAimRotation();
}

function Actor ShootSpecial(Actor A)
{
	if ( !Controller.bCanDoSpecial || (Weapon == None) )
		return None;

	Controller.FireWeaponAt(A);
	Controller.bFire = 0;
	return A;
}

/* return a value (typically 0 to 1) adjusting pawn's perceived strength if under some special influence (like berserk)
*/
function float AdjustedStrength()
{
	return 0;
}

function HandlePickup(Pickup pick)
{
	MakeNoise(0.2);
	if ( Controller != None )
		Controller.HandlePickup(pick);
}

function ReceiveLocalizedMessage( class<LocalMessage> Message, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	if ( PlayerController(Controller) != None )
		PlayerController(Controller).ReceiveLocalizedMessage( Message, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject );
}

event ClientMessage( coerce string S, optional Name Type )
{
	if ( PlayerController(Controller) != None )
		PlayerController(Controller).ClientMessage( S, Type );
}

function Trigger( actor Other, pawn EventInstigator )
{
	if ( Controller != None )
		Controller.Trigger(Other, EventInstigator);
}

//***************************************

function bool CanTrigger(Trigger T)
{
	return true;
}

function CreateInventory(string InventoryClassName)
{
}

function GiveWeapon(string aClassName )
{
	local class<Weapon> WeaponClass;
	local Weapon NewWeapon;

	WeaponClass = class<Weapon>(DynamicLoadObject(aClassName, class'Class'));

	if( FindInventoryType(WeaponClass) != None )
		return;
	newWeapon = Spawn(WeaponClass);
	if( newWeapon != None )
		newWeapon.GiveTo(self);
}

function SetDisplayProperties(ERenderStyle NewStyle, Material NewTexture, bool bLighting )
{
	Style = NewStyle;
	Texture = NewTexture;
	bUnlit = bLighting;
	if ( Weapon != None )
		Weapon.SetDisplayProperties(Style, Texture, bUnlit);

	if ( !bUpdatingDisplay && (Inventory != None) )
	{
		bUpdatingDisplay = true;
		Inventory.SetOwnerDisplay();
	}
	bUpdatingDisplay = false;
}

function SetDefaultDisplayProperties()
{
	Style = Default.Style;
	texture = Default.Texture;
	bUnlit = Default.bUnlit;
	if ( Weapon != None )
		Weapon.SetDefaultDisplayProperties();

	if ( !bUpdatingDisplay && (Inventory != None) )
	{
		bUpdatingDisplay = true;
		Inventory.SetOwnerDisplay();
	}
	bUpdatingDisplay = false;
}

function FinishedInterpolation()
{
	DropToGround();
}

function JumpOutOfWater(vector jumpDir)
{
	Falling();
	Velocity = jumpDir * WaterSpeed;
	Acceleration = jumpDir * AccelRate;
	velocity.Z = FMax(380,JumpZ); //set here so physics uses this for remainder of tick
	bUpAndOut = true;
}

/*
Modify velocity called by physics before applying new velocity
for this tick.

Velocity,Acceleration, etc. have been updated by the physics, but location hasn't
*/
simulated event ModifyVelocity(float DeltaTime, vector OldVelocity);

event FellOutOfWorld(eKillZType KillType)
{
	if ( Role < ROLE_Authority )
		return;
	if ( (Controller != None) && Controller.AvoidCertainDeath() )
		return;
	Health = -1;

	if( KillType == KILLZ_Lava)
		Died( None, class'FellLava', Location );
	else if(KillType == KILLZ_Suicide)
		Died( None, class'Fell', Location );
	else
	{
	SetPhysics(PHYS_None);
		Died( None, class'Fell', Location );
	}
}

/* ShouldCrouch()
Controller is requesting that pawn crouch
*/
function ShouldCrouch(bool Crouch)
{
	bWantsToCrouch = Crouch;
}

// Stub events called when physics actually allows crouch to begin or end
// use these for changing the animation (if script controlled)
event EndCrouch(float HeightAdjust)
{
	EyeHeight -= HeightAdjust;
	OldZ += HeightAdjust;
	BaseEyeHeight = Default.BaseEyeHeight;
}

event StartCrouch(float HeightAdjust)
{
	EyeHeight += HeightAdjust;
	OldZ -= HeightAdjust;
	BaseEyeHeight = FMin(0.8 * CrouchHeight, CrouchHeight - 10);
}

function RestartPlayer();
function AddVelocity( vector NewVelocity)
{
	if ( bIgnoreForces || (NewVelocity == vect(0,0,0)) )
		return;
	if ( (Physics == PHYS_Walking)
		|| (((Physics == PHYS_Ladder) || (Physics == PHYS_Spider)) && (NewVelocity.Z > Default.JumpZ)) )
		SetPhysics(PHYS_Falling);
	if ( (Velocity.Z > 380) && (NewVelocity.Z > 0) )
		NewVelocity.Z *= 0.5;
	Velocity += NewVelocity;
}

function KilledBy( pawn EventInstigator )
{
	local Controller Killer;

	Health = 0;
	if ( EventInstigator != None )
		Killer = EventInstigator.Controller;
	Died( Killer, class'Suicided', Location );
}

function TakeFallingDamage()
{
	local float Shake, EffectiveSpeed;

	if (Velocity.Z < -0.5 * MaxFallSpeed)
	{
		if ( Role == ROLE_Authority )
		{
		    MakeNoise(1.0);
		    if (Velocity.Z < -1 * MaxFallSpeed)
		    {
			    EffectiveSpeed = Velocity.Z;
			    if ( TouchingWaterVolume() )
					EffectiveSpeed = FMin(0, EffectiveSpeed + 100);
			    if ( EffectiveSpeed < -1 * MaxFallSpeed )
				    TakeDamage(-100 * (EffectiveSpeed + MaxFallSpeed)/MaxFallSpeed, None, Location, vect(0,0,0), class'Fell');
		    }
		}
		if ( Controller != None )
		{
			Shake = FMin(1, -1 * Velocity.Z/MaxFallSpeed);
			Controller.ShakeView(Shake);
		}
	}
	else if (Velocity.Z < -1.4 * JumpZ)
		MakeNoise(0.5);
}

function ClientReStart()
{
	Velocity = vect(0,0,0);
	Acceleration = vect(0,0,0);
	BaseEyeHeight = Default.BaseEyeHeight;
	EyeHeight = BaseEyeHeight;
	PlayWaiting();
}

function ClientSetLocation( vector NewLocation, rotator NewRotation )
{
	if ( Controller != None )
		Controller.ClientSetLocation(NewLocation, NewRotation);
}

function ClientSetRotation( rotator NewRotation )
{
	if ( Controller != None )
		Controller.ClientSetRotation(NewRotation);
}

simulated function FaceRotation( rotator NewRotation, float DeltaTime )
{
	if ( Physics == PHYS_Ladder )
		SetRotation(OnLadder.Walldir);
	else
	{
		if ( (Physics == PHYS_Walking) || (Physics == PHYS_Falling) )
			NewRotation.Pitch = 0;
		SetRotation(NewRotation);
	}
}

function ClientDying(class<DamageType> DamageType, vector HitLocation)
{
	if ( Controller != None )
		Controller.ClientDying(DamageType, HitLocation);
}

function ClientRevived();

function bool InCurrentCombo()
{
	return false;
}

event TornOff()
{
	AddTimer( 'DestroyTimer', 12.0 );
}

//=============================================================================
// Inventory related functions.

// check before throwing
simulated function bool CanThrowWeapon()
{
    return ( (Weapon != None) && ((Level.Game == None) || !Level.Game.bWeaponStay) );
}

// toss out a weapon
function TossWeapon(Vector TossVel)
{
	local Vector X,Y,Z;

	Weapon.Velocity = TossVel;
	GetAxes(Rotation,X,Y,Z);
	Weapon.DropFrom(Location + 0.8 * CollisionRadius * X - 0.5 * CollisionRadius * Y); 
}	

exec function SwitchToLastWeapon()
{
	if ( (Weapon != None) && (Weapon.OldWeapon != None) && Weapon.OldWeapon.HasAmmo() )
	{
		PendingWeapon = Weapon.OldWeapon;
		Weapon.PutDown();
	}
}

// The player/bot wants to select next item
exec function NextItem()
{
	if (SelectedItem==None) {
		SelectedItem = Inventory.SelectNext();
		Return;
	}
	if (SelectedItem.Inventory!=None)
		SelectedItem = SelectedItem.Inventory.SelectNext(); 
	else
		SelectedItem = Inventory.SelectNext();

	if ( SelectedItem == None )
		SelectedItem = Inventory.SelectNext();
}

// FindInventoryType()
// returns the inventory item of the requested class
// if it exists in this pawn's inventory 

function Inventory FindInventoryType( class DesiredClass )
{
	local Inventory Inv;
	local int Count;

	for( Inv=Inventory; Inv!=None; Inv=Inv.Inventory )   
	{
		if ( Inv.class == DesiredClass )
			return Inv;
		Count++;
		if ( Count > 1000 )
			return None;
	}
	return None;
} 

// Add Item to this pawn's inventory. 
// Returns true if successfully added, false if not.
function bool AddInventory( inventory NewItem )
{
	// Skip if already in the inventory.
	local inventory Inv;
	local actor Last;

	Last = self;
	
	// The item should not have been destroyed if we get here.
	if (NewItem ==None )
		log("tried to add none inventory to "$self);

	for( Inv=Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		if( Inv == NewItem )
			return false;
		Last = Inv;
	}

	// Add to back of inventory chain (so minimizes net replication effect).
	NewItem.SetOwner(Self);
	NewItem.Inventory = None;
	Last.Inventory = NewItem;

	if ( Controller != None )
		Controller.NotifyAddInventory(NewItem);

	NewItem.NotifyAddInventory( Self );

	return true;
}

// Remove Item from this pawn's inventory, if it exists.
function DeleteInventory( inventory Item )
{
	// If this item is in our inventory chain, unlink it.
	local actor Link;
	local int Count;

	if( Controller != None )
		Controller.NotifyDeleteInventory(Item);

	Item.NotifyDeleteInventory( Self );

	if ( Item == Weapon )
		Weapon = None;
	if ( Item == SelectedItem )
		SelectedItem = None;
	for( Link = Self; Link!=None; Link=Link.Inventory )
	{
		if( Link.Inventory == Item )
		{
			Link.Inventory = Item.Inventory;
			Item.Inventory = None;
			break;
		}
		if ( Level.NetMode == NM_Client )
		{
			Count++;
			if ( Count > 1000 )
				break;
		}
	}
	Item.SetOwner(None);
}

// Just changed to pendingWeapon
function ChangedWeapon()
{
	local Weapon OldWeapon;

	OldWeapon = Weapon;

	if (Weapon == PendingWeapon)
	{
		if ( Weapon == None )
		{
			Controller.SwitchToBestWeapon();
			return;
		}
		//!!ARL: This causes weapons that are switched away from and then
		// switched back to before the finish DownWeapon to go directly into
		// the Idle state (thus missing the BringUp call in ServerChangeWeapon)
		// which causes a visual pop and also results in having no crosshair.
		//else if ( Weapon.IsInState('DownWeapon') ) 
		//	Weapon.GotoState('Idle');
		PendingWeapon = None;
		ServerChangedWeapon(OldWeapon, Weapon);
		if (Level.NetMode == NM_Client)
			ClientChangedWeapon(OldWeapon, Weapon);
		return;
	}
	if ( PendingWeapon == None )
		PendingWeapon = Weapon;
		
	Weapon = PendingWeapon;
	if ( (Weapon != None) && (Level.NetMode == NM_Client) )
		Weapon.BringUp(OldWeapon);
	PendingWeapon = None;
	Weapon.Instigator = self;
	ServerChangedWeapon(OldWeapon, Weapon);
	if (Level.NetMode == NM_Client)
		ClientChangedWeapon(OldWeapon, Weapon);
	if ( Controller != None )
		Controller.ChangedWeapon();
}

function name GetWeaponBoneFor(Inventory I)
{
	return 'bone_weapon';
}

function ServerChangedWeapon(Weapon OldWeapon, Weapon W)
{
	ClientChangedWeapon(OldWeapon, W);
}
function ClientChangedWeapon(Weapon OldWeapon, Weapon W)
{
	if ( OldWeapon != None )
	{
		OldWeapon.SetDefaultDisplayProperties();
		OldWeapon.DetachFromPawn(self);		
	}
	Weapon = W;
	if ( Weapon == None )
		return;

	if ( Weapon != None )
	{
		//log("ServerChangedWeapon: Attaching Weapon to actor bone.");
		Weapon.AttachToPawn(self);
	}

	Weapon.SetRelativeLocation(Weapon.Default.RelativeLocation);
	Weapon.SetRelativeRotation(Weapon.Default.RelativeRotation);
	if ( OldWeapon == Weapon )
	{
		if ( Weapon.IsInState('DownWeapon') ) 
			Weapon.BringUp();
		Inventory.OwnerEvent('ChangedWeapon'); // tell inventory that weapon changed (in case any effect was being applied)
		return;
	}
	else if ( Level.Game != None )
		MakeNoise(0.1 * Level.Game.GameDifficulty);		
	Inventory.OwnerEvent('ChangedWeapon'); // tell inventory that weapon changed (in case any effect was being applied)

	PlayWeaponSwitch(W);
	Weapon.BringUp(OldWeapon);
}

function NotifyAmmoChanged()
{
	if( Controller? )
		Controller.NotifyAmmoChanged();
}

//==============
// Encroachment
event bool EncroachingOn( actor Other )
{
	if ( Other.bWorldGeometry )
		return true;
		
	if ( ((Controller == None) || !Controller.bIsPlayer || bWarping) && (Pawn(Other) != None) )
		return true;
		
	return false;
}

event EncroachedBy( actor Other )
{
	//local vector MomentumTransfer;
	//local Pawn Instigator;
	//local int ImpactSpeed;

	if ( (Pawn(Other) != None) )
	{
		if ( KVehicle(Other) != None && Health > 0 && Role == ROLE_Authority)
		{
			KVehicle(Other).Smack( Self );
			/*
			Instigator = Pawn(Other);
			if (KVehicle(Other).PassengerPawns[0] != None)
				Instigator = KVehicle(Other).PassengerPawns[0];
			ImpactSpeed = clamp(VSize(Pawn(Other).velocity),400,1200);
			MomentumTransfer = normal(Self.location - other.location);
			MomentumTransfer *= (ImpactSpeed * 80);
			MomentumTransfer.Z+=32000;
			
			if ( (Other.GetTeam() == Self.GetTeam()) || Other.GetTeam() == 255 )
			{
				//Hit by Neutral or same-team vehicle
				Self.TakeDamage(
					((VSize(Pawn(Other).velocity) / 10 )* KVehicle(Other).SameTeamCollisionDamageMultiplier)
					, Instigator
					, Location
					, MomentumTransfer
					, class'Crushed');
			}
			else
			{
				//Hit by vehicle controlled by other team
				Self.TakeDamage(
					((VSize(Pawn(Other).velocity) / 10 ) * KVehicle(Other).CollisionDamageMultiplier)
					, Instigator
					, Location
					, MomentumTransfer
					, class'Crushed');
			}
*/
		} else if (Health > 0 && Role == ROLE_Authority)
		gibbedBy(Other);
	}
}

function gibbedBy(actor Other)
{
	if ( Role < ROLE_Authority )
		return;
	if ( Pawn(Other) != None )
		Died(Pawn(Other).Controller, class'DamTypeTelefragged', Location);
	else
		Died(None, class'Gibbed', Location);
}

//NEW (mdf) U2 base handling approach
/*OLD
//Base change - if new base is pawn or decoration, damage based on relative mass and old velocity
// Also, non-players will jump off pawns immediately
function JumpOffPawn()
{
	Velocity += (100 + CollisionRadius) * VRand();
	Velocity.Z = 200 + CollisionHeight;
	SetPhysics(PHYS_Falling);
	bNoJumpAdjust = true;
	Controller.SetFall();
}
*/

singular event BaseChange()
{
	local float decorMass;
	
	if ( bInterpolating )
		return;
		
	if ( (base == None) && (Physics == PHYS_None) )
	{
		SetPhysics(PHYS_Falling);
	}
//NEW (mdf) U2 base handling approach
	else if ( Base != None && !Base.bCanBeBaseForPawns )
	{	
		Base.LaunchPawnDamage( Self );
		if( Base != None ) //NOTE: Base could be destroyed in LaunchOffPawnDamage so check for None here
			Base.LaunchPawn( Self );
	}
/*OLD
	// Pawns can only set base to non-pawns, or pawns which specifically allow it.
	// Otherwise we do some damage and jump off.
	else if ( Pawn(Base) != None )
	{	
		if ( !Pawn(Base).bCanBeBaseForPawns )
		{
			Base.TakeDamage( (1-Velocity.Z/400)* Mass/Base.Mass, Self,Location,0.5 * Velocity , class'Crushed');
			JumpOffPawn();
		}
	}
*/
	else if ( (Decoration(Base) != None) && (Velocity.Z < -400) )
	{
		decorMass = FMax(Decoration(Base).Mass, 1);
		Base.TakeDamage((-2* Mass/decorMass * Velocity.Z/400), Self, Location, 0.5 * Velocity, class'Crushed');
	}
}

event UpdateEyeHeight( float DeltaTime )
{
	local float smooth, MaxEyeHeight;
	local float OldEyeHeight;
	local Actor HitActor;
	local vector HitLocation,HitNormal;

	if (Controller == None )
	{
		EyeHeight = 0;
		return;
	}
	if ( bTearOff )
	{
		EyeHeight = 0;
		bUpdateEyeHeight = false;
		return;
	}
	HitActor = trace(HitLocation,HitNormal,Location + (CollisionHeight + MAXSTEPHEIGHT + 14) * vect(0,0,1),
					Location + CollisionHeight * vect(0,0,1),true);
	if ( HitActor == None )
		MaxEyeHeight = CollisionHeight + MAXSTEPHEIGHT;
	else
		MaxEyeHeight = HitLocation.Z - Location.Z - 14;

	// smooth up/down stairs
	smooth = FMin(1.0, 10.0 * DeltaTime/Level.TimeDilation);
	If( Controller.WantsSmoothedView() )
	{
		OldEyeHeight = EyeHeight;
		EyeHeight = FClamp((EyeHeight - Location.Z + OldZ) * (1 - smooth) + BaseEyeHeight * smooth,
							-0.5 * CollisionHeight, MaxEyeheight);
	}
	else
	{
		bJustLanded = false;
		EyeHeight = FMin(EyeHeight * ( 1 - smooth) + BaseEyeHeight * smooth, MaxEyeHeight);
	}
	Controller.AdjustView(DeltaTime);
}

/* EyePosition()
Called by PlayerController to determine camera position in first person view.  Returns
the offset from the Pawn's location at which to place the camera
*/
simulated function vector EyePosition()
{
	return EyeHeight * vect(0,0,1) + WalkBob;
}

//=============================================================================

simulated event Destroyed()
{
	RemoveAllTimers();
	RemovePawn(); // NEW (mdf)
	DestroyShadow();

	if ( Controller != None )
		Controller.PawnDied(self);
	if ( Level.NetMode == NM_Client )
		return;

	while ( Inventory != None )
		Inventory.Destroy();

	Weapon = None;
	Super.Destroyed();
}

//=============================================================================
//
// Called immediately before gameplay begins.
//
event PreBeginPlay()
{
	Super.PreBeginPlay();
	
	AddPawn(); //NEW (mdf)
	
	Instigator = self;
	DesiredRotation = Rotation;
	if ( bDeleteMe )
		return;

	if ( BaseEyeHeight == 0 )
		BaseEyeHeight = 0.8 * CollisionHeight;
	EyeHeight = BaseEyeHeight;

	if ( menuname == "" )
		menuname = GetItemName(string(class));
}

simulated function CreateShadow()
{
	if (bPendingDelete || Level.NetMode==NM_DedicatedServer)
		return;
    if (bActorShadows && !Shadow && GetClient().PlayerShadows)
    {
        Shadow = Spawn(class'ShadowProjector',Self,'',Location);
        Shadow.ShadowActor = self;
        Shadow.LightDirection = Normal(vect(1,1,3));
        Shadow.LightDistance = 320;
        Shadow.MaxTraceDistance = 350;
        Shadow.InitShadow();
    }
}

simulated function DestroyShadow()
{
	if (Shadow?)
	{
		Shadow.Destroy();
		Shadow = None;
	}
}

simulated event PostBeginPlay()
{
	local AIScript A;

	Super.PostBeginPlay();

	CreateShadow();

	SplashTime = 0;
	SpawnTime = Level.TimeSeconds;
	EyeHeight = BaseEyeHeight;
	OldRotYaw = Rotation.Yaw;

	// automatically add controller to pawns which were placed in level
	// NOTE: pawns spawned during gameplay are not automatically possessed by a controller
	if( (Level.bStartup || (ControllerClass != None && ControllerClass.default.bShouldPossess)) && Health > 0 )
	{
		// check if I have an AI Script
		if ( AIScriptTag != '' )
		{
			ForEach AllActors(class'AIScript',A,AIScriptTag)
				break;
			// let the AIScript spawn and init my controller
			if ( A != None )
			{
				A.SpawnControllerFor(self);
				if ( Controller != None )
					return;
			}
		}
		if ( (ControllerClass != None) && (Controller == None) )
			Controller = spawn(ControllerClass);
		if ( Controller != None )
		{
			Controller.Possess(self);
			AIController(Controller).Skill += SkillModifier;
		}
	}
}

// called after PostBeginPlay on net client
simulated event PostNetBeginPlay()
{
	if ( Level.bDropDetail || (Level.DetailMode == DM_Low) )
		MaxLights = Min(4,MaxLights);
	if ( Role == ROLE_Authority )
		return;
	if ( Controller != None )
	{
		Controller.Pawn = self;
		if ( (PlayerController(Controller) != None)
			&& (PlayerController(Controller).ViewTarget == Controller) )
			PlayerController(Controller).SetViewTarget(self);
	} 

	if ( Role == ROLE_AutonomousProxy )
		bUpdateEyeHeight = true;

	if ( (PlayerReplicationInfo != None) 
		&& (PlayerReplicationInfo.Owner == None) )
		PlayerReplicationInfo.SetOwner(Controller);
	PlayWaiting();
}

simulated function SetMesh()
{
    if (Mesh != None)
        return;

	LinkMesh(default.Mesh);
}

function Gasp();
function SetMovementPhysics();

function bool IsRealPlayer()
{
	return Controller != None && PlayerController(Controller) != None;
}

static native final function bool ValidPawn( Pawn P );

function CalcHitIndication( int Damage, vector HitLocation, vector Momentum )
{
	local vector HitDir;
	local float Angle;

	// HitIndicators.
	if( Damage > 0 && IsRealPlayer() )
	{
		//PlayerController(Controller).ClientSendEvent(0);	//"HealthFlash"

		// Calculate hit angle.
		if(Momentum?)	HitDir = normal(-Momentum);
		else			HitDir = normal(HitLocation - Location);
		if(!HitDir)		HitDir = VRand();
		HitDir = HitDir << Rotation;	// transform to playerspace.
		Angle = atan( HitDir.y, HitDir.x );
		Angle *= 180.0 / PI;
		Angle -= 90.0;
		Angle = Angle * 256.0 / 360.0;	// scale for replication.
		DamageIndicatorAngle = Angle;

		//PlayerController(Controller).ClientSendEvent(1);	//"PainFlash"
		PlayerController(Controller).ClientSendEvent(0);	//"PainFlash" && "HealthFlash"
	}
}

function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	local int actualDamage;
	local bool bAlreadyDead;
	local Controller Killer;

	if ( damagetype == None )
	{
		if ( InstigatedBy != None )
		warn("No damagetype for damage by "$instigatedby$" with weapon "$InstigatedBy.Weapon);
		DamageType = class'DamageType';
	}
	
	if ( Role < ROLE_Authority )
	{
		log(self$" client damage type "$damageType$" by "$instigatedBy);
		return;
	}

	bAlreadyDead = (Health <= 0);

	if (Physics == PHYS_None)
		SetMovementPhysics();
	if (Physics == PHYS_Walking)
		momentum.Z = FMax(momentum.Z, 0.4 * VSize(momentum));
	if ( (instigatedBy == self) 
		|| ((Controller != None) && (InstigatedBy != None) && (InstigatedBy.Controller != None) && InstigatedBy.Controller.SameTeamAs(Controller)) )
		momentum *= 0.6;
	momentum = momentum/Mass;

	if( bInvulnerableBody )
		return;

	actualDamage = Level.Game.ReduceDamage(Damage, self, instigatedBy, HitLocation, Momentum, DamageType);

	Health -= actualDamage;
	if ( HitLocation == vect(0,0,0) )
		HitLocation = Location;
	if ( bAlreadyDead )
	{
		Warn(self$" took regular damage "$damagetype$" from "$instigatedby$" while already dead at "$Level.TimeSeconds);
		ChunkUp(Rotation, DamageType);
		return;
	}

	PlayHit(actualDamage,InstigatedBy, hitLocation, damageType, Momentum);
	if ( Health <= 0 )
	{
		// pawn died
		if ( instigatedBy != None )
			Killer = instigatedBy.GetKillerController();
		if ( bPhysicsAnimUpdate )
			TearOffMomentum = momentum;
		Died(Killer, damageType, HitLocation);
	}
	else
	{
		if ( (InstigatedBy != None) && (InstigatedBy != self) && (Controller != None)
			&& (InstigatedBy.Controller != None) && InstigatedBy.Controller.SameTeamAs(Controller) ) 
			Momentum *= 0.5;
			
		AddVelocity( momentum ); 
		if ( Controller != None )
			Controller.NotifyTakeHit(instigatedBy, HitLocation, actualDamage, DamageType, Momentum);
	}
	MakeNoise(1.0); 
}

simulated function int GetTeam()
{
	if( Controller? )
		return Controller.GetTeam();
	else
	{
		if ( PlayerReplicationInfo != None && PlayerReplicationInfo.Team != None )
			return PlayerReplicationInfo.Team.TeamIndex;
		else
			return TeamIndex;
	}
}

simulated function SetTeam(int NewTeam)
{
	if( Controller? )
		Controller.SetTeam( NewTeam );
	else if ( (PlayerReplicationInfo != None) && (PlayerReplicationInfo.Team != None) )
		PlayerReplicationInfo.Team.TeamIndex = NewTeam;
	else
		TeamIndex = NewTeam;
}

simulated function bool SameTeam( Pawn Other )
{
	return GetTeam() == Other.GetTeam();
}

function Controller GetKillerController()
{
	return Controller;
}

function Died(Controller Killer, class<DamageType> damageType, vector HitLocation)
{
    local Vector TossVel;

	if ( bDeleteMe || Level.bLevelChange )
		return; // already destroyed, or level is being cleaned up

	// mutator hook to prevent deaths
	// WARNING - don't prevent bot suicides - they suicide when really needed
	if ( Level.Game.PreventDeath(self, Killer, damageType, HitLocation) )
	{
		Health = max(Health, 1); //mutator should set this higher
		return;
	}
	Health = Min(0, Health);

    if (Weapon != None && Weapon.bCanThrow && !CanBeRevived() )
    {
		if ( Controller != None )
			Controller.LastPawnWeapon = Weapon.Class;
//!!MERGE        Weapon.HolderDied();
        TossVel = Vector(GetViewRotation());
        TossVel = TossVel * ((Velocity Dot TossVel) + 500) + Vect(0,0,200);
        TossWeapon(TossVel);
    }

	if ( Controller != None ) 
	{   
		Controller.WasKilledBy(Killer);
		Level.Game.Killed(Killer, Controller, self, damageType);
	}
	else
		Level.Game.Killed(Killer, Controller(Owner), self, damageType);

	TossArtifacts();

	if ( Killer != None )
		TriggerEvent(Event, self, Killer.Pawn);
	else
		TriggerEvent(Event, self, None);

	Velocity.Z *= 1.3;
	if ( IsHumanControlled() )
		PlayerController(Controller).ForceDeathUpdate();
	// Disable until gibs are implemented
    //if ( (DamageType != None) && DamageType.default.bAlwaysGibs )
	//	ChunkUp( Rotation, DamageType );
	//else
	//{
		PlayDying(DamageType, HitLocation);
		if ( Level.Game.bGameEnded )
			return;
	//!!ARL Commented out for now to allow Pawn to enter Dying state on clients.
	//	if ( !bPhysicsAnimUpdate && !IsLocallyControlled() )
			ClientDying(DamageType, HitLocation);
	// Disable until gibs are implemented
	//}
}

simulated function HandleDying()
{
	GotoState('Dying');

	if( Controller != None )
		Controller.HandleDying();
}

simulated function HandleRevived();

function NotifyUse( actor Other );
function NotifyUnuse( actor Other );

function TossArtifacts();

function bool Gibbed(class<DamageType> damageType)
{
	if ( damageType.default.GibModifier == 0 )
		return false; 
	if ( damageType.default.GibModifier >= 100 )
		return true;	
	if ( (Health < -80) || ((Health < -40) && (FRand() < 0.6)) )
		return true;
	return false;
}

event Falling()
{
	//SetPhysics(PHYS_Falling); //Note - physics changes type to PHYS_Falling by default
	if ( Controller != None )
		Controller.SetFall();
}

event HitWall(vector HitNormal, actor Wall);

event Landed(vector HitNormal)
{
	LandBob = FMin(50, 0.055 * Velocity.Z); 
	TakeFallingDamage();
	if ( Health > 0 )
		PlayLanded(Velocity.Z);
	bJustLanded = true;
}

event HeadVolumeChange(PhysicsVolume newHeadVolume)
{
	if ( (Level.NetMode == NM_Client) || (Controller == None) )
		return;
	if ( HeadVolume.bWaterVolume )
	{
		if (!newHeadVolume.bWaterVolume)
		{
			if ( Controller.bIsPlayer && (BreathTime > 0) && (BreathTime < 8) )
				Gasp();
			BreathTime = -1.0;
		}
	}
	else if ( newHeadVolume.bWaterVolume )
		BreathTime = UnderWaterTime;
}

function bool TouchingWaterVolume()
{
	local PhysicsVolume V;

	ForEach TouchingActors(class'PhysicsVolume',V)
		if ( V.bWaterVolume )
			return true;
			
	return false;
}

//Pain timer just expired.
//Check what zone I'm in (and which parts are)
//based on that cause damage, and reset BreathTime

function bool IsInPain()
{
	local PhysicsVolume V;

	ForEach TouchingActors(class'PhysicsVolume',V)
		if ( V.bPainCausing && (V.DamageType != ReducedDamageType) 
			&& (V.DamagePerSec > 0) )
			return true;
	return false;
}
	
event BreathTimer()
{
	if ( (Health < 0) || (Level.NetMode == NM_Client) )
		return;
	TakeDrowningDamage();
	if ( Health > 0 )
		BreathTime = 2.0;
}

function TakeDrowningDamage();		

function bool CheckWaterJump(out vector WallNormal)
{
	local actor HitActor;
	local vector HitLocation, HitNormal, checkpoint, start, checkNorm, Extent;

	checkpoint = vector(Rotation);
	checkpoint.Z = 0.0;
	checkNorm = Normal(checkpoint);
	checkPoint = Location + CollisionRadius * checkNorm;
	Extent = CollisionRadius * vect(1,1,0);
	Extent.Z = CollisionHeight;
	HitActor = Trace(HitLocation, HitNormal, checkpoint, Location, true, Extent);
	if ( (HitActor != None) && (Pawn(HitActor) == None) )
	{
		WallNormal = -1 * HitNormal;
		start = Location;
		start.Z += 1.1 * MAXSTEPHEIGHT;
		checkPoint = start + 2 * CollisionRadius * checkNorm;
		HitActor = Trace(HitLocation, HitNormal, checkpoint, start, true);
		if (HitActor == None)
			return true;
	}

	return false;
}

function DoDoubleJump( bool bUpdating );
function bool CanDoubleJump();

function UpdateRocketAcceleration(float DeltaTime, float YawChange, float PitchChange);

//Player Jumped
function bool DoJump( bool bUpdating )
{
	if ( !bIsCrouched && !bWantsToCrouch && ((Physics == PHYS_Walking) || (Physics == PHYS_Ladder) || (Physics == PHYS_Spider)) )
	{
		if ( Role == ROLE_Authority )
		{
			if ( (Level.Game != None) && (Level.Game.GameDifficulty > 2) )
				MakeNoise(0.1 * Level.Game.GameDifficulty);
			if ( bCountJumps && (Inventory != None) )
				Inventory.OwnerEvent('Jumped');
		}
		if ( Physics == PHYS_Spider )
			Velocity = JumpZ * Floor;
		else if ( Physics == PHYS_Ladder )
			Velocity.Z = 0;
		else if ( bIsWalking )
			Velocity.Z = Default.JumpZ;
		else
			Velocity.Z = JumpZ;
		if ( (Base != None) && !Base.bWorldGeometry )
			Velocity.Z += Base.Velocity.Z; 
		SetPhysics(PHYS_Falling);
        return true;
	}
    return false;
}

/* PlayMoverHitSound()
Mover Hit me, play appropriate sound if any
*/
function PlayMoverHitSound();

function PlayDyingSound();

function PlayHit(float Damage, Pawn InstigatedBy, vector HitLocation, class<DamageType> damageType, vector Momentum)
{
	local vector BloodOffset, Mo, HitNormal;
	local class<Effects> DesiredEffect;
/*!!MERGE
	local class<Emitter> DesiredEmitter;
*/
	local PlayerController PC;

	if ( (Damage <= 0) && ((Controller == None) || !Controller.bGodMode) )
		return;
		
	if (Damage > DamageType.Default.DamageThreshold) //spawn some blood
	{

		HitNormal = Normal(HitLocation - Location);
	
		// Play any set effect
		if ( EffectIsRelevant(Location,true) )
		{	
		DesiredEffect = DamageType.static.GetPawnDamageEffect(HitLocation, Damage, Momentum, self, (Level.bDropDetail || Level.DetailMode == DM_Low));

		if ( DesiredEffect != None )
		{
			BloodOffset = 0.2 * CollisionRadius * HitNormal;
			BloodOffset.Z = BloodOffset.Z * 0.5;

			Mo = Momentum;
			if ( Mo.Z > 0 )
				Mo.Z *= 0.5;
			spawn(DesiredEffect,self,,HitLocation + BloodOffset, rotator(Mo));
		}
/*!!MERGE
		// Spawn any preset emitter
		DesiredEmitter = DamageType.Static.GetPawnDamageEmitter(HitLocation, Damage, Momentum, self, (Level.bDropDetail || Level.DetailMode == DM_Low)); 		
		if (DesiredEmitter != None)
			spawn(DesiredEmitter,,,HitLocation+HitNormal, Rotator(HitNormal));
*/
		}		
	}
	if ( Health <= 0 )
	{
		if ( PhysicsVolume.bDestructive && (PhysicsVolume.ExitActor != None) )
			Spawn(PhysicsVolume.ExitActor);
		return;
	}
	
	// jdf ---
	if ( (Level.NetMode != NM_DedicatedServer) && (Level.NetMode != NM_ListenServer) )
	{
		PC = PlayerController(Controller);
		if ( PC != None && PC.bEnableDamageForceFeedback )
			PC.ClientPlayForceFeedback("Damage");
	}
	// --- jdf
	
	if ( Level.TimeSeconds - LastPainTime > 0.1 )
	{
		PlayTakeHit(HitLocation,Damage,damageType);
		LastPainTime = Level.TimeSeconds;
	}
}

function bool CanBeRevived( optional Actor Other ) { return false; }
function bool CanBeMercyKilled( optional Actor Other ) { return false; }

/* 
Pawn was killed - detach any controller, and die
*/

// blow up into little pieces (implemented in subclass)		

simulated function ChunkUp( Rotator HitRotation, class<DamageType> D ) 
{
	if ( (Level.NetMode != NM_Client) && (Controller != None) )
	{
		if ( Controller.bIsPlayer )
			Controller.PawnDied(self);
		else
			Controller.Destroy();
	}
	destroy();
}

simulated function DestroyTimer()
{
	if ( !PlayerCanSeeMe() )
		Destroy();
	else
		AddTimer( 'DestroyTimer', 2.0 );
}

State Dying
{
ignores Trigger, Bump, HitWall, HeadVolumeChange, PhysicsVolumeChange, Falling, BreathTimer, ChangedWeapon;

	event ChangeAnimation() {}
	event StopPlayFiring() {}
	function PlayFiring(float Rate, name FiringMode) {}
	function PlayWeaponSwitch(Weapon NewWeapon) {}
	function PlayTakeHit(vector HitLoc, int Damage, class<DamageType> damageType) {}
	simulated function PlayNextAnimation() {}

	function Died(Controller Killer, class<DamageType> damageType, vector HitLocation)
	{
	}

	event FellOutOfWorld(eKillZType KillType)
	{
		if(KillType == KILLZ_Lava || KillType == KILLZ_Suicide )
			return;

		Destroy();
	}

	function Landed(vector HitNormal)
	{
		local rotator finalRot;

		LandBob = FMin(50, 0.055 * Velocity.Z); 
		if( Velocity.Z < -500 )
			TakeDamage( (1-Velocity.Z/30),Instigator,Location,vect(0,0,0) , class'Crushed');

		finalRot = Rotation;
		finalRot.Roll = 0;
		finalRot.Pitch = 0;
		setRotation(finalRot);
		SetPhysics(PHYS_None);
		SetCollision(true, false, false);
		KSetBlockKarma(false);

		if ( !IsAnimating(0) )
			LieStill();
	}

	/* ReduceCylinder() made obsolete by ragdoll deaths */
	function ReduceCylinder()
	{
		SetCollision(false, false, false);
		KSetBlockKarma(false);
	}

	function LandThump()
	{
		// animation notify - play sound if actually landed, and animation also shows it
		if ( Physics == PHYS_None)
			bThumped = true;
	}

	event AnimEnd(int Channel)
	{
		if ( Channel != 0 )
			return;
		if ( Physics == PHYS_None )
			LieStill();
		else if ( PhysicsVolume.bWaterVolume )
		{
			bThumped = true;
			LieStill();
		}
	}

	function LieStill()
	{
		if ( !bThumped )
			LandThump();
		SetCollision(false, false, false);
		KSetBlockKarma(false);
	}

	singular function BaseChange()
	{
		if( base == None )
			SetPhysics(PHYS_Falling);
		else if ( Pawn(base) != None ) // don't let corpse ride around on someone's head
        	ChunkUp( Rotation, class'Fell' ); 
	}

	function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, 
							Vector momentum, class<DamageType> damageType)
	{
		SetPhysics(PHYS_Falling);
		if ( (Physics == PHYS_None) && (Momentum.Z < 0) )
			Momentum.Z *= -1;
		Velocity += 3 * momentum/(Mass + 200);
		if ( bInvulnerableBody )
			return;
		Damage *= DamageType.Default.GibModifier;
		Health -=Damage;
		if ( ((Damage > 30) || !IsAnimating()) && (Health < -80) )
        	ChunkUp( Rotation, DamageType ); 
	}

	simulated function BeginState()
	{
		if ( (LastStartSpot != None) && (LastStartTime - Level.TimeSeconds < 6) )
			LastStartSpot.LastSpawnCampTime = Level.TimeSeconds;
		if ( bTearOff && (Level.NetMode == NM_DedicatedServer) )
			LifeSpan = 1.0;
		else
			AddTimer( 'DestroyTimer', 12.0 );
		SetPhysics(PHYS_Falling);
		bInvulnerableBody = true;
		bUseEnabled = false;
		if ( Controller != None )
		{
			if ( Controller.bIsPlayer )
				Controller.PawnDied(self);
			else
				Controller.Destroy();
		}
	}

	event EndState()
	{
		DeathAnim = '';
		RemoveTimer( 'DestroyTimer' );		// just in case
	}

Begin:
	Sleep(0.15);
	bInvulnerableBody = false;
	PlayDyingSound();
}

state Passenger
{
ignores ChangedWeapon;

	event BeginState()
	{
		// This should get done by Weapon.PutDown in PlayerWalking.EndState,
		// but at that point on the client, the Pawn is already set to the vehicle.
		// The vehicle should probably handle this if we ever do vehicle specific crosshairs.

		//PWC 2003.05.05 removed HideCrosshair since ClientKDriverEnter in KVehicle is doing a weapon.putdown()
/*		if( IsLocallyControlled() )
			PlayerController(Controller).SendEvent("HideCrosshair");
*/
	}
}

//=============================================================================
// Animation interface for controllers

simulated event SetAnimAction(name NewAction);

/* PlayXXX() function called by controller to play transient animation actions 
*/
simulated event PlayDying(class<DamageType> DamageType, vector HitLoc)
{
	AmbientSound = None;
	HandleDying();
	if ( bPhysicsAnimUpdate )
	{
		bReplicateMovement = false;
		if( !CanBeRevived() )
		{
			bTearOff = true;
			Velocity += TearOffMomentum;
		}
		SetPhysics(PHYS_Falling);
	}
}

simulated function PlayFiring(float Rate, name FiringMode);
function PlayWeaponSwitch(Weapon NewWeapon);
simulated event StopPlayFiring()
{
	bSteadyFiring = false;
}

function PlayTakeHit(vector HitLoc, int Damage, class<DamageType> damageType)
{
	local Sound DesiredSound;

	if (Damage==0)
		return;
	// 		
	// Play a hit sound according to the DamageType

 	DesiredSound = DamageType.Static.GetPawnDamageSound();
	if (DesiredSound != None)
		PlayOwnedSound(DesiredSound,SLOT_Misc);
}

//=============================================================================
// Pawn internal animation functions

simulated event ChangeAnimation()
{
	if ( (Controller != None) && Controller.bControlAnimations )
		return;
	// player animation - set up new idle and moving animations
	PlayWaiting();
	PlayMoving();
}

simulated event AnimEnd(int Channel)
{
	if ( Channel == 0 )
		PlayWaiting();
}

// Animation group checks (usually implemented in subclass)

function bool CannotJumpNow()
{
	return false;
}

//!!mdf-tbd: remove all of these (not used)?
simulated event PlayJump();
simulated event PlayFalling();
simulated function PlayMoving();
simulated function PlayWaiting();

//!!mdf-tbd: remove (not used)?
function PlayLanded(float impactVel)
{
	if ( !bPhysicsAnimUpdate )
		PlayLandingAnimation(impactvel);
}

//!!mdf-tbd: remove all of these (not used)?
simulated event PlayLandingAnimation(float ImpactVel);

function PlayVictoryAnimation();

function HoldCarriedObject(CarriedObject O, name AttachmentBone);

defaultproperties
{
	bCanBeDamaged=true
	 bUseEnabled=true
	 bNoRepMesh=true
	 bJumpCapable=true
 	 bCanJump=true
	 bCanWalk=true
	 bCanSwim=false
	 bCanFly=false
	 bCanPickupInventory=true
	 bUpdateSimulatedPosition=true
	 BaseEyeHeight=54.0 //OLD (mdf): +00064.000000
     EyeHeight=+00054.000000
	 CollisionRadius=28.000 //OLD (mdf): +00034.000000
     CollisionHeight=58.000 //OLD (mdf): +00078.000000
     GroundSpeed=+00600.000000
     AirSpeed=+00600.000000
     WaterSpeed=+00300.000000
     AccelRate=+02048.000000
     JumpZ=450.000 //OLD (mdf): +00420.000000
	 MaxFallSpeed=+1200.0
	 DrawType=DT_Mesh
	 bLOSHearing=true
	 HearingThreshold=+2800.0
     Health=100
     Visibility=128
	 LadderSpeed=+200.0
     noise1time=-00010.000000
     noise2time=-00010.000000
     AvgPhysicsTime=+00000.100000
     SoundDampening=+00001.000000
     DamageScaling=+00001.000000
     bDirectional=True
     bCanTeleport=True
	 bStasis=True
     SoundRadius=60
	 SoundVolume=255
     bCollideActors=True
     bCollideWorld=True
     bBlockActors=True
     bBlockPlayers=True
     bProjTarget=True
     bRotateToDesired=True
 	 bCanCrouch=True
     RotationRate=(Pitch=4096,Yaw=20000,Roll=3072)
	 Texture=S_Pawn
     RemoteRole=ROLE_SimulatedProxy
     NetPriority=+00002.000000
	 AirControl=+0.05
	 ControllerClass=class'AIController'
	 CrouchHeight=42.0 //OLD (mdf): +40.0
	 CrouchRadius=28.0 //OLD (mdf): +34.0
     MaxDesiredSpeed=+00001.000000
     DesiredSpeed=+00001.000000
 	 LandMovementState=PlayerWalking
	 WaterMovementState=PlayerSwimming
	 SightRadius=+05000.000000
	 bOwnerNoSee=true
	 bAcceptsProjectors=True
	 BlendChangeTime=0.25
	 WalkingPct=+0.5
	 CrouchedPct=+0.5
	 bTravel=true
	 BaseMovementRate=+525.0
	 ForwardStrafeBias=+0.0
	 BackwardStrafeBias=+0.0
	 bShouldBaseAtStartup=true
     Bob=0.0085
	 bDisturbFluidSurface=true
	 bBlockKarma=False
	 bWeaponBob=true
	 bUseCompressedPosition=true
	 HeadScale=+1.0
	 TeamIndex=255
	 bCanBeBaseForPawns=false
}
