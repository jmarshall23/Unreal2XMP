//=============================================================================
// U2Pawn.uc
//=============================================================================

class U2Pawn extends Pawn
	native;

#exec OBJ LOAD FILE=XMPPlayer.uax

//-----------------------------------------------------------------------------

const MAXJUMPHEIGHT = 94.0;

enum ESurfaceType
{
    SFT_None,
    SFT_Default,
    SFT_Terrain,
    SFT_Water,
};

//-----------------------------------------------------------------------------

var() float StaminaMax;				// maximum stamina possible (fully charged)
var() float StaminaRechargeRate;	// how fast the stamina is regained
var   float Stamina;				// current amount of stamina used for sprinting, etc.

//-----------------------------------------------------------------------------
// Sprinting

var() bool  bCanSprint;				// does pawn have the ability to run fast?
var() float SprintRatio;			// how much faster sprinting speed is over the ground speed
var() float SprintCost;				// amount of stamina sprinting consumes
var   bool  bSprinting;				// whether pawn is currently sprinting

//-----------------------------------------------------------------------------
// Jump jets

var() bool  bCanDoubleJump;			// does the player have jump jets?
var() float JumpJetZ;				// speed increase in Z direction when deploying jump jets
var() float JumpJetCost;			// resources need to use the jump jets
var() float JumpJetDuration;		// length of time (seconds) across which to provide the speed
var() Sound JumpJetThrustSound;		// sound to use while jump jets are active
var   bool  bDoubleJumping;			// is the player in the middle of a double jump?
var   float DoubleJumpStartSecs;	// time double jump started

var() float SkillMax;				// max skill ability for using xmp items, hacking, etc.
var() float SkillRechargeRate;		// speed at which skill level is regained
var() float SkillTeamEnergyCost;	// how much team energy is needed to replenish skill
var   float Skill;					// current amount of skill available

//-----------------------------------------------------------------------------
// Mantling

// !!mdf: turn most of these into constants once good values found?
var(Pawn) float MantleHandsUpwardReach;				// vertical reach of hands above CC
var(Pawn) float MantleHandsForwardReachMin;			// min horizontal reach of hands in front of CC
var(Pawn) float MantleHandsForwardReachMax;			// max horizontal reach of hands in front of CC
var(Pawn) float MantleMinHitNormalDeltaZ;			// abort mantle if ledge normal Z component is less than this (.707 ~= 45 degrees)
var(Pawn) float MantleSpeed;						// speed at which pawn mantles up (replaces AirSpeed during mantling)
var(Pawn) float MantleMaxStandingMantleHeight;		// max height which can be mantled while standing
var(Pawn) int   MantleDebugLevel;					// debugging control (set to 0: none, 1: show traces, 2: add begin/end 3: add other log info)
var(Pawn) Sound MantleSound;
var(Pawn) Sound MantleHurtSound;
var	      float MaxFallingMantleSpeed;				// can't do a falling mantle at speeds above this
var       float MaxRisingMantleSpeed;				// can't do a rising mantle at speeds above this

var vector MantleDestination;						// set when mantling begins to approx mantle destination for Pawn
var float NextCheckMantleTime;						// can be set to LevelTime + X to prevent re-mantling for a while
var float MantleHeight;								// set to height of ledge relative to player when mantling begins (for syncing animation)
var bool bMantling;									// true whenever pawn is mantling

var Actor UsingActor;

// character using
var bool bCanBeMercyKilled;						// can be FUBAR-ed by enemy
var int RevivedHealth;							// health wounded player has after you've revived him
var float ReviveCost;							// skill resources needed to revive a teammate
var float HackRate;								// hacking rate scaling factor
var float HackCost;								// skill need to hack per second

var string DefaultPowerSuitName;
var float PowerSuitPower;
var float PowerSuitMax;

var float FlashbangTimer;
var sound FlashbangHum;

var bool bLastDamageWasHeadshot;

var float MaxPlayerIDDistance;

var localized string ReviveMsg;
var float DeployProtectionTime;

//-----------------------------------------------------------------------------
// animations

var enum EFireAnimState
{
    FAS_None,
    FAS_PlayOnce,
    FAS_Looping,
    FAS_Ready
} FireState;

var eDoubleClickDir CurrentDir;
var name MantleAnim;

var(JumpJets) ParticleGenerator JumpJetEffect;
var ParticleGenerator JumpJetA,JumpJetB;
var byte JumpJetIndex,LastJumpJetIndex;

var(LatentDamage) ParticleRadiator FlameEffect,EMPEffect,ToxicEffect;
var ParticleRadiator FlameRadiator,EMPRadiator,ToxicRadiator;
var byte FlameIndex,LastFlameIndex;
var byte EMPIndex,LastEMPIndex;
var byte ToxicIndex,LastToxicIndex;

var config bool bPlayOwnFootsteps;
var(Sounds) array<Sound> SoundFootstepsDefault;
var(Sounds) array<Sound> SoundFootstepsTerrain;
var(Sounds) array<Sound> SoundFootstepsWater;
var(Sounds) array<Sound> SoundLandDefault;
var(Sounds) array<Sound> SoundLandTerrain;
var(Sounds) array<Sound> SoundLandWater;
var(Sounds) float FootstepVolume, FootstepPitch, LandVolume, LandPitch;
var(Sounds) float MovementRadius;
var(Sounds)	float MovementPitchVariancePercent;		// percentage to vary final pitch by +/-
var(Sounds)	float MovementVolumeVariancePercent;	// percentage to vary final volume by +/-
var(Sounds) float TimeBetweenFootSteps;				// for viewbob path

var float NextFootStepTime, LastLandTime;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && bNetOwner && Role==ROLE_Authority )
        Stamina, Skill, FlashbangTimer;
	reliable if( bNetDirty && Role==ROLE_Authority )
		PowerSuitPower, PowerSuitMax, bCanBeMercyKilled;
	unreliable if( bNetDirty && !bNetOwner && Role==ROLE_Authority )
		JumpJetIndex;
	unreliable if( bNetDirty && Role==ROLE_Authority )
		FlameIndex, EMPIndex, ToxicIndex;
}

//-----------------------------------------------------------------------------

simulated event PostBeginPlay()
{
    Super.PostBeginPlay();
    AssignInitialPose();
	Stamina = StaminaMax;
}

//-----------------------------------------------------------------------------

simulated function PostNetBeginPlay()
{
	Super.PostNetBeginPlay();
	LastJumpJetIndex = JumpJetIndex;
	LastFlameIndex = FlameIndex;
	LastEMPIndex = EMPIndex;
	LastToxicIndex = ToxicIndex;
}

//-----------------------------------------------------------------------------

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();

	if (!bPendingDelete && (Level.NetMode != NM_DedicatedServer))
	{
		if (JumpJetEffect?)
		{
			JumpJetA = class'ParticleGenerator'.static.CreateNew( self, JumpJetEffect, Location );
			AttachToBone(JumpJetA, 'JumpJetLeft');
			JumpJetA.bAttachDelayTick=false;
			JumpJetA.IdleTime=0.0;

			JumpJetB = class'ParticleGenerator'.static.CreateNew( self, JumpJetEffect, Location );
			AttachToBone(JumpJetB, 'JumpJetRight');
			JumpJetB.bAttachDelayTick=false;
			JumpJetB.IdleTime=0.0;
		}
		if (FlameEffect?)
		{
			FlameRadiator = ParticleRadiator(class'ParticleGenerator'.static.CreateNew( self, FlameEffect, Location ));
			FlameRadiator.SetBase(Self);
			FlameRadiator.MeshOwner=Self;
			FlameRadiator.bStopIfHidden=true;
			FlameRadiator.IdleTime=0.0;
		}
		if (EMPEffect?)
		{
			EMPRadiator = ParticleRadiator(class'ParticleGenerator'.static.CreateNew( self, EMPEffect, Location ));
			EMPRadiator.SetBase(Self);
			EMPRadiator.MeshOwner=Self;
			EMPRadiator.bStopIfHidden=true;
			EMPRadiator.IdleTime=0.0;
		}
		if (ToxicEffect?)
		{
			ToxicRadiator = ParticleRadiator(class'ParticleGenerator'.static.CreateNew( self, ToxicEffect, Location ));
			ToxicRadiator.SetBase(Self);
			ToxicRadiator.MeshOwner=Self;
			ToxicRadiator.bStopIfHidden=true;
			ToxicRadiator.IdleTime=0.0;
		}
	}
}

//-----------------------------------------------------------------------------

simulated function Destroyed()
{
	if (JumpJetA?)
	{
		JumpJetA.ParticleDestroy();
		JumpJetA = None;
	}
	if (JumpJetB?)
	{
		JumpJetB.ParticleDestroy();
		JumpJetB = None;
	}
	if (FlameRadiator?)
	{
		FlameRadiator.ParticleDestroy();
		FlameRadiator.MeshOwner=None;
		FlameRadiator = None;
	}
	if (EMPRadiator?)
	{
		EMPRadiator.ParticleDestroy();
		EMPRadiator.MeshOwner=None;
		EMPRadiator = None;
	}
	if (ToxicRadiator?)
	{
		ToxicRadiator.ParticleDestroy();
		ToxicRadiator.MeshOwner=None;
		ToxicRadiator = None;
	}
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

function PossessedBy( Controller C )
{
	Super.PossessedBy( C );
	//!!MERGE AssignDefaultInventory();
 	//!!MERGE SetDefaultWeapon();
	SetupSkillBar();
}

//-----------------------------------------------------------------------------

function SetupSkillBar()
{
	if( XMPGame(Level.Game)? && NumTimesSpawned() > 0 )
		Skill += XMPGame(Level.Game).DrainTeamEnergy( GetTeam(), SkillMax * SkillTeamEnergyCost, 'InitialSkillNeededForPlayerSpawn' );
	else
		Skill = SkillMax;
}

//-----------------------------------------------------------------------------

// Set up default blending parameters and pose. Ensures the mesh doesn't have only a T-pose whenever it first springs into view.
simulated function AssignInitialPose()
{
	if( Mesh != None && !bPlayedDeath && Health > 0 )
	{
		TweenAnim(MovementAnims[0],0.0);
		AnimBlendParams(1, 1.0, 0.2, 0.2, 'Bip01 Spine1');
		BoneRefresh();	
	}
}

//-----------------------------------------------------------------------------

function name GetWeaponBoneFor(Inventory I)
{
	if( I.IsA('Weapon') )
		return 'bone_weapon';
	if( I.IsA('Artifact') )
		return 'bone_artifact';
}

//-----------------------------------------------------------------------------

function int NumTimesSpawned()
{
	if( PlayerReplicationInfo? )
		return PlayerReplicationInfo.Deaths;
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Sprinting

function SetSprint( bool bEnabled )
{
	if( Health > 0 && bCanSprint && (bSprinting != bEnabled) )
	{
		bSprinting = bEnabled;
		if( bSprinting )
			GroundSpeed = default.GroundSpeed * SprintRatio;
		else
			GroundSpeed = default.GroundSpeed;
			
		TimeBetweenFootSteps = default.TimeBetweenFootSteps * default.GroundSpeed / GroundSpeed;
			
		ChangeAnimation();
	}
}

//-----------------------------------------------------------------------------

event Falling()
{
	bCanDoubleJump = true;
	Super.Falling();
}

//-----------------------------------------------------------------------------

simulated function MantlingBegin()
{
    local name Anim;

    Anim = MantleAnim;
    if( PlayAnim( Anim, 1.0, 0.1 ) )
        bWaitForAnim = true;
    AnimAction = Anim;
}

//-----------------------------------------------------------------------------
// Double jumping / Jump jets

function bool CanDoubleJump()
{
	if( Health <= 0 || Physics != PHYS_Falling || Skill < JumpJetCost )
		return false;
	
	//NOTE (mdf): this code runs on the client (despite lack of simulated) because class is ROLE_AutonomousProxy
	if( XMPGame(Level.Game) != None && XMPGame(Level.Game).JumpJetRechargeTime > 0.0 )
	{
		// testing: double jump re-enabled after X secs (longer delay in lower gravity)
		// (only works in singleplayer currently because clients don't have an XMPGame)
		return( (XMPGame(Level.Game).bJumpJetRechargesOnLanding && bCanDoubleJump) ||
				((Level.TimeSeconds - DoubleJumpStartSecs) > XMPGame(Level.Game).JumpJetRechargeTime * PhysicsVolume.default.Gravity.Z / PhysicsVolume.Gravity.Z) );
	}
	else
	{
		// double jump re-enabled upon landing
		return bCanDoubleJump;
	}
}

//-----------------------------------------------------------------------------

function DoDoubleJump( bool bUpdating )
{
	if( Health > 0 )
	{
		PlayDoubleJump();
		bDoubleJumping = true;
		bCanDoubleJump = false;
		DoubleJumpStartSecs = Level.TimeSeconds;
		if( !bUpdating )
			PlaySound( JumpJetThrustSound, SLOT_Pain );
		//NOTE (mdf) afaik we should already be in PHYS_Falling here?
		//SetPhysics( PHYS_Falling );
		UseSkill( JumpJetCost );
	}
}

//-----------------------------------------------------------------------------

function bool DoJump( bool bUpdating )
{
	if( Health > 0 )
	{
		if( !bUpdating && CanDoubleJump() )
		{
			if( PlayerController(Controller) != None )
				PlayerController(Controller).bDoubleJump = true;
			DoDoubleJump(bUpdating);
			return true;
		}

		if( Super.DoJump(bUpdating) )
		{
			//if( !bUpdating )
			//	PlayOwnedSound(GetSound(EST_Jump), SLOT_Pain, GruntVolume,,80);
			bCanDoubleJump = true;
			return true;
		}
	}
	
    return false;
}

//-----------------------------------------------------------------------------

function bool Dodge( eDoubleClickDir DoubleClickMove )
{
    local vector X,Y,Z, TraceStart, TraceEnd;
    local float VelocityZ;
    local name Anim;

    if ( Health <= 0 || bIsCrouched || bWantsToCrouch || (Physics != PHYS_Walking && Physics != PHYS_Falling) )
        return false;

    GetAxes(Rotation,X,Y,Z);

    if ( Physics == PHYS_Falling )
    {
        if (DoubleClickMove == DCLICK_Forward)
            TraceEnd = -X;
        else if (DoubleClickMove == DCLICK_Back)
            TraceEnd = X;
        else if (DoubleClickMove == DCLICK_Left)
            TraceEnd = Y;
        else if (DoubleClickMove == DCLICK_Right)
            TraceEnd = -Y;
        TraceStart = Location - CollisionHeight*Vect(0,0,1) + TraceEnd*CollisionRadius;
        TraceEnd = TraceStart + TraceEnd*32.0;
        if (FastTrace(TraceEnd, TraceStart))
        {
            return false;
        }
        else
        {
            if (DoubleClickMove == DCLICK_Forward)
                Anim = WallDodgeAnims[0];
            else if (DoubleClickMove == DCLICK_Back)
                Anim = WallDodgeAnims[1];
            else if (DoubleClickMove == DCLICK_Left)
                Anim = WallDodgeAnims[2];
            else if (DoubleClickMove == DCLICK_Right)
                Anim = WallDodgeAnims[3];

            if ( PlayAnim(Anim, 1.0, 0.1) )
            {
                bWaitForAnim = true;
	            AnimAction = Anim;
			}
			
            if (Velocity.Z < -DodgeSpeedZ*0.5) Velocity.Z += DodgeSpeedZ*0.5;
        }
    }

    VelocityZ = Velocity.Z;

    if (DoubleClickMove == DCLICK_Forward)
        Velocity = DodgeSpeedFactor*GroundSpeed*X + (Velocity Dot Y)*Y;
    else if (DoubleClickMove == DCLICK_Back)
        Velocity = -DodgeSpeedFactor*GroundSpeed*X + (Velocity Dot Y)*Y; 
    else if (DoubleClickMove == DCLICK_Left)
        Velocity = -DodgeSpeedFactor*GroundSpeed*Y + (Velocity Dot X)*X; 
    else if (DoubleClickMove == DCLICK_Right)
        Velocity = DodgeSpeedFactor*GroundSpeed*Y + (Velocity Dot X)*X; 
 
    Velocity.Z = VelocityZ + DodgeSpeedZ;
    CurrentDir = DoubleClickMove;
    SetPhysics(PHYS_Falling);
    //!!sounds PlayOwnedSound(GetSound(EST_Dodge), SLOT_Pain, GruntVolume,,80);
    return true;
}

//-----------------------------------------------------------------------------

function HandleUsing( Pawn Other, float DeltaTime );

//-----------------------------------------------------------------------------

function bool HasNeededSkill( float SkillNeeded )
{
	return( SkillNeeded<=0 || Skill >= SkillNeeded );
}

//-----------------------------------------------------------------------------

function UseSkill( float SkillUsed )
{
	Skill -= SkillUsed;
}

//-----------------------------------------------------------------------------

static function string GetQualifiedClassName( int Team )
{
	local int i, pos, numTeamColors;
	local string Result;

	Result = Super.GetQualifiedClassName( Team );

	// removed previous team color
	numTeamColors = ArrayCount(class'TeamInfo'.default.ColorNames);
	for( i=0; i < numTeamColors; i++ )
	{
		pos = InStr( Result, class'TeamInfo'.default.ColorNames[i] );		//!!FIXME -- use Split to search from the right
		if( pos != -1 ) Result = Left( Result, pos );
	}

	// add new team color
	if( Team >= 0 && Team < numTeamColors )
		Result = Result $ class'TeamInfo'.default.ColorNames[Team];

	return Result;
}

//-----------------------------------------------------------------------------

interface function bool FlashbangEnabled()	{ return FlashbangTimer > 0; }
interface function float FlashbangAlpha()	{ return FMin( 255.0, (FlashbangTimer / 4.0) * 255.0 ); }

//-----------------------------------------------------------------------------

simulated function Tick( float DeltaTime )
{
	local Controller UsingController;
	local U2Pawn UsingPawn;
	local float AddedSkill;

	Super.Tick( DeltaTime );

	if( JumpJetIndex != LastJumpJetIndex )
	{
		LastJumpJetIndex = JumpJetIndex;
		if (JumpJetA?) JumpJetA.Trigger(self,Instigator);
		if (JumpJetB?) JumpJetB.Trigger(self,Instigator);
	}

	if( FlameIndex != LastFlameIndex )
	{
		LastFlameIndex = FlameIndex;
		if (FlameRadiator?)
			FlameRadiator.Trigger(self,Instigator);
	}
	if( EMPIndex != LastEMPIndex )
	{
		LastEMPIndex = EMPIndex;
		if (EMPRadiator?)
			EMPRadiator.Trigger(self,Instigator);
	}
	if( ToxicIndex != LastToxicIndex )
	{
		LastToxicIndex = ToxicIndex;
		if (ToxicRadiator?)
			ToxicRadiator.Trigger(self,Instigator);
	}

	if( FlashbangTimer > 0 )
	{
		if( IsLocallyControlled() )
		{
			FlashbangTimer -= DeltaTime;
			AmbientSound = FlashbangHum;
			SoundVolume = FlashbangAlpha();
			if( FlashbangTimer < 0 )
			{
				FlashbangTimer = 0;
				AmbientSound = None;
				SoundVolume = 128;
			}
		}
	}

	if( Role < ROLE_Authority )
		return;

	if( bCanSprint )
	{
		if( bSprinting )
		{
			if( VSize2D(Velocity) > 0.0 )
				Stamina -= DeltaTime * SprintCost;
			if( Stamina < 0 )
				Stamina = 0;
		}
		else
		{
			if( Stamina < StaminaMax )
			{
				Stamina += StaminaRechargeRate * DeltaTime;
				if( Stamina > StaminaMax ) Stamina = StaminaMax;
			}
		}
	}

	if( ValidPawn(Self) && Skill < SkillMax )
	{
		AddedSkill = SkillRechargeRate * SkillTeamEnergyCost * DeltaTime;
		if( Skill + AddedSkill > SkillMax )
			AddedSkill = SkillMax - Skill;
		Skill += XMPGame(Level.Game).DrainTeamEnergy( GetTeam(), AddedSkill, 'SkillRechargeTick' );
	}

	// healing, energizing, re-equiping
	if( UsingActor? )
	{
		UsingController = U2PlayerController(UsingActor);
		if( UsingController? )
			UsingPawn = U2Pawn(UsingController.Pawn);
		if( ValidPawn( Self ) && ValidPawn( UsingPawn ) )
			UsingPawn.HandleUsing( Self, DeltaTime );
		else
			OnUnuse( None );
	}
}

//-----------------------------------------------------------------------------
// Animation support.

simulated function PlayStanding()
{
	LoopAnim( 'Stand' );
}

//-----------------------------------------------------------------------------

function OnUse( Actor Other )
{
	if( GetTeam() == Other.GetTeam() )
		UsingActor = Other;
}

//-----------------------------------------------------------------------------

function OnUnuse( Actor Other )
{
	UsingActor = None;
}

//-----------------------------------------------------------------------------

interface function bool HasUseBar( Controller User )
{
	if( User.GetTeam() == GetTeam() )
	{
		if( Health > 0 )									// teammate wants to heal/repair/supply
			return true;
		if( bCanBeRevived )									// teammate wants to revive
			return true;
	}

	if( U2Pawn(User.Pawn)? )
		return U2Pawn(User.Pawn).GetSpecialHasUseBar( Self );

	return false;										// tapped out pawn or other condition
}

interface function float GetUsePercent( Controller User )
{
	if( Health > 0 && U2Pawn(User.Pawn)? )
		return U2Pawn(User.Pawn).GetSpecialUsePercent( Self );
	else return -1.0;
}

interface function string GetDescription( Controller User )
{
	if( U2Pawn(User.Pawn)? )
		return U2Pawn(User.Pawn).GetSpecialUseDescription( Self );
	else 
		return "";
}

interface function int GetUseIconIndex( Controller User )
{
	local U2Pawn P;
	if( Health <= 0 )
		return 3;
	P = U2Pawn(User.Pawn);
	if( P? )
		return P.GetSpecialUseIconIndex(Self);
	else
		return 0;
}

// skill-based using (Ranger: heal Tech: shield Gunner: equip
interface function bool   GetSpecialHasUseBar( Pawn Other )			{ return false; }
interface function string GetSpecialUseDescription( Pawn Other )	{ return "";	}
interface function float  GetSpecialUsePercent ( Pawn Other )		{ return 0.0;	}
interface function int    GetSpecialUseIconIndex( Pawn Other )		{ return 0;		}

interface function int GetPlayerClassIndex() { return IconIndex; }
interface function int GetPlayerTeamIndex() { return GetTeam(); }

interface function bool GetArtifactEnabled()	{ return PlayerReplicationInfo.bHasArtifact; }
interface function int GetArtifactIndex()		{ return PlayerReplicationInfo.HasArtifact.IconIndex; }

interface function bool LowHealth()	{ return (PlayerReplicationInfo.RequestMedicTime >= Level.TimeSeconds) || (GetHealthPct() < 0.3); }
interface function bool LowEnergy()	{ return (PlayerReplicationInfo.RequestRepairsTime >= Level.TimeSeconds) || ((PowerSuitPower / PowerSuitMax) < 0.3); }
interface function bool LowAmmo()	{ return (PlayerReplicationInfo.RequestSuppliesTime >= Level.TimeSeconds) || (PlayerReplicationInfo.AmmoEquipedPct < 0.5); }

interface function float GetPIDDistancePct() { return FMax( 64, 255 * ( 1.0 - VSize(Location - class'UIConsole'.static.GetPlayerOwner().Pawn.Location) / MaxPlayerIDDistance ) ); }

//-----------------------------------------------------------------------------
// Power suits

function SetPowerSuitPower( float NewPower )	{	PowerSuitPower = NewPower;	}
function float GetPowerSuitPower()				{	return PowerSuitPower;	}
function float GetPowerSuitMax()				{	return PowerSuitMax;	}

//-----------------------------------------------------------------------------

simulated function bool CanBeRevived( optional Actor Other )
{
	local U2Pawn P;
	P = U2Pawn(Other);
	if( P? )
	{
		// we got a healer, make sure he's got skillz
		return( bCanBeRevived && P.GetTeam() == GetTeam() && P.Skill >= P.ReviveCost );
	}
	else
	{
		// just check if we have have the ability
		return true;
	}
}

//-----------------------------------------------------------------------------

function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	local float HeadShotScale;

	// This is getting called on the client sometimes.
	if( Role < ROLE_Authority )
		return;

	HeadShotScale = HeadshotScaling(HitLocation);

	bLastDamageWasHeadshot = HeadShotScale > 1.0;

	Damage *= HeadShotScale;

	CalcHitIndication( Damage, HitLocation, Momentum );

	if( Damage > 0 )
	{
		if( ClassIsChildOf( DamageType, class'DamageTypeThermal' ) && (DamageType != class'DamageTypeThermalFlaming') )
		{
			FlameIndex++;
			//!!ARL: Set up (reset) latent damage timer here.
		}
		else if( ClassIsChildOf( DamageType, class'DamageTypeEMP' ) )
		{
			EMPIndex++;
		}
		else if( ClassIsChildOf( DamageType, class'DamageTypeBiologicalGas' ) )
		{
			ToxicIndex++;
		}
	}

	Super.TakeDamage( Damage, instigatedBy, hitlocation, momentum, damageType );
}

//-----------------------------------------------------------------------------

function float HeadshotScaling( vector HitLocation )
{
	local float Height;
	Height = (HitLocation - Location).Z;
//	if (Height <= 0.0)
//		return 1.0;
	if (Height < CollisionHeight*0.4)
		return 1.0;
	return 1.5;
}

//-----------------------------------------------------------------------------

function TossArtifacts()
{
	local Inventory Inv;
	local Artifact A;
	local ArtifactPickup AP;
	local vector TossVel;

	TossVel = Vector(GetViewRotation()) + 0.2*VRand();
	TossVel = TossVel * ((Velocity Dot TossVel) + 500) + Vect(0,0,200);

	for( Inv = Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		A = Artifact(Inv);
		if( A != None )
		{
			XMPGame(Level.Game).NotifyArtifactDropped( A.ID, Self );

			AP = ArtifactPickup( TossItem( Inv, TossVel ) );
			if( AP != None )
			{
				AP.ID = A.ID;
				AP.SetTeam( A.GetTeam() );
			}
			else
			{
				// spawn failed, send home
				Log("Spawning artifact pickup failed -- sending artifact #" @ A.ID @ "home to team" @ XMPGame(Level.Game).GetArtifactTeam( A.ID ) );
				XMPGame(Level.Game).NotifyArtifactSentHome( A.ID, None );
			}
		}
	}
}

//-----------------------------------------------------------------------------

function Pickup TossItem( Inventory Inv, Vector TossVel )
{
	local Vector X,Y,Z;
	Inv.Velocity = TossVel;
	GetAxes(Rotation,X,Y,Z);
	return Inv.DropFrom(Location + 0.8 * CollisionRadius * X - 0.5 * CollisionRadius * Y); 
}	

//-----------------------------------------------------------------------------

simulated function HandleDying()
{
	GotoState('DyingWaiting');
	if( Controller != None )
		Controller.HandleDying();
}

//-----------------------------------------------------------------------------

simulated function bool CanBeMercyKilled( optional Actor MercyKiller )
{
	if( MercyKiller? )
		return( bCanBeMercyKilled && !bDeleteMe && Health <= 0 && MercyKiller? && MercyKiller.GetTeam() != GetTeam() );
	else
		return( bCanBeMercyKilled && !bDeleteMe && Health <= 0 );
}

//-----------------------------------------------------------------------------

auto state JustDeployed
{
Begin:
	bInvulnerableBody = true;
	Sleep( DeployProtectionTime );
	bInvulnerableBody = false;
	GotoState('');
}

//-----------------------------------------------------------------------------

state DyingWaiting
{
ignores Trigger, Bump, HitWall, HeadVolumeChange, PhysicsVolumeChange, Falling, BreathTimer, ChangedWeapon;

	function Died(Controller Killer, class<DamageType> damageType, vector HitLocation);

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

		if ( !IsAnimating(0) )
			LieStill();
		Velocity = vect(0,0,0);
		Acceleration = vect(0,0,0);
		SetCollision(true, false, false);
		KSetBlockKarma( false );
	}

	function DoRevive( U2Pawn Other )
	{
		Health = Other.RevivedHealth;
		HandleRevived();
		bPlayedDeath = false; // turn off death animation
		SetPhysics(PHYS_Falling);
	    AssignInitialPose();      
		SetCollision( default.bCollideActors, default.bBlockActors, default.bBlockPlayers );
		KSetBlockKarma( default.bBlockKarma );
		Other.UseSkill( Other.ReviveCost );
		XMPGame(Level.Game).NotifyRevived( Other.Controller, Controller );
	}

	simulated function HandleDying()
	{
		bTearOff=true;
		GotoState('Dying');
	}

	function DoMercyKill( U2Pawn Killer )
	{
		Level.Game.BroadcastLocalized(Level.Game, Level.Game.DeathMessageClass, 2, Killer.PlayerReplicationInfo, PlayerReplicationInfo, class'DamageType');

		if( Controller != None )
			Controller.HandleMercyKilled();
	}

	function ClientRevived()
	{
		if ( Controller != None )
			Controller.ClientRevived();
	}

	simulated function HandleRevived()
	{
		if( Level.NetMode != NM_Client && Level.NetMode != NM_ListenServer )
			ClientRevived();

		bUseEnabled = true;
		GotoState('PlayerWalking');

		if( Controller != None )
			Controller.HandleRevived();
	}

	function ReduceCylinder()
	{
	}

	function LandThump()
	{
		// animation notify - play sound if actually landed, and animation also shows it
		if ( Physics == PHYS_None)
			bThumped = true;
	}

	function LieStill()
	{
		if ( !bThumped )
			LandThump();
		Velocity = vect(0,0,0);
		Acceleration = vect(0,0,0);
	}

	function OnUse( Actor Other )
	{
		local U2Pawn User;
		if( Controller(Other)? )
			Other = Controller(Other).Pawn;
		User = U2Pawn(Other);
		if( CanBeRevived( User ) )
			DoRevive( User );
		else if( CanBeMercyKilled( User ) )
			DoMercyKill( User );
	}

	function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, 
							Vector momentum, class<DamageType> damageType)
	{
		// if gib or loses too much health, then die/gib for real
	}

	event BeginState()
	{
		// in case we're already landed
		if( Physics != PHYS_Falling )
			SetCollision(true, false, false);
		bUseEnabled = false;
		bCanBeMercyKilled = true;
		bCanBeRevived = true;
	}

	event EndState()
	{
		bCanBeMercyKilled = false;
		bCanBeRevived = false;
	}

} // DyingWaiting

//-----------------------------------------------------------------------------
// animation

simulated function name GetAnimSequence()
{
    local name anim;
    local float frame, rate;
    
    GetAnimParams(0, anim, frame, rate);
    return anim;
}

//-----------------------------------------------------------------------------

simulated function PlayDoubleJump()
{
    local name Anim;

	if( Health > 0 )
	{
		Anim = DoubleJumpAnims[Get4WayDirection()];
		if ( PlayAnim(Anim, 1.0, 0.1) )
			bWaitForAnim = true;
		AnimAction = Anim;

		JumpJetIndex++;
	}
}

//-----------------------------------------------------------------------------

simulated function bool CheckTauntValid( name Sequence ) 
{
	local int i;

	for(i=0; i<TauntAnims.Length; i++)
	{
		if(Sequence == TauntAnims[i])
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

simulated function bool SelectTaunt( optional int Index )
{
	local name SelectedAnim;

	if( Index <= 0 || Index >= TauntAnims.Length-3 )
		Index = Rand(TauntAnims.Length - 3);
	else
		Index--;

	SelectedAnim = TauntAnims[ Index+4 ];
	if( CheckTauntValid( SelectedAnim ) )
	{
		SetAnimAction( SelectedAnim );
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------

simulated function bool SelectGesture( optional int Index )
{
	local name SelectedAnim;

	if( Index <= 0 || Index > 4 )
		Index = Rand(TauntAnims.Length - 3);
	else
		Index--;

	SelectedAnim = TauntAnims[ Index ];
	if( CheckTauntValid( SelectedAnim ) )
	{
		SetAnimAction( SelectedAnim );
		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------

simulated event SetAnimAction( name NewAction )
{
    AnimAction = NewAction;
    if( Health <= 0 )
    {
		if( !bPlayedDeath )
		{
		    PlayAnim( NewAction, , 0.1 );
			AnimBlendToAlpha( 1, 0.0, 0.05 );
			bWaitForAnim = true;
			DeathAnim = NewAction;
		}
    }
    else if (!bWaitForAnim )
    {
		if ( AnimAction == 'Weapon_Switch' )
        {
            AnimBlendParams(1, 1.0, 0.0, 0.2, FireRootBone);
            PlayAnim( NewAction, , 0.0, 1 );
        }
        else if ( (Physics == PHYS_None) 
			|| ((Level.Game != None) && Level.Game.IsInState('MatchOver')) )
        {
            PlayAnim(AnimAction , , 0.1 );
			AnimBlendToAlpha( 1, 0.0, 0.05 );
        }
        else if ( (Physics == PHYS_Falling) || ((Physics == PHYS_Walking) && (Velocity.Z != 0)) ) 
		{
			if ( CheckTauntValid(AnimAction) )
			{
				if (FireState == FAS_None || FireState == FAS_Ready)
				{
					AnimBlendParams( 1, 1.0, 0.0, 0.2, FireRootBone );
					PlayAnim( NewAction, , 0.1, 1 );
					FireState = FAS_Ready;
				}
			}			
			else 
			{
				if ( PlayAnim(AnimAction) )
				{
					if ( Physics != PHYS_None )
						bWaitForAnim = true;
				}
			}
		}
        else if (bIsIdle && !bIsCrouched ) // standing taunt
        {
            PlayAnim( AnimAction, , 0.1 );
			AnimBlendToAlpha( 1, 0.0, 0.05 );
        }
        else // running taunt
        {
            if (FireState == FAS_None || FireState == FAS_Ready)
            {
                AnimBlendParams( 1, 1.0, 0.0, 0.2, FireRootBone );
                PlayAnim( NewAction, , 0.1, 1 );
                FireState = FAS_Ready;
            }
        }
    }
}

//-----------------------------------------------------------------------------

simulated function SetWeaponAttachment( U2WeaponAttachment NewAtt )
{
    if( NewAtt.bHeavy )
        IdleWeaponAnim = IdleHeavyAnim;
    else
        IdleWeaponAnim = IdleRifleAnim;
}

//-----------------------------------------------------------------------------

simulated function StartFiring(bool bHeavy, bool bRapid)
{
    local name FireAnim;

	/*
    if ( HasUDamage() && (Level.TimeSeconds - LastUDamageSoundTime > 0.25) )
    {
        LastUDamageSoundTime = Level.TimeSeconds;
        PlaySound(UDamageSound, SLOT_None, 1.5*TransientSoundVolume,,700);
    }
	*/
	
    if (Physics == PHYS_Swimming)
        return;

	if( Health <= 0 )
		return;
		
    if (bHeavy)
    {
        if (bRapid)
            FireAnim = FireHeavyRapidAnim;
        else
            FireAnim = FireHeavyBurstAnim;
    }
    else
    {
        if (bRapid)
            FireAnim = FireRifleRapidAnim;
        else
            FireAnim = FireRifleBurstAnim;
    }

    AnimBlendParams(1, 1.0, 0.0, 0.2, FireRootBone);

    if (bRapid)
    {
        if (FireState != FAS_Looping)
        {
            LoopAnim(FireAnim,, 0.0, 1);
            FireState = FAS_Looping;
        }
    }
    else
    {
        PlayAnim(FireAnim,, 0.0, 1);
        FireState = FAS_PlayOnce;
    }

    IdleTime = Level.TimeSeconds;
}

//-----------------------------------------------------------------------------

simulated function StopFiring()
{
    if (FireState == FAS_Looping)
    {
        FireState = FAS_PlayOnce;
    }
    IdleTime = Level.TimeSeconds;
}

//-----------------------------------------------------------------------------

simulated function AnimEnd(int Channel)
{
    if (Channel == 1)
    {
		if( Health > 0 )
		{
			if (FireState == FAS_Ready)
			{
				AnimBlendToAlpha(1, 0.0, 0.12);
				FireState = FAS_None;
			}
			else if (FireState == FAS_PlayOnce)
			{
				PlayAnim(IdleWeaponAnim,, 0.2, 1);
				FireState = FAS_Ready;
				IdleTime = Level.TimeSeconds;
			}
			else
				AnimBlendToAlpha(1, 0.0, 0.12);
		}
    }
    /*
    else if ( bKeepTaunting && (Channel == 0) )
		PlayVictoryAnimation();
	*/
}

//-----------------------------------------------------------------------------

function PlayWeaponSwitch(Weapon NewWeapon)
{
    SetAnimAction('Weapon_Switch');
}

//-----------------------------------------------------------------------------

function PlayVictoryAnimation()
{
	local int tauntNum;

	// First 4 taunts are 'order' anims. Don't pick them.
	tauntNum = Rand(TauntAnims.Length - 3);
	SetAnimAction(TauntAnims[3 + tauntNum]);
}

/*
//-----------------------------------------------------------------------------
// Event called whenever ragdoll convulses

event KSkelConvulse()
{
	if(RagConvulseMaterial != None)
		SetOverlayMaterial(RagConvulseMaterial, 0.4, true);
}
*/

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	if ( PlayerReplicationInfo != None )
    {
		//!!mdf??? Setup(class'xUtil'.static.FindPlayerRecord(PlayerReplicationInfo.CharacterName));
        bNetNotify = false;
    }
}

//-----------------------------------------------------------------------------

//!!mdf??? simulated function Setup( xUtil.PlayerRecord rec, optional bool bLoadNow )
simulated function Setup()
{
	/*!!mdf??
	if ( class'DeathMatch'.default.bForceDefaultCharacter )
		rec = class'xUtil'.static.FindPlayerRecord("Gorge");	
 
    if ( rec.Species == None )
    {
		if ( Level.NetMode == NM_Standalone )
			warn("Could not load species "$rec.Species$" for "$rec.DefaultName);
		return;
	}
    Species = rec.Species;
	RagdollOverride = rec.Ragdoll;
	if ( Species.static.Setup(self,rec) )
	*/
		ResetPhysicsBasedAnim();
}

//-----------------------------------------------------------------------------

simulated function ResetPhysicsBasedAnim()
{
    bIsIdle = false;
    bWaitForAnim = false;
}

//-----------------------------------------------------------------------------

simulated final function RandSpin(float spinRate)
{
    DesiredRotation = RotRand(true);
    RotationRate.Yaw = spinRate * 2 *FRand() - spinRate;
    RotationRate.Pitch = spinRate * 2 *FRand() - spinRate;
    RotationRate.Roll = spinRate * 2 *FRand() - spinRate;   

    bFixedRotationDir = true;
    bRotateToDesired = false;
}

//-----------------------------------------------------------------------------

function PlayTakeHit(vector HitLocation, int Damage, class<DamageType> DamageType)
{
    PlayDirectionalHit(HitLocation);

	/*!!sounds
    if( Level.TimeSeconds - LastPainSound < MinTimeBetweenPainSounds )
        return;

    LastPainSound = Level.TimeSeconds;

    if( HeadVolume.bWaterVolume )
    {
        if( DamageType.IsA('Drowned') )
            PlaySound( GetSound(EST_Drown), SLOT_Pain,1.5*TransientSoundVolume ); 
        else
            PlaySound( GetSound(EST_HitUnderwater), SLOT_Pain,1.5*TransientSoundVolume ); 
        return;
    }

    PlaySound(SoundGroupClass.static.GetHitSound(), SLOT_Pain,2*TransientSoundVolume,,400); 
	*/
}

/*!!mdf-tbd:
//-----------------------------------------------------------------------------
// Called when in Ragdoll when we hit something over a certain threshold velocity
// Used to play impact sounds.

event KImpact(actor other, vector pos, vector impactVel, vector impactNorm)
{
	local int numSounds, soundNum;
	numSounds = RagImpactSounds.Length;

	//log("ouch! iv:"$VSize(impactVel));

	if(numSounds > 0 && Level.TimeSeconds > RagLastSoundTime + RagImpactSoundInterval)
	{
		soundNum = Rand(numSounds);
		//Log("Play Sound:"$soundNum);
		PlaySound(RagImpactSounds[soundNum], SLOT_Pain, RagImpactVolume);
		RagLastSoundTime = Level.TimeSeconds;
	}
}
*/

//-----------------------------------------------------------------------------

simulated function PlayDying(class<DamageType> DamageType, vector HitLoc)
{
	Super.PlayDying( DamageType, HitLoc );
    PlayDirectionalDeath(HitLoc);
}

//-----------------------------------------------------------------------------

simulated function PlayDirectionalDeath( vector HitLoc )
{
    local vector X,Y,Z, Dir;
	local name AnimName;

    GetAxes(Rotation, X,Y,Z);
    HitLoc.Z = Location.Z;
    
    // random
    if ( VSize(Velocity) < 10.0 && VSize(Location - HitLoc) < 1.0 )
    {
        Dir = VRand();
    }
    // velocity based
    else if ( VSize(Velocity) > 0.0 )
    {
        Dir = Normal(Velocity*Vect(1,1,0));
    }
    // hit location based
    else
    {
        Dir = -Normal(Location - HitLoc);
    }

    if ( Dir Dot X > 0.7 || Dir == vect(0,0,0))
        AnimName = 'DeathB';
    else if ( Dir Dot X < -0.7 )
        AnimName = 'DeathF';
    else if ( Dir Dot Y > 0 )
        AnimName = 'DeathL';
    else
        AnimName = 'DeathR';
        
	SetAnimAction( AnimName );
}

//-----------------------------------------------------------------------------

simulated function PlayDirectionalHit(Vector HitLoc)
{
    local Vector X,Y,Z, Dir;

    GetAxes(Rotation, X,Y,Z);
    HitLoc.Z = Location.Z;
    
    // random
    if ( VSize(Location - HitLoc) < 1.0 )
    {
        Dir = VRand();
    }
    // hit location based
    else
    {
        Dir = -Normal(Location - HitLoc);
    }

    if ( Dir Dot X > 0.7 || Dir == vect(0,0,0))
    {
        PlayAnim('HitF',, 0.1);
    }
    else if ( Dir Dot X < -0.7 )
    {
        PlayAnim('HitB',, 0.1);
    }
    else if ( Dir Dot Y > 0 )
    {
        PlayAnim('HitR',, 0.1);
    }
    else
    {
        PlayAnim('HitL',, 0.1);
    }
}

//-----------------------------------------------------------------------------

simulated function PlayFootStep()
{
	if ( (Role==ROLE_SimulatedProxy) || (PlayerController(Controller) == None) || PlayerController(Controller).bBehindView )
		FootStepping();
}

//-----------------------------------------------------------------------------

function ESurfaceType GetSurfaceType()
{
    local int i;
    local Actor A;
	local vector HL, HN, Start, End;

    for( i=0; i<Touching.Length; i++ )
		if( (PhysicsVolume(Touching[i]) != None && PhysicsVolume(Touching[i]).bWaterVolume) || FluidSurfaceInfo(Touching[i]) != None )
			return SFT_Water;
	
	if( Base != None )
	{
		if( Base.IsA( 'TerrainInfo') )
			return SFT_Terrain;
	}
	else
	{
		Start = Location - vect(0,0,1)*CollisionHeight;
		End = Start - vect(0,0,32);
		A = Trace( HL, HN, End, Start, false );
		if( A != None && A.IsA( 'TerrainInfo') )
			return SFT_Terrain;
	}		


	return SFT_Default;
}	

//-----------------------------------------------------------------------------

simulated function FootStepping()
{
	local int SoundIndex;
    local float SelectedVolume, SelectedPitch;
    local Sound SelectedSound;
    local ESurfaceType SurfaceType;

	if( bIsCrouched || bIsWalking )
		return;
	
	SurfaceType = GetSurfaceType();
	
	switch( SurfaceType )
	{
	case SFT_Water:
		SoundIndex = Rand( SoundFootstepsWater.Length );
		SelectedSound = SoundFootstepsWater[ SoundIndex ];
		break;
	case SFT_Terrain:
		SoundIndex = Rand( SoundFootstepsTerrain.Length );
		SelectedSound = SoundFootstepsTerrain[ SoundIndex ];
		break;
	default:
		SoundIndex = Rand( SoundFootstepsDefault.Length );
		SelectedSound = SoundFootstepsDefault[ SoundIndex ];
		break;
	}
		
	SelectedPitch = class'Util'.static.PerturbFloatPercent( FootstepPitch, MovementPitchVariancePercent );
	SelectedVolume = class'Util'.static.PerturbFloatPercent( FootstepVolume, MovementVolumeVariancePercent * VSize(Velocity) / default.GroundSpeed );
	PlaySound( SelectedSound, SLOT_Interact, SelectedVolume, /*bNoOverride*/, MovementRadius, SelectedPitch );
	//DMTN( "FootStepping: " $ SelectedSound $ " @ " $ SelectedVolume );
}

//-----------------------------------------------------------------------------

simulated function PlayFootStepLeft()
{
	//DMTN( "PlayFootStepLeft" );
    PlayFootStep();
}

//-----------------------------------------------------------------------------

simulated function PlayFootStepRight()
{
	//DMTN( "PlayFootRight" );
    PlayFootStep();
}

//-----------------------------------------------------------------------------

function CheckBob( float DeltaTime, vector Y )
{
	//!!mdf-tbd: is this approach so bad (can scale with speed/model)? Will be out of sync with viewbob though...
	Super.CheckBob( DeltaTime, Y );
		
	if( Physics != PHYS_Walking || VSize(Velocity) < (WalkingPct*GroundSpeed)  || 
	    (PlayerController(Controller) != None && PlayerController(Controller).bBehindView) )
		return;
		
	if( bPlayOwnFootSteps && !bIsWalking && !bIsCrouched && Level.TimeSeconds >= NextFootstepTime )
	{
		NextFootStepTime = Level.TimeSeconds + TimeBetweenFootSteps; 
		FootStepping(); //assumes left/right use same sounds, otherwise change to alternate 1/-1
	}
			
	/*
	local float OldBobTime;
	local int m,n;

	OldBobTime = BobTime;
	
	Super.CheckBob( DeltaTime, Y );
	
	if( Physics != PHYS_Walking || VSize(Velocity) < 10 || 
	    (PlayerController(Controller) != None && PlayerController(Controller).bBehindView) )
		return;

	m = int(0.5 * Pi + 9.0 * OldBobTime/Pi);
	n = int(0.5 * Pi + 9.0 * BobTime/Pi);

	DMTN( "CheckBob" );
	
	if( m != n && !bIsWalking && !bIsCrouched )
	{
		FootStepping();
	}
	else if( !bWeaponBob && bPlayOwnFootSteps && !bIsWalking && !bIsCrouched && Level.TimeSeconds >= NextFootStepTime )
	{
		NextFootStepTime = Level.TimeSeconds + 0.35; 
		FootStepping();
	}
	*/
}

//-----------------------------------------------------------------------------

event Landed( vector HitNormal )
{
	local int SoundIndex;
    local float SelectedVolume, SelectedPitch;
    local Sound SelectedSound;
    local ESurfaceType SurfaceType;

    Super.Landed( HitNormal );

	if( (Level.TimeSeconds - LastLandTime) < 1.0 )
		return;
		
	LastLandTime = Level.TimeSeconds; 
   
	SurfaceType = GetSurfaceType();
	
	switch( SurfaceType )
	{
	case SFT_Water:
		SoundIndex = Rand( SoundLandWater.Length );
		SelectedSound = SoundLandWater[ SoundIndex ];
		break;
	case SFT_Terrain:
		SoundIndex = Rand( SoundLandTerrain.Length );
		SelectedSound = SoundLandTerrain[ SoundIndex ];
		break;
	default:
		SoundIndex = Rand( SoundLandDefault.Length );
		SelectedSound = SoundLandDefault[ SoundIndex ];
		break;
	}
		
	SelectedPitch = class'Util'.static.PerturbFloatPercent( LandPitch, MovementPitchVariancePercent );
	SelectedVolume = class'Util'.static.PerturbFloatPercent( LandVolume, MovementVolumeVariancePercent ) * Abs(Velocity.Z) / 400.0;
	PlaySound( SelectedSound, SLOT_Interact, SelectedVolume, /*bNoOverride*/, MovementRadius, SelectedPitch );
	//DMTN( "Land: " $ SelectedSound $ " @ " $ SelectedVolume );
}

//-----------------------------------------------------------------------------

function bool CanPickupItem( Actor Item )
{
	return false;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bCanWalkOffLedges=true

	bUsable=true 
	bShowUseReticle=false
	ControllerClass=class'U2PlayerController'
	AmbientGlow=64
	bActorShadows=true
	bHasBlood=true

	AirControl=0.25

	// stamina
	StaminaMax=100.0
	StaminaRechargeRate=10.0

	// sprinting
	bCanSprint=true
	SprintCost=25.0
	SprintRatio=1.5

	// jump jets
	bCanDoubleJump=true
	JumpJetZ=500.0
	JumpJetCost=0.0050
	JumpJetDuration=0.25
	JumpJetThrustSound=Sound'U2A.Jumping.JumpJetThrust'
	MaxFallSpeed=1600.0

	// latent damage
	FlameEffect=ParticleRadiator'onfire_fx.ParticleRadiator0'
	EMPEffect=ParticleRadiator'EMP_Hit_FX.ParticleRadiator0'
	ToxicEffect=ParticleRadiator'Toxic_Hit_FX.ParticleRadiator0'

	// skill level
	SkillMax=0.0100
	SkillRechargeRate=0.0010
	SkillTeamEnergyCost=3.0

	// mantling
	MantleHandsUpwardReach=16.000000
	MantleHandsForwardReachMin=8.000000
	MantleHandsForwardReachMax=24.000000
	MantleMinHitNormalDeltaZ=0.707000
	MantleSpeed=600.000000
	MantleMaxStandingMantleHeight=125.0
	MantleDebugLevel=0
	MantleSound=Sound'U2A.Mantling.Mantle'
	MantleHurtSound=Sound'U2A.Mantling.MantleHurt'
	MaxFallingMantleSpeed=1400.0f
	MaxRisingMantleSpeed=-1.0f

	RevivedHealth=1
	ReviveCost=0.0050
	HackRate=0.6
	HackCost=0.0018

	// animation
    bDoTorsoTwist=true
    DodgeSpeedFactor=1.5
	bPhysicsAnimUpdate=true
    MovementAnims(0)=RunF
    MovementAnims(1)=RunB
    MovementAnims(2)=RunL
    MovementAnims(3)=RunR
    SwimAnims(0)=SwimF
    SwimAnims(1)=SwimB
    SwimAnims(2)=SwimL
    SwimAnims(3)=SwimR
    CrouchAnims(0)=CrouchF
    CrouchAnims(1)=CrouchB
    CrouchAnims(2)=CrouchL
    CrouchAnims(3)=CrouchR
    WalkAnims(0)=WalkF
    WalkAnims(1)=WalkB
    WalkAnims(2)=WalkL
    WalkAnims(3)=WalkR
    AirStillAnim=Jump_Mid
    AirAnims(0)=JumpF_Mid
    AirAnims(1)=JumpB_Mid
    AirAnims(2)=JumpL_Mid
    AirAnims(3)=JumpR_Mid
    TakeoffStillAnim=Jump_Takeoff
    TakeoffAnims(0)=JumpF_Takeoff
    TakeoffAnims(1)=JumpB_Takeoff
    TakeoffAnims(2)=JumpL_Takeoff
    TakeoffAnims(3)=JumpR_Takeoff
    LandAnims(0)=JumpF_Land
    LandAnims(1)=JumpB_Land
    LandAnims(2)=JumpL_Land
    LandAnims(3)=JumpR_Land
    MantleAnim=Mantle //!!mdf-tbd: Mantle (add to anims)
    DoubleJumpAnims(0)=DoubleJumpF
    DoubleJumpAnims(1)=DoubleJumpB
    DoubleJumpAnims(2)=DoubleJumpL
    DoubleJumpAnims(3)=DoubleJumpR
    DodgeAnims(0)=DodgeF
    DodgeAnims(1)=DodgeB
    DodgeAnims(2)=DodgeL
    DodgeAnims(3)=DodgeR
    WallDodgeAnims(0)=WallDodgeF
    WallDodgeAnims(1)=WallDodgeB
    WallDodgeAnims(2)=WallDodgeL
    WallDodgeAnims(3)=WallDodgeR

    TurnRightAnim=TurnR
    TurnLeftAnim=TurnL
    CrouchTurnRightAnim=Crouch_TurnR
    CrouchTurnLeftAnim=Crouch_TurnL
    IdleRestAnim=Idle_Rest
    IdleCrouchAnim=Crouch
    IdleSwimAnim=Swim_Tread
    IdleWeaponAnim=Idle_Rifle
    IdleHeavyAnim=Idle_Biggun
    IdleRifleAnim=Idle_Rifle
    FireHeavyRapidAnim=Biggun_Burst
    FireHeavyBurstAnim=Biggun_Aimed
    FireRifleRapidAnim=Rifle_Burst
    FireRifleBurstAnim=Rifle_Aimed

	TauntAnims(0)=gesture_point
	TauntAnimNames(0)="Point"

	TauntAnims(1)=gesture_beckon
	TauntAnimNames(1)="Beckon"

	TauntAnims(2)=gesture_halt
	TauntAnimNames(2)="Halt"

	TauntAnims(3)=gesture_cheer
	TauntAnimNames(3)="Cheer"

	TauntAnims(4)=PThrust
	TauntAnimNames(4)="Pelvic Thrust"

	TauntAnims(5)=AssSmack
	TauntAnimNames(5)="Ass Smack"
	
	TauntAnims(6)=ThroatCut
	TauntAnimNames(6)="Throat Cut"
	
	TauntAnims(7)=Specific_1
	TauntAnimNames(7)="Unique"

    //MaxMultiJump=1
    //MultiJumpRemaining=1
    //MultiJumpBoost=25
    //VoiceType="xGame.MercMaleVoice"

    RootBone="Bip01"
    HeadBone="Bip01 Head"
    SpineBone1="Bip01 Spine1"
    SpineBone2="Bip01 Spine2"
    FireRootBone="Bip01 Spine"

	bPlayOwnFootsteps=true

	FlashbangHum=sound'U2A.Effects.Flashbang'

	MaxPlayerIDDistance=5000.0

	ReviveMsg="Revive"

	DeployProtectionTime=1.0
}
