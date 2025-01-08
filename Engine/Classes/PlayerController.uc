//=============================================================================
// PlayerController
//
// PlayerControllers are used by human players to control pawns.
//
// This is a built-in Unreal class and it shouldn't be modified.
// for the change in Possess().
//=============================================================================
class PlayerController extends Controller
	config(user)
	native
    nativereplication;

// Player info.
var const player Player;

// player input control
var globalconfig	bool 	bLookUpStairs;	// look up/down stairs (player)
var globalconfig	bool	bSnapToLevel;	// Snap to level eyeheight when not mouselooking
var globalconfig	bool	bAlwaysMouseLook;
var globalconfig	bool	bKeyboardLook;	// no snapping when true
var bool					bCenterView;

// Player control flags
var bool		bBehindView;    // Outside-the-player view.
var bool		bFrozen;		// set when game ends or player dies to temporarily prevent player from restarting (until cleared by timer)
var bool		bPressedJump;
var	bool		bDoubleJump;
var bool		bUpdatePosition;
var bool		bIsTyping;
var bool		bFixedCamera;	// used to fix camera in position (to view animations)
var bool		bJumpStatus;	// used in net games
var	bool		bUpdating;
var globalconfig bool	bNeverSwitchOnPickup;	// if true, don't automatically switch to picked up weapon
var globalconfig bool	bAutoSwitch;			// if true, switch to best weapon when out of ammo

var bool		bZooming;

var globalconfig bool bAlwaysLevel;
var bool		bSetTurnRot;
var bool		bCheatFlying;	// instantly stop in flying mode
var bool		bFreeCamera;	// free camera when in behindview mode (for checking out player models and animations)
var	bool		bZeroRoll;
var	bool		bCameraPositionLocked;
var	bool		bViewBot;
var bool		UseFixedVisibility;
var bool	bBlockCloseCamera;
var bool	bValidBehindCamera;
var bool	bForcePrecache;
var bool	bClientDemo;
var const bool bAllActorsRelevant;	// used by UTTV.  DO NOT SET THIS TRUE - it has a huge impact on network performance
var bool	bShortConnectTimeOut;	// when true, reduces connect timeout to 15 seconds
var bool	bPendingDestroy;		// when true, playercontroller is being destroyed

var globalconfig bool bNoVoiceMessages;
var globalconfig bool bNoVoiceTaunts;
var globalconfig bool bNoAutoTaunts;
var globalconfig bool bAutoTaunt;
var globalconfig bool bNoMatureLanguage;
var globalconfig bool bDynamicNetSpeed;

var globalconfig byte AnnouncerLevel;  // 0=none, 1=no possession announcements, 2=all
var globalconfig byte AnnouncerVolume; // 1 to 4
var globalconfig float AimingHelp;
var globalconfig float MaxResponseTime;		// how long server will wait for client move update before setting position
var float WaitDelay;			// Delay time until can restart

var input float
	aBaseX, aBaseY, aBaseZ,	aMouseX, aMouseY,
	aForward, aTurn, aStrafe, aUp, aLookUp;

var input byte
	bStrafe, bSnapLevel, bLook, bFreeLook, bTurn180, bTurnToNearest, bXAxis, bYAxis;

var EDoubleClickDir DoubleClickDir;		// direction of movement key double click (for special moves)

// Camera info.
var int ShowFlags;
var int Misc1,Misc2;
var int RendMap;
var float        OrthoZoom;     // Orthogonal/map view zoom factor.
var const actor ViewTarget;
var const Controller RealViewTarget;
var PlayerController DemoViewer;
var float CameraDist;		// multiplier for behindview camera dist
var vector OldCameraLoc;		// used in behindview calculations
var rotator OldCameraRot;
var transient array<CameraEffect> CameraEffects;	// A stack of camera effects.
var Rotator TempTurretRotation;

var Actor LastViewTargetActor;
var float LastViewTargetTime;
var float LastViewTargetTraceDistance;
var float LastViewTargetActualDistance;
var vector LastViewTargetLocation;
var rotator LastViewTargetRotation;

var globalconfig float DesiredFOV;
var globalconfig float DefaultFOV;
var float		ZoomLevel;

// Vehicle Passenger info
var int		PassengerNumber;
var KVehicle DrivenVehicle;
var float	EntryTime;

// Fixed visibility.
var vector	FixedLocation;
var rotator	FixedRotation;
var matrix	RenderWorldToCamera;

// Screen flashes
var vector FlashScale, FlashFog;
var float ConstantGlowScale;
var vector ConstantGlowFog;

// Distance fog fading.
var color	LastDistanceFogColor;
var float	LastDistanceFogStart;
var float	LastDistanceFogEnd;
var float	CurrentDistanceFogEnd;
var float	TimeSinceLastFogChange;
var int		LastZone;

// Remote Pawn ViewTargets
var rotator		TargetViewRotation; 
var rotator     BlendedTargetViewRotation;
var float		TargetEyeHeight;
var vector		TargetWeaponViewOffset;

var float LastPlaySound;
var float LastPlaySpeech;

// Music info.
var string				Song;
var EMusicTransition	Transition;
var array<string>		LoadingMusic;

// Move buffering for network games.  Clients save their un-acknowledged moves in order to replay them
// when they get position updates from the server.
var SavedMove SavedMoves;	// buffered moves pending position updates
var SavedMove FreeMoves;	// freed moves, available for buffering
var SavedMove PendingMove;	
var float CurrentTimeStamp,LastUpdateTime,ServerTimeStamp,TimeMargin, ClientUpdateTime;
var globalconfig float MaxTimeMargin;
var Weapon OldClientWeapon;
var int WeaponUpdate;

// Progess Indicator - used by the engine to provide status messages (HUD is responsible for displaying these).
var string	ProgressMessage[4];
var color	ProgressColor[4];
var float	ProgressTimeOut;

// Localized strings
var localized string QuickSaveString;
var localized string NoPauseMessage;
var localized string ViewingFrom;
var localized string OwnCamera;

// ReplicationInfo
var GameReplicationInfo GameReplicationInfo;

// Stats Logging
var globalconfig string StatsUsername;
var globalconfig string StatsPassword;

var class<LocalMessage> LocalMessageClass;

// view shaking (affects roll, and offsets camera position)
var       float ShakeDuration,ShakeTime;
var       float ShakeMagX,ShakeMagY;
var const float ShakePeriodX,ShakePeriodY;
var vector	ShakeOffset; //current magnitude to offset camera from shake

var Pawn		TurnTarget;
var config int	EnemyTurnSpeed;
var int			GroundPitch;
var rotator		TurnRot180;

var vector OldFloor;		// used by PlayerSpider mode - floor for which old rotation was based;

// Components ( inner classes )
var private transient CheatManager	CheatManager;	// Object within playercontroller that manages "cheat" commands
var class<CheatManager>				CheatClass;		// class of my CheatManager
var transient PlayerInput			PlayerInput;	// Object within playercontroller that manages player input.
var config class<PlayerInput>		InputClass;		// class of my PlayerInput
var const vector FailedPathStart;

var float CheatFlyingSpeed;							// ghost can specify speed

// Use
var Actor UseTarget;			// Actor currently in view to be used
var float UseDistance;			// Max distance at which an actor can be used
var float UseLatency;			// Max time before a new trace must be used to find the use target
var Projector UseProjector;		// Used to show which items are usable.

var sound RocketLockSound;

// Demo recording view rotation
var int DemoViewPitch;
var int DemoViewYaw;

var Security PlayerSecurity;	// Used for Cheat Protection
var float ForcePrecacheTime;

var float LastPingUpdate;
var float ExactPing;
var float OldPing;
var float SpectateSpeed;
var globalconfig float DynamicPingThreshold;
var float NextSpeedChange;
var int ClientCap;

var(ForceFeedback) globalconfig bool bEnablePickupForceFeedback;
var(ForceFeedback) globalconfig bool bEnableWeaponForceFeedback;
var(ForceFeedback) globalconfig bool bEnableDamageForceFeedback;
var(ForceFeedback) globalconfig bool bEnableGUIForceFeedback;
var(ForceFeedback) bool bForceFeedbackSupported;  // true if a device is detected

var EPhysics SavedPhysics;	// for saving old physics temporarily

replication
{
	// Things the server should send to the client.
	reliable if( bNetDirty && bNetOwner && Role==ROLE_Authority )
        GameReplicationInfo;
	unreliable if ( bNetOwner && Role==ROLE_Authority && (ViewTarget != Pawn) && (Pawn(ViewTarget) != None) )
		TargetViewRotation, TargetEyeHeight, TargetWeaponViewOffset;
	reliable if( bDemoRecording && Role==ROLE_Authority )
		DemoViewPitch, DemoViewYaw;

	// Functions server can call.
	reliable if( Role==ROLE_Authority )
		ClientSetHUD,ClientReliablePlaySound, FOV, StartZoom, 
		ToggleZoom, StopZoom, EndZoom, ClientSetMusic, ClientRestart,
		ClientAdjustGlow, 
		ClientSetBehindView, ClientSetFixedCamera, ClearProgressMessages, 
        SetProgressMessage, SetProgressTime,
		GivePawn, ClientGotoState,
		ClientChangeVoiceChatter,
		ClientLeaveVoiceChat,
		ClientValidate,
        ClientSetViewTarget, ClientCapBandwidth;

	reliable if ( (Role == ROLE_Authority) && (!bDemoRecording || (bClientDemoRecording && bClientDemoNetFunc)) )
		ClientMessage, TeamMessage, ReceiveLocalizedMessage;
	unreliable if( Role==ROLE_Authority && !bDemoRecording )
        ClientPlaySound,PlayAnnouncement;
	reliable if( Role==ROLE_Authority && !bDemoRecording )
        ClientStopForceFeedback, ClientTravel;
	unreliable if( Role==ROLE_Authority )
        SetFOVAngle, ClientShake, ClientFlash,
		NotifyRocketLockOnA, NotifyRocketLockOnB, NotifyRocketLockOnC, NotifyRocketLockOnD,
		ContinueNotifyRocketLockOnA, ContinueNotifyRocketLockOnB, ContinueNotifyRocketLockOnC, ContinueNotifyRocketLockOnD,
		ClientAdjustPosition, ShortClientAdjustPosition, VeryShortClientAdjustPosition, LongClientAdjustPosition;
	unreliable if( (!bDemoRecording || bClientDemoRecording && bClientDemoNetFunc) && Role==ROLE_Authority )
		ClientHearSound;
    reliable if( bClientDemoRecording && ROLE==ROLE_Authority )
		DemoClientSetHUD;
	reliable if( Role==ROLE_Authority )
		ClientSendEvent;

	// Functions client can call.
	unreliable if( Role<ROLE_Authority )
        ServerUpdatePing, ShortServerMove, ServerMove, 
		Say, TeamSay, 
		ServerViewNextPlayer, ServerViewSelf, ServerUse, ServerUnuse, ServerDrive, ServerToggleBehindView;
	reliable if( Role<ROLE_Authority )
        Speech, Pause, SetPause, Mutate,
		PrevItem, ActivateItem, ServerReStartGame, AskForPawn, ForceRestart, ServerQuit,
		ChangeName, ChangeTeam, Suicide,
		ServerReload, 
		ServerThrowWeapon, BehindView, Typing,
		ServerChangeVoiceChatter,
		ServerGetVoiceChatters,
		ServerValidationResponse, ServerVerifyViewTarget, ServerSpectateSpeed, ServerSetClientDemo,
		ServerCheckCommandLine,
		ServerChangeClass,
		ServerDumpStates;

	reliable if( Role==ROLE_Authority ) // calls will be sent to client if called on server
		ClientAddCheats;
	reliable if( Role<ROLE_Authority )	// calls will be sent to server if called on client
		ServerSummon,
		ServerAddCheats,
		ServerCanCheat,
		ServerSetTeamEnergy;
}

native final function SetNetSpeed(int NewSpeed);
native final function string GetPlayerNetworkAddress();
native final function string GetServerNetworkAddress();
native function string ConsoleCommand( string Command );
native final function LevelInfo GetEntryLevel();
native(544) final function ResetKeyboard();
native final function SetViewTarget(Actor NewViewTarget);
native event ClientTravel( string URL, ETravelType TravelType, bool bItems );
native(546) final function UpdateURL(string NewOption, string NewValue, bool bSaveDefault);
native final function string GetDefaultURL(string Option);
// Execute a console command in the context of this player, then forward to Actor.ConsoleCommand.
native function CopyToClipboard( string Text );
native function string PasteFromClipboard();

// Validation.
private native event ClientValidate(string C);
private native event ServerValidationResponse(string R);

/* FindStairRotation()
returns an integer to use as a pitch to orient player view along current ground (flat, up, or down)
*/
native(524) final function int FindStairRotation(float DeltaTime);

native event ClientHearSound ( 
	actor Actor, 
	int Id, 
	sound S, 
	vector SoundLocation, 
	vector Parameters,
	bool Attenuate
);

event PostBeginPlay()
{
	Super.PostBeginPlay();
	if (Level.LevelEnterText != "" )
		ClientMessage(Level.LevelEnterText);

	DesiredFOV = DefaultFOV;
	SetViewTarget(self);  // MUST have a view target!
	ServerAddCheats();
    
    bForcePrecache = (Role < ROLE_Authority);
    ForcePrecacheTime = Level.TimeSeconds + 2;
}

exec function SetSpectateSpeed(Float F)
{
	SpectateSpeed = F;
	ServerSpectateSpeed(F);
}

function ServerSpectateSpeed(Float F)
{
	SpectateSpeed = F;
}

function ServerGivePawn()
{
	GivePawn(Pawn);
}

function ClientCapBandwidth(int Cap)
{
	ClientCap = Cap;
	if ( (Player != None) && (Player.CurrentNetSpeed > Cap) )
		SetNetSpeed(Cap);
}

event PostNetBeginPlay()
{
	Super.PostNetBeginPlay();
	ServerAddCheats();
}

function bool ServerCheckCommandLine( string CommandStr ) 
{ 
	local bool bRes;
	
	bRes = ( InStr( GetCommandLine(), CommandStr) != -1 );
	return bRes;
}

function PendingStasis()
{
	bStasis = true;
	Pawn = None;
	GotoState('Scripting');
}

interface function bool ShowHUD() { return false; }

interface function string GetUseText()		  { if( UseTarget? ) return UseTarget.GetDescription(Self);  else return ""; }
interface function bool   GetUseBarEnable()   { if( UseTarget? ) return UseTarget.HasUseBar(Self); 		 else return false;	}
interface function float  GetUseBarPercent()  { if( UseTarget? ) return UseTarget.GetUsePercent(Self);	 else return 1.0; }
interface function int    GetUseBarIconIndex(){ if( UseTarget? ) return UseTarget.GetUseIconIndex(Self); else return 0; }

function SetUseTarget()
{
	local Actor HitActor;

	if( Pawn != None && Pawn.bUseEnabled )
	{
		HitActor = PlayerSees( UseDistance, UseLatency );
		if( HitActor != UseTarget )
		{
			ServerUnuse();
			UseTarget = HitActor;
		}
	}
	else if( UseTarget != None )
	{
		ServerUnuse();
		UseTarget = None;
	}
}

function bool ServerCanCheat()
{
	local bool bRes;
	
	bRes = ( Level.NetMode == NM_Standalone || ServerCheckCommandLine( "cheats" ) );
	return bRes;
}

function bool ServerDeployCheat() 
{
	return ServerCheckCommandLine( "deploycheat" ); 
}
	
function ClientAddCheats()
{
	if( CheatManager == None )
		CheatManager = new(Self) CheatClass;
}

function ServerAddCheats()
{
	if( ServerCanCheat() )
		ClientAddCheats();
}

function HandlePickup(Pickup pick)
{
	ReceiveLocalizedMessage(pick.MessageClass,,,,pick.class);
}

function HandleSpawnedObject( Object Obj, string ClassName )
{
	if( Obj == None )
	{
		DM( "error spawning object for class " $ ClassName $ "!" );
	}
	else
	{
		// shove into inventory if an inventory item (no other possible use)?
		if( Inventory(Obj) != None && class'UtilGame'.static.GiveInventory( Pawn, Inventory(Obj) ) )
			DM( "added " $ Obj $ " to inventory" );
		else
			DM( "spawned " $ Obj );
	}
}

exec function ServerSummon( string ClassName )
{
	local Object Obj;
	local class<Object> NewClass;
	
	if( ServerCanCheat() )
	{
		NewClass = class<Object>( DynamicLoadObject( ClassName, class'Class' ) );
		Obj = class'Util'.static.SummonClass( NewClass, ClassName, Self, Pawn, true );
		HandleSpawnedObject( Obj, ClassName );
	}
}

event ClientSetViewTarget( Actor a )
{
	if ( A == None )
		ServerVerifyViewTarget();
    SetViewTarget( a );
}

function ServerVerifyViewTarget()
{
	if ( ViewTarget == self )
		return;

	ClientSetViewTarget(ViewTarget);
}

/* Reset() 
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
    PawnDied(Pawn);
	Super.Reset();
	SetViewTarget(self);
	bBehindView = false;
	WaitDelay = Level.TimeSeconds + 2;
    FixFOV();
    if ( !PlayerReplicationInfo.bOnlySpectator )
		GotoState('PlayerWaiting');
}

function CleanOutSavedMoves()
{
    local SavedMove Next;
	
	// clean out saved moves
	while ( SavedMoves != None )
	{
		Next = SavedMoves.NextMove;
		SavedMoves.Destroy();
		SavedMoves = Next;
	}
	if ( PendingMove != None )
	{
		PendingMove.Destroy();
		PendingMove = None;
	}
}

/* InitInputSystem()
Spawn the appropriate class of PlayerInput
Only called for playercontrollers that belong to local players
*/
event InitInputSystem()
{
	PlayerInput = new(Self) InputClass;
}

/* InitPlayer()
Notification that the Player has just been set.
*/
simulated event InitPlayer()
{
	if( Viewport(Player)? )	// only local players need these.
	{
		if (!UseProjector)
		{
			AddTimer( 'SetUseTarget', 0.1, true );
			UseProjector = Spawn(class'UseProjector',self);
			UseProjector.Trigger(self,Pawn);
		}
	}
}

/* ClientGotoState()
server uses this to force client into NewState
*/
function ClientGotoState(name NewState, name NewLabel)
{
	GotoState(NewState,NewLabel);
}

function AskForPawn()
{
	if ( IsInState('GameEnded') )
		ClientGotoState('GameEnded', 'Begin');
	else if ( Pawn != None )
		GivePawn(Pawn);
	else
	{
		bFrozen = false;
		ServerRestartPlayer();
	}		
}	

function GivePawn(Pawn NewPawn)
{
	if ( NewPawn == None )
		return;
	Pawn = NewPawn;
	NewPawn.Controller = self;
	ClientRestart();
}	

/* GetFacingDirection()
returns direction faced relative to movement dir
0 = forward
16384 = right
32768 = back
49152 = left
*/
function int GetFacingDirection()
{
	local vector X,Y,Z, Dir;

	GetAxes(Pawn.Rotation, X,Y,Z);
	Dir = Normal(Pawn.Acceleration);
	if ( Y Dot Dir > 0 )
		return ( 49152 + 16384 * (X Dot Dir) );
	else
		return ( 16384 - 16384 * (X Dot Dir) );
}

// Possess a pawn
function Possess(Pawn aPawn)
{
    if ( PlayerReplicationInfo.bOnlySpectator )
		return;

	SetRotation(aPawn.Rotation);
	aPawn.PossessedBy(self);
	Pawn = aPawn;
	Pawn.bStasis = false;
    CleanOutSavedMoves();  // don't replay moves previous to possession
	PlayerReplicationInfo.bIsFemale = Pawn.bIsFemale;
	//if (!ClassIsChildOf( aPawn.Class, class'XMPVehicleBarrel' ))
		Restart();
}

// unpossessed a pawn (not because pawn was killed)
function UnPossess()
{
	if ( Pawn != None )
	{
		SetLocation(Pawn.Location);
		Pawn.RemoteRole = ROLE_SimulatedProxy;
		Pawn.UnPossessed();
		CleanOutSavedMoves();  // don't replay moves previous to unpossession
		if ( Viewtarget == Pawn )
			SetViewTarget(self);
	}
	Pawn = None;
	GotoState('Spectating');
}

function ViewNextBot()
{
	if ( CheatManager != None )
		CheatManager.ViewBot();
}

// unpossessed a pawn (because pawn was killed)
function PawnDied(Pawn P)
{
	if ( P != Pawn )
		return;
	EndZoom();
	if ( Pawn != None )
		Pawn.RemoteRole = ROLE_SimulatedProxy;
	if ( ViewTarget == Pawn )
		bBehindView = true;

    Super.PawnDied(P);
}

function ClientSetHUD(name NewHUD, name NewScoreboard)
{
}

// jdf ---
// Server ignores this call, client plays effect
simulated function ClientPlayForceFeedback( String EffectName )
{
    if (bForceFeedbackSupported && Viewport(Player) != None)
        PlayFeedbackEffect( EffectName );
}

simulated function StopForceFeedback( optional String EffectName )
{
    if (bForceFeedbackSupported && Viewport(Player) != None)
    {
		if (EffectName != "")
			StopFeedbackEffect( EffectName );
		else
			StopFeedbackEffect();
	}
}

function ClientStopForceFeedback( optional String EffectName )
{
    if (bForceFeedbackSupported && Viewport(Player) != None)
    {
		if (EffectName != "")
			StopFeedbackEffect( EffectName );
		else
			StopFeedbackEffect();
	}
}
// --- jdf

simulated event ReceiveLocalizedMessage( class<LocalMessage> Message, optional int Switch, optional PlayerReplicationInfo RelatedPRI_1, optional PlayerReplicationInfo RelatedPRI_2, optional Object OptionalObject )
{
	Message.Static.ClientReceive( Self, Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject );
	if ( Message.default.bIsConsoleMessage && (Player != None) && (Player.Console != None) )
		Player.Console.Message(Message.Static.GetString(Switch, RelatedPRI_1, RelatedPRI_2, OptionalObject),0 );
}

event ClientMessage( coerce string S, optional Name Type )
{
	if (Type == '')
		Type = 'Event';

	TeamMessage(PlayerReplicationInfo, S, Type);
}

event TeamMessage( PlayerReplicationInfo PRI, coerce string S, name Type, optional sound MessageSound )
{
    if ( ((Type == 'Say') || (Type == 'TeamSay')) && (PRI != None) )
		S = PRI.PlayerName$": "$S;

    Player.Console.Message( S, 6.0 );
}

simulated function PlayBeepSound();

simulated function PlayAnnouncement(sound ASound, byte AnnouncementLevel, optional bool bForce)
{	
	local float Atten;
	
	if ( AnnouncementLevel > AnnouncerLevel )
		return;
	if ( !bForce && (Level.TimeSeconds - LastPlaySound < 1) )
		return;
	LastPlaySound = Level.TimeSeconds;	// so voice messages won't overlap
	LastPlaySpeech = Level.TimeSeconds;	// don't want chatter to overlap announcements
	
	Atten = FClamp(0.1 + float(AnnouncerVolume)*0.225,0.2,1.0);
	ClientPlaySound(ASound,true,Atten,SLOT_Talk);
}

function bool AllowVoiceMessage(name MessageType)
{
	if ( Level.NetMode == NM_Standalone )
		return true;
		
	if ( Level.TimeSeconds - OldMessageTime < 3 )
	{
		if ( (MessageType == 'TAUNT') || (MessageType == 'AUTOTAUNT') )
			return false;
		if ( Level.TimeSeconds - OldMessageTime < 1 )
			return false;
	}
	if ( Level.TimeSeconds - OldMessageTime < 6 )
		OldMessageTime = Level.TimeSeconds + 3;
	else
		OldMessageTime = Level.TimeSeconds;
	return true;
}
	
//Play a sound client side (so only client will hear it
simulated function ClientPlaySound(sound ASound, optional bool bVolumeControl, optional float inAtten, optional ESoundSlot slot )
{
    local float atten;

    atten = 0.9;
    if( bVolumeControl )
        atten = FClamp(inAtten,0,1);

	if ( ViewTarget != None )
		ViewTarget.PlaySound(ASound, slot, atten,,,,false);
}

simulated function ClientReliablePlaySound(sound ASound, optional bool bVolumeControl )
{
	ClientPlaySound(ASound, bVolumeControl);
}

simulated function NotifyRocketLockOnA()
{
	if (RocketLockSound? && ViewTarget?)
		ViewTarget.PlayOwnedSound(RocketLockSound,SLOT_Interface,5.0,,,1.3);
	ContinueNotifyRocketLockOnA();
}

simulated function NotifyRocketLockOnB()
{
	if (RocketLockSound? && ViewTarget?)
		ViewTarget.PlayOwnedSound(RocketLockSound,SLOT_Interface,5.0,,,1.2);
	ContinueNotifyRocketLockOnB();
}

simulated function NotifyRocketLockOnC()
{
	if (RocketLockSound? && ViewTarget?)
		ViewTarget.PlayOwnedSound(RocketLockSound,SLOT_Interface,5.0,,,1.1);
	ContinueNotifyRocketLockOnC();
}

simulated function NotifyRocketLockOnD()
{
	if (RocketLockSound? && ViewTarget?)
		ViewTarget.PlayOwnedSound(RocketLockSound,SLOT_Interface,5.0,,,1.0);
	ContinueNotifyRocketLockOnD();
}

simulated function ContinueNotifyRocketLockOnA(){ SendEvent("RocketNotifyA"); }
simulated function ContinueNotifyRocketLockOnB(){ SendEvent("RocketNotifyB"); }
simulated function ContinueNotifyRocketLockOnC(){ SendEvent("RocketNotifyC"); }
simulated function ContinueNotifyRocketLockOnD(){ SendEvent("RocketNotifyD"); }

simulated event Destroyed()
{
	local SavedMove Next;
    local StationaryTurret DrivenTurret;
    local SVehicle DrivenSVehicle;

	// cheatmanager, adminmanager, and playerinput cleaned up in C++ PostScriptDestroyed()
 
    StopFeedbackEffect();
 
	if ( Pawn != None )
	{
		// If its a vehicle or turret, just destroy the driver, otherwise do the normal.
        DrivenTurret = StationaryTurret(Pawn);
		if(DrivenVehicle != None)
		{	
			DrivenVehicle.KDriverLeave(PassengerNumber, true); // Force the driver out of the car
		}
		else if(DrivenTurret != None)
		{
		DrivenTurret.KDriverLeave(true);
		}
		else
		{
			DrivenSVehicle = SVehicle(Pawn);
			if(DrivenSVehicle != None)
			{	
				DrivenSVehicle.KDriverLeave(true); // Force the driver out of the car
			}
        }
		Pawn.Health = 0;
		Pawn.Unpossessed();
		Pawn.Died( self, class'Suicided', Pawn.Location );
	}


/*!!MERGE
	if(HUD?)
		class'UIConsole'.static.DestroyComponent(HUD);
	if(Scoreboard?)
		class'UIConsole'.static.DestroyComponent(Scoreboard);
*/

	while ( FreeMoves != None )
	{
		Next = FreeMoves.NextMove;
		FreeMoves.Destroy();
		FreeMoves = Next;
	}
	while ( SavedMoves != None )
	{
		Next = SavedMoves.NextMove;
		SavedMoves.Destroy();
		SavedMoves = Next;
	}
    
    if( PlayerSecurity != None )
    {
        PlayerSecurity.Destroy();
        PlayerSecurity = None;
    }

	//NEW (mdf)
	if( CheatManager != None )
		CheatManager.Cleanup();
	//OLD

	if( UseProjector != None )
	{
		UseProjector.Destroy();
		UseProjector = None;
	}
	
	RemoveAllTimers();
    Super.Destroyed();
}

function ClientSetMusic( string NewSong, EMusicTransition NewTransition )
{
	StopAllMusic( 0.0 );
	PlayMusic( NewSong, 3.0 );
	Song        = NewSong;
	Transition  = NewTransition;
}

function interface PlayLoadingMusic()
{
	ClientSetMusic( LoadingMusic[ Rand(LoadingMusic.length) ], MTRAN_Fade );
	ConsoleCommand("LOCKMUSIC");
}

function interface StopLoadingMusic()
{
	ConsoleCommand("UNLOCKMUSIC");
	StopAllMusic( 3.0 );
}

// ------------------------------------------------------------------------
// Zooming/FOV change functions

function ToggleZoom()
{
	if ( DefaultFOV != DesiredFOV )
		EndZoom();
	else
		StartZoom();
}
	
function StartZoom()
{
	ZoomLevel = 0.0;
	bZooming = true;
}

function StopZoom()
{
	bZooming = false;
}

function EndZoom()
{
	bZooming = false;
	DesiredFOV = DefaultFOV;
}

function FixFOV()
{
	FOVAngle = Default.DefaultFOV;
	DesiredFOV = Default.DefaultFOV;
	DefaultFOV = Default.DefaultFOV;
}

function SetFOV(float NewFOV)
{
	DesiredFOV = NewFOV;
	FOVAngle = NewFOV;
}

function ResetFOV()
{
	DesiredFOV = DefaultFOV;
	FOVAngle = DefaultFOV;
}

exec function FOV(float F)
{
	if( (F >= 80.0) || (Level.Netmode==NM_Standalone) )
	{
		DefaultFOV = FClamp(F, 1, 170);
		DesiredFOV = DefaultFOV;
		SaveConfig();
	}
}

exec function Mutate(string MutateString)
{
	if( Level.NetMode == NM_Client )
		return;
	Level.Game.BaseMutator.Mutate(MutateString, Self);
}

exec function SetSensitivity(float F)
{
	PlayerInput.UpdateSensitivity(F);
}

exec function SetMouseSmoothing( int Mode )
{
    PlayerInput.UpdateSmoothing( Mode );
}

exec function SetMouseAccel(float F)
{
	PlayerInput.UpdateAccel(F);
}

exec function TraceForMemStats()
{
	local actor HitActor;
	local vector HitLocation, HitNormal;

	HitActor = Trace(HitLocation, HitNormal, pawn.location + (vect(0,0,1) * pawn.baseEyeHeight) + 10000.0 * vector(pawn.getViewRotation()), pawn.location + (vect(0,0,1) * pawn.baseEyeHeight), true);
	log("hit Actor = "$HitActor);

	if(HitActor != NONE)
	{
		log("class="$HitActor.Class$" name="$HitActor.name);
		consoleCommand("stat setAsset class="$HitActor.Class$" name="$HitActor.name);
	}
}

// ------------------------------------------------------------------------
// Messaging functions

// Send a message to all players.
exec function Say( string Msg )
{
	if (PlayerReplicationInfo.bAdmin && left(Msg,1) == "#" )
	{
		Level.Game.AdminSay(right(Msg,len(Msg)-1));
		return;
	}
	Level.Game.Broadcast(self, Msg, 'Say');
}

exec function TeamSay( string Msg )
{
	if( !GameReplicationInfo.bTeamGame )
	{
		Say( Msg );
		return;
	}

    Level.Game.BroadcastTeam( self, Level.Game.ParseMessageString( Level.Game.BaseMutator , self, Msg ) , 'TeamSay');
}

// Revised interface to send UI events
native final function SendEvent( string Event );
final function ClientSendEvent( byte EventID )
{
	switch(EventID)	// Fix ARL: Use better pattern to keep these in sync (enums, tables, LocalMessage or something).
	{
	//case 0: SendEvent("HealthFlash"); break;
	//case 1: SendEvent("PainFlash"); break;
	case 0:  SendEvent("HealthFlash"); SendEvent("PainFlash"); break;
	case 1:  SendEvent("ShieldFlash"); break;
	case 2:  SendEvent("CanEnterLimboOn"); break;
	case 3:  SendEvent("ShowDeployPointOff"); break;
	case 4:  SendEvent("ShowDeployPointOn"); break;
	case 5:  SendEvent("SetDeployPointOff"); SendEvent("ShowDeployPointOn"); break;
	case 6:  SendEvent("SetDeployPointOn");  SendEvent("ShowDeployPointOff"); break;
	case 7:  SendEvent("RedTeamWins"); break;
	case 8:  SendEvent("BlueTeamWins"); break;
	case 9:  SendEvent("RedTeamLoses"); break;
	case 10: SendEvent("BlueTeamLoses"); break;
	case 11: SendEvent("GameTimeUp"); break;
	case 12: SendEvent("ShowPassengers"); break;
	case 13: SendEvent("HidePassengers"); break;
	case 14: SendEvent("LevelLoadStart"); break;
	default: warn("Unknown EventID:"@EventID); break;
	};
}

function bool IsDead()
{
	return false;
}

function HandleDying()
{
	GotoState( 'Wounded' );
}

function HandleRevived()
{
	GotoState( 'PlayerWalking' );
}

function HandleMercyKilled()
{
	GotoState( 'Dead' );
}

event PreClientTravel()
{
    log("PreClientTravel");
    ClientStopForceFeedback();  // jdf
}

function ClientSetFixedCamera(bool B)
{
	bFixedCamera = B;
}

function ClientSetBehindView(bool B)
{
	bBehindView = B;
}

function ClientVoiceMessage(PlayerReplicationInfo Sender, PlayerReplicationInfo Recipient, name messagetype, byte messageID)
{
	local VoicePack V;

	if ( (Sender == None) || (Sender.voicetype == None) || (Player.Console == None) )
		return;
		
	V = Spawn(Sender.voicetype, self);
	if ( V != None )
		V.ClientInitialize(Sender, Recipient, messagetype, messageID);
}

/* ForceDeathUpdate()
Make sure ClientAdjustPosition immediately informs client of pawn's death
*/
function ForceDeathUpdate()
{
	LastUpdateTime = Level.TimeSeconds - 10;
}

/* ShortServerMove()
compressed version of server move for bandwidth saving
*/
function ShortServerMove
(
	float TimeStamp,
	vector ClientLoc,
	bool NewbRun,
	bool NewbSprint,
	bool NewbDuck,
	bool NewbJump,
	bool NewbJumpStatus, 
	byte ClientRoll,
	int View
)
{
    ServerMove(TimeStamp,vect(0,0,0),ClientLoc,NewbRun,NewbSprint,NewbDuck,NewbJump,NewbJumpStatus,false,DCLICK_None,ClientRoll,View);
}

/* ServerMove() 
- replicated function sent by client to server - contains client movement and firing info
Passes acceleration in components so it doesn't get rounded.
*/
function ServerMove
(
	float TimeStamp, 
	vector InAccel, 
	vector ClientLoc,
	bool NewbRun,
	bool NewbSprint,
	bool NewbDuck,
	bool NewbJump, 
	bool NewbJumpStatus, 
	bool NewbDoubleJump,
	eDoubleClickDir DoubleClickMove, 
	byte ClientRoll, 
	int View,
	optional byte OldTimeDelta,
	optional int OldAccel
)
{
	local float DeltaTime, clientErr, OldTimeStamp;
	local rotator DeltaRot, Rot, ViewRot;
	local vector Accel, LocDiff, ClientVel, ClientFloor;
	local int maxPitch, ViewPitch, ViewYaw;
	local bool NewbPressedJump, OldbRun, OldbSprint, OldbDoubleJump;
	local eDoubleClickDir OldDoubleClickMove;
	local actor ClientBase;
	local ePhysics ClientPhysics;

	// If this move is outdated, discard it.
	if ( CurrentTimeStamp >= TimeStamp )
		return;

	bShortConnectTimeOut = true;

	// if OldTimeDelta corresponds to a lost packet, process it first
	if (  OldTimeDelta != 0 )
	{
		OldTimeStamp = TimeStamp - float(OldTimeDelta)/500 - 0.001;
		if ( CurrentTimeStamp < OldTimeStamp - 0.001 )
		{
			// split out components of lost move (approx)
			Accel.X = OldAccel >>> 23;
			if ( Accel.X > 127 )
				Accel.X = -1 * (Accel.X - 128);
			Accel.Y = (OldAccel >>> 15) & 255;
			if ( Accel.Y > 127 )
				Accel.Y = -1 * (Accel.Y - 128);
			Accel.Z = (OldAccel >>> 7) & 255;
			if ( Accel.Z > 127 )
				Accel.Z = -1 * (Accel.Z - 128);
			Accel *= 20;
			
			OldbRun = ( (OldAccel & 128) != 0 );
			OldbSprint = ( (OldAccel & 64) != 0 );
			OldbDoubleJump = ( (OldAccel & 32) != 0 );
			NewbPressedJump = ( (OldAccel & 16) != 0 );
			if ( NewbPressedJump )
				bJumpStatus = NewbJumpStatus;
			switch (OldAccel & 7)
			{
				case 0:
					OldDoubleClickMove = DCLICK_None;
					break;
				case 1:
					OldDoubleClickMove = DCLICK_Left;
					break;
				case 2:
					OldDoubleClickMove = DCLICK_Right;
					break;
				case 3:
					OldDoubleClickMove = DCLICK_Forward;
					break;
				case 4:
					OldDoubleClickMove = DCLICK_Back;
					break;
			}
			//log("Recovered move from "$OldTimeStamp$" acceleration "$Accel$" from "$OldAccel);
			OldTimeStamp = FMin(OldTimeStamp, CurrentTimeStamp + MaxResponseTime);
			MoveAutonomous(OldTimeStamp - CurrentTimeStamp, OldbRun, OldbSprint, (bDuck == 1), (bJump == 1), NewbPressedJump, OldbDoubleJump, OldDoubleClickMove, Accel, rot(0,0,0));
			CurrentTimeStamp = OldTimeStamp;
		}
	}		

	// View components
	ViewPitch = View/32768;
	ViewYaw = 2 * (View - 32768 * ViewPitch);
	ViewPitch *= 2;
	// Make acceleration.
	Accel = InAccel/10;

	NewbPressedJump = (bJumpStatus != NewbJumpStatus);
	bJumpStatus = NewbJumpStatus;

	// Save move parameters.
    DeltaTime = FMin(MaxResponseTime,TimeStamp - CurrentTimeStamp);

	if ( ServerTimeStamp > 0 )
	{
		// allow 1% error
        TimeMargin = FMax(0,TimeMargin + DeltaTime - 1.01 * (Level.TimeSeconds - ServerTimeStamp));
		if ( TimeMargin > MaxTimeMargin )
		{
			// player is too far ahead
			TimeMargin -= DeltaTime;
			if ( TimeMargin < 0.5 )
				MaxTimeMargin = Default.MaxTimeMargin;
			else
				MaxTimeMargin = 0.5;
			DeltaTime = 0;
		}
	}

	CurrentTimeStamp = TimeStamp;
	ServerTimeStamp = Level.TimeSeconds;
	ViewRot.Pitch = ViewPitch;
	ViewRot.Yaw = ViewYaw;
	ViewRot.Roll = 0;
	SetRotation(ViewRot);

	if ( Pawn != None && Pawn != DrivenVehicle)
	{
		Rot.Roll = 256 * ClientRoll;
		Rot.Yaw = ViewYaw;
		if ( (Pawn.Physics == PHYS_Swimming) || (Pawn.Physics == PHYS_Flying) )
			maxPitch = 2;
		else
            maxPitch = 0;
		If ( (ViewPitch > maxPitch * RotationRate.Pitch) && (ViewPitch < 65536 - maxPitch * RotationRate.Pitch) )
		{
			If (ViewPitch < 32768) 
				Rot.Pitch = maxPitch * RotationRate.Pitch;
			else
				Rot.Pitch = 65536 - maxPitch * RotationRate.Pitch;
		}
		else
			Rot.Pitch = ViewPitch;
		DeltaRot = (Rotation - Rot);
		Pawn.SetRotation(Rot);
	}

	// Perform actual movement
	if ( (Level.Pauser == None) && (DeltaTime > 0) )
        MoveAutonomous(DeltaTime, NewbRun, NewbSprint, NewbDuck, NewbJump, NewbPressedJump, NewbDoubleJump, DoubleClickMove, Accel, DeltaRot);

	// Accumulate movement error.
	if ( Level.TimeSeconds - LastUpdateTime > 0.3 )
		ClientErr = 10000;
	else if ( Level.TimeSeconds - LastUpdateTime > 180.0/Player.CurrentNetSpeed )
	{
		if ( Pawn == None )
			LocDiff = Location - ClientLoc;
		else
			LocDiff = Pawn.Location - ClientLoc;
		ClientErr = LocDiff Dot LocDiff;
	}

	// If client has accumulated a noticeable positional error, correct him.
	if ( ClientErr > 3 )
	{
		if ( Pawn == None )
		{
			ClientPhysics = Physics;
			ClientLoc = Location;
			ClientVel = Velocity;
		}
		else
		{
			ClientPhysics = Pawn.Physics;
			ClientVel = Pawn.Velocity;
			ClientBase = Pawn.Base;
			if ( Mover(Pawn.Base) != None )
				ClientLoc = Pawn.Location - Pawn.Base.Location;
			else
				ClientLoc = Pawn.Location;
			ClientFloor = Pawn.Floor;
		}
		//	log(Level.TimeSeconds$" Client Error at "$TimeStamp$" is "$ClientErr$" with acceleration "$Accel$" LocDiff "$LocDiff$" Physics "$Pawn.Physics);
		LastUpdateTime = Level.TimeSeconds;

		if ( (Pawn == None) || (Pawn.Physics != PHYS_Spider) )
		{
			if ( ClientVel == vect(0,0,0) )
			{
					ShortClientAdjustPosition
					(
						TimeStamp, 
						GetStateName(), 
						ClientPhysics, 
						ClientLoc.X, 
						ClientLoc.Y, 
						ClientLoc.Z, 
						ClientBase
					);
			}
			else
				ClientAdjustPosition
				(
					TimeStamp, 
					GetStateName(), 
					ClientPhysics, 
					ClientLoc.X, 
					ClientLoc.Y, 
					ClientLoc.Z, 
					ClientVel.X, 
					ClientVel.Y, 
					ClientVel.Z,
					ClientBase
				);
		}
		else
			LongClientAdjustPosition
			(
				TimeStamp, 
				GetStateName(), 
				ClientPhysics, 
				ClientLoc.X, 
				ClientLoc.Y, 
				ClientLoc.Z, 
				ClientVel.X, 
				ClientVel.Y, 
				ClientVel.Z,
				ClientBase,
				ClientFloor.X,
				ClientFloor.Y,
				ClientFloor.Z
			);
	}
	//log("Server moved stamp "$TimeStamp$" location "$Pawn.Location$" Acceleration "$Pawn.Acceleration$" Velocity "$Pawn.Velocity);
}	

// Only executed on server
function ServerDrive(float InForward, float InStrafe, bool InJump, rotator DeltaRot)
{
	if ( (IsInState('PlayerDriving')) || (IsInState('PlayerTurreting')) )
	ProcessDrive(InForward, InStrafe, InJump, DeltaRot);
}

function ProcessDrive(float InForward, float InStrafe, bool InJump, rotator DeltaRot)
{
	Log("ProcessDrive Not Valid Outside State PlayerDriving or State PlayerTurreting");
}

function ProcessMove ( float DeltaTime, vector newAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)
{
	if ( Pawn != None )
		Pawn.Acceleration = newAccel;
}

final function MoveAutonomous
(	
	float DeltaTime, 	
	bool NewbRun,
	bool NewbSprint,
	bool NewbDuck,
	bool NewbJump,
	bool NewbPressedJump, 
    bool NewbDoubleJump,
	eDoubleClickDir DoubleClickMove, 
	vector newAccel, 
	rotator DeltaRot
)
{
	if ( NewbRun )
		bRun = 1;
	else
		bRun = 0;

	if ( NewbSprint )
		bSprint = 1;
	else
		bSprint = 0;

	if ( NewbDuck )
		bDuck = 1;
	else
		bDuck = 0;

	if ( NewbJump )
		bJump = 1;
	else
		bJump = 0;

	bPressedJump = NewbPressedJump;
	bDoubleJump = NewbDoubleJump;
	HandleWalking();
	ProcessMove(DeltaTime, newAccel, DoubleClickMove, DeltaRot);
	if ( Pawn != None )	
		Pawn.AutonomousPhysics(DeltaTime);
	else
		AutonomousPhysics(DeltaTime);
	bDoubleJump = false;
	//log("Role "$Role$" moveauto time "$100 * DeltaTime$" ("$Level.TimeDilation$")");
}

/* VeryShortClientAdjustPosition
bandwidth saving version, when velocity is zeroed, and pawn is walking
*/
function VeryShortClientAdjustPosition
(
	float TimeStamp, 
	float NewLocX, 
	float NewLocY, 
	float NewLocZ, 
	Actor NewBase
)
{
	local vector Floor;

	if ( Pawn != None )
		Floor = Pawn.Floor;
	LongClientAdjustPosition(TimeStamp,'PlayerWalking',PHYS_Walking,NewLocX,NewLocY,NewLocZ,0,0,0,NewBase,Floor.X,Floor.Y,Floor.Z);
}

/* ShortClientAdjustPosition
bandwidth saving version, when velocity is zeroed
*/
function ShortClientAdjustPosition
(
	float TimeStamp, 
	name newState, 
	EPhysics newPhysics,
	float NewLocX, 
	float NewLocY, 
	float NewLocZ, 
	Actor NewBase
)
{
	local vector Floor;

	if ( Pawn != None )
		Floor = Pawn.Floor;
	LongClientAdjustPosition(TimeStamp,newState,newPhysics,NewLocX,NewLocY,NewLocZ,0,0,0,NewBase,Floor.X,Floor.Y,Floor.Z);
}

/* ClientAdjustPosition
- pass newloc and newvel in components so they don't get rounded
*/
function ClientAdjustPosition
(
	float TimeStamp, 
	name newState, 
	EPhysics newPhysics,
	float NewLocX, 
	float NewLocY, 
	float NewLocZ, 
	float NewVelX, 
	float NewVelY, 
	float NewVelZ,
	Actor NewBase
)
{
	local vector Floor;

	if ( Pawn != None )
		Floor = Pawn.Floor;
	LongClientAdjustPosition(TimeStamp,newState,newPhysics,NewLocX,NewLocY,NewLocZ,NewVelX,NewVelY,NewVelZ,NewBase,Floor.X,Floor.Y,Floor.Z);
}

/* LongClientAdjustPosition 
long version, when care about pawn's floor normal
*/
function LongClientAdjustPosition
(
	float TimeStamp, 
	name newState, 
	EPhysics newPhysics,
	float NewLocX, 
	float NewLocY, 
	float NewLocZ, 
	float NewVelX, 
	float NewVelY, 
	float NewVelZ,
	Actor NewBase,
	float NewFloorX,
	float NewFloorY,
	float NewFloorZ
)
{
    local vector NewLocation, NewVelocity, NewFloor;
	local Actor MoveActor;
    local SavedMove CurrentMove;

	// update ping
	if ( (PlayerReplicationInfo != None) && !bDemoOwner )
	{
		if ( ExactPing < 0.006 )
			ExactPing = Level.TimeSeconds - TimeStamp;
		else
			ExactPing = 0.99 * ExactPing + 0.008 * (Level.TimeSeconds - TimeStamp); // placebo effect
		PlayerReplicationInfo.Ping = 1000 * ExactPing;

		if ( Level.TimeSeconds - LastPingUpdate > 4 )
		{
			if ( bDynamicNetSpeed && (OldPing > DynamicPingThreshold * 0.001) && (ExactPing > DynamicPingThreshold * 0.001) )
			{
				if ( Player.CurrentNetSpeed > 5000 )
					SetNetSpeed(5000);
				else if ( Level.MoveRepSize < 80 )
					Level.MoveRepSize += 8;
				else if ( Player.CurrentNetSpeed > 4000 )
					SetNetSpeed(4000);
				OldPing = 0;
			}
			else
				OldPing = ExactPing;
			LastPingUpdate = Level.TimeSeconds;
			ServerUpdatePing(1000 * ExactPing);
		}
	}
	if (NewState == 'PlayerDriving' || NewState == 'PlayerTurreting')
		GotoState(NewState);
		
	if ( Pawn != None )
	{
		if ( Pawn.bTearOff )
		{
			Pawn = None;
			if ( !IsInState('GameEnded') && !IsInState('Dead') )
			{
			GotoState('Dead');
            }
			return;
		}
		MoveActor = Pawn;
        if ( (ViewTarget != Pawn)
			&& ((ViewTarget == self) || ((Pawn(ViewTarget) != None) && (Pawn(ViewTarget).Health <= 0))) )
		{
			bBehindView = false;
			SetViewTarget(Pawn);
		}
	}
	else 
    {
		MoveActor = self;
 	   	if( GetStateName() != newstate )
		{
		    if ( NewState == 'GameEnded' )
			    GotoState(NewState);
			else if ( IsInState('Dead') )
			{
		    	if ( (NewState != 'PlayerWalking') && (NewState != 'PlayerSwimming') )
		        {
				    GotoState(NewState);
		        }
		        return;
			}
			else if ( NewState == 'Dead' )
				GotoState(NewState);
		}
	}
    if ( CurrentTimeStamp >= TimeStamp )
		return;
	CurrentTimeStamp = TimeStamp;

	NewLocation.X = NewLocX;
	NewLocation.Y = NewLocY;
	NewLocation.Z = NewLocZ;
    NewVelocity.X = NewVelX;
    NewVelocity.Y = NewVelY;
    NewVelocity.Z = NewVelZ;

	// skip update if no error
    CurrentMove = SavedMoves;
    while ( CurrentMove != None )
    {
        if ( CurrentMove.TimeStamp <= CurrentTimeStamp )
        {
            SavedMoves = CurrentMove.NextMove;
            CurrentMove.NextMove = FreeMoves;
            FreeMoves = CurrentMove;
			if ( CurrentMove.TimeStamp == CurrentTimeStamp )
			{ 
				FreeMoves.Clear();
				if ( (VSize(CurrentMove.SavedLocation - NewLocation) < 3)
					&& (VSize(CurrentMove.SavedVelocity - NewVelocity) < 3)
					&& (GetStateName() == NewState)
					&& ((MoveActor.Physics != PHYS_Flying) || !IsInState('PlayerWalking')) )
				{
					return;
				}
				CurrentMove = None;
			}
			else
			{
				FreeMoves.Clear();
				CurrentMove = SavedMoves;
			}
        }
        else
			CurrentMove = None;
    }

	NewFloor.X = NewFloorX;
	NewFloor.Y = NewFloorY;
	NewFloor.Z = NewFloorZ;
	if (!(ClassIsChildOf( MoveActor.Class, class'XMPVehicleBarrel' ) || ClassIsChildOf( MoveActor.Class, class'KVehicle' ) || ClassIsChildOf( MoveActor.Class, class'StationaryTurret' )))
	{
	MoveActor.SetBase(NewBase, NewFloor);
	if ( Mover(NewBase) != None )
		NewLocation += NewBase.Location;

	if ( !bDemoOwner )
	{
	//log("Client "$Role$" adjust "$self$" stamp "$TimeStamp$" location "$MoveActor.Location);
	MoveActor.bCanTeleport = false;
    if ( !MoveActor.SetLocation(NewLocation) && (Pawn(MoveActor) != None)
		&& (MoveActor.CollisionHeight > Pawn(MoveActor).CrouchHeight)
		&& !Pawn(MoveActor).bIsCrouched
		&& (newPhysics == PHYS_Walking)
		&& (MoveActor.Physics != PHYS_Karma) && (MoveActor.Physics != PHYS_KarmaRagDoll) )
	{
		MoveActor.SetPhysics(newPhysics);
		Pawn(MoveActor).ForceCrouch();
	MoveActor.SetLocation(NewLocation);
	}
	MoveActor.bCanTeleport = true;
	}
	// Hack. Don't let network change physics mode of karma stuff on the client.
	if( MoveActor.Physics != PHYS_Karma && MoveActor.Physics != PHYS_KarmaRagDoll &&
		newPhysics != PHYS_Karma && newPhysics != PHYS_KarmaRagDoll )
	{
		MoveActor.SetPhysics(newPhysics);
	}

    MoveActor.Velocity = NewVelocity;
	}
	if( GetStateName() != newstate )
		GotoState(newstate);

	bUpdatePosition = true;
}

function ServerUpdatePing(int NewPing)
{
	PlayerReplicationInfo.Ping = NewPing;
	PlayerReplicationInfo.bReceivedPing = true;
}

function ClientUpdatePosition()
{
	local SavedMove CurrentMove;
	local int realbRun, realbSprint, realbDuck, realbJump;
	local bool bRealJump;

	// Dont do any network position updates on things running PHYS_Karma
	if( Pawn != None && (Pawn.Physics == PHYS_Karma || Pawn.Physics == PHYS_KarmaRagDoll) )
		return;

	bUpdatePosition = false;
	realbRun= bRun;
	realbSprint = bSprint;
	realbDuck = bDuck;
	realbJump = bJump;
	bRealJump = bPressedJump;
	CurrentMove = SavedMoves;
	bUpdating = true;
	while ( CurrentMove != None )
	{
		if ( CurrentMove.TimeStamp <= CurrentTimeStamp )
		{
			SavedMoves = CurrentMove.NextMove;
			CurrentMove.NextMove = FreeMoves;
			FreeMoves = CurrentMove;
			FreeMoves.Clear();
			CurrentMove = SavedMoves;
		}
		else
		{
            MoveAutonomous(CurrentMove.Delta, CurrentMove.bRun, CurrentMove.bSprint, CurrentMove.bDuck, CurrentMove.bJump, CurrentMove.bPressedJump, CurrentMove.bDoubleJump, CurrentMove.DoubleClickMove, CurrentMove.Acceleration, rot(0,0,0));
			CurrentMove = CurrentMove.NextMove;
		}
	}
    if ( PendingMove != None )
        MoveAutonomous(PendingMove.Delta, PendingMove.bRun, PendingMove.bSprint, PendingMove.bDuck, PendingMove.bJump, PendingMove.bPressedJump, PendingMove.bDoubleJump, PendingMove.DoubleClickMove, PendingMove.Acceleration, rot(0,0,0));
    
	//log("Client updated position to "$Pawn.Location);
	bUpdating = false;
	bJump = realbJump;
	bDuck = realbDuck;
	bSprint = realbSprint;
	bRun = realbRun;
	bPressedJump = bRealJump;
}

final function SavedMove GetFreeMove()
{
	local SavedMove s, first;
	local int i;

	if ( FreeMoves == None )
	{
        // don't allow more than 100 saved moves
		For ( s=SavedMoves; s!=None; s=s.NextMove )
		{
			i++;
            if ( i > 100 )
			{
				first = SavedMoves;
				SavedMoves = SavedMoves.NextMove;
				first.Clear();
				first.NextMove = None;
				// clear out all the moves
				While ( SavedMoves != None )
				{
					s = SavedMoves;
					SavedMoves = SavedMoves.NextMove;
					s.Clear();
					s.NextMove = FreeMoves;
					FreeMoves = s;
				}
				return first;
			}
		}
		return Spawn(class'SavedMove');
	}
	else
	{
		s = FreeMoves;
		FreeMoves = FreeMoves.NextMove;
		s.NextMove = None;
		return s;
	}	
}

function int CompressAccel(int C)
{
	if ( C >= 0 )
		C = Min(C, 127);
	else
		C = Min(abs(C), 127) + 128;
	return C;
}

/* 
========================================================================
Here's how player movement prediction, replication and correction works in network games:

Every tick, the PlayerTick() function is called.  It calls the PlayerMove() function (which is implemented 
in various states).  PlayerMove() figures out the acceleration and rotation, and then calls ProcessMove() 
(for single player or listen servers), or ReplicateMove() (if its a network client).

ReplicateMove() saves the move (in the PendingMove list), calls ProcessMove(), and then replicates the move 
to the server by calling the replicated function ServerMove() - passing the movement parameters, the client's 
resultant position, and a timestamp.

ServerMove() is executed on the server.  It decodes the movement parameters and causes the appropriate movement 
to occur.  It then looks at the resulting position and if enough time has passed since the last response, or the 
position error is significant enough, the server calls ClientAdjustPosition(), a replicated function.

ClientAdjustPosition() is executed on the client.  The client sets its position to the servers version of position, 
and sets the bUpdatePosition flag to true.  

When PlayerTick() is called on the client again, if bUpdatePosition is true, the client will call 
ClientUpdatePosition() before calling PlayerMove().  ClientUpdatePosition() replays all the moves in the pending 
move list which occured after the timestamp of the move the server was adjusting.
*/

//
// Replicate this client's desired movement to the server.
//
function ReplicateMove
(
	float DeltaTime, 
	vector NewAccel, 
	eDoubleClickDir DoubleClickMove, 
	rotator DeltaRot
)
{
	local SavedMove NewMove, OldMove, LastMove;
	local byte ClientRoll;
	local float OldTimeDelta, NetMoveDelta;
	local int OldAccel;
	local vector BuildAccel, AccelNorm, MoveLoc;

	// find the most recent move, and the most recent interesting move
	if ( SavedMoves != None )
	{
        LastMove = SavedMoves;
		AccelNorm = Normal(NewAccel);
        while ( LastMove.NextMove != None )
		{
			// find most recent interesting move to send redundantly
            if ( LastMove.bPressedJump || LastMove.bDoubleJump || ((LastMove.DoubleClickMove != DCLICK_NONE) && (LastMove.DoubleClickMove < 5))
                || ((LastMove.Acceleration != NewAccel) && ((normal(LastMove.Acceleration) Dot AccelNorm) < 0.95)) )
                OldMove = LastMove;
            LastMove = LastMove.NextMove;
		}
        if ( LastMove.bPressedJump || LastMove.bDoubleJump || ((LastMove.DoubleClickMove != DCLICK_NONE) && (LastMove.DoubleClickMove < 5))
            || ((LastMove.Acceleration != NewAccel) && ((normal(LastMove.Acceleration) Dot AccelNorm) < 0.95)) )
            OldMove = LastMove;
	}
    // Get a SavedMove actor to store the movement in.
    if ( PendingMove != None )
        PendingMove.SetMoveFor(self, DeltaTime, NewAccel, DoubleClickMove);

	NewMove = GetFreeMove();
	if ( NewMove == None )
		return;
	NewMove.SetMoveFor(self, DeltaTime, NewAccel, DoubleClickMove);
	
	// Simulate the movement locally.
    bDoubleJump = false;
	ProcessMove(NewMove.Delta, NewMove.Acceleration, NewMove.DoubleClickMove, DeltaRot);
    
	if ( Pawn != None )
		Pawn.AutonomousPhysics(NewMove.Delta);
	else
		AutonomousPhysics(DeltaTime);

	if ( PendingMove == None )
		PendingMove = NewMove;
	else
	{
		NewMove.NextMove = FreeMoves;
		FreeMoves = NewMove;
		FreeMoves.Clear();
		NewMove = PendingMove;
	}
    NewMove.PostUpdate(self);
    NetMoveDelta = FMax(80.0/Player.CurrentNetSpeed, 0.015);
	
    // Decide whether to hold off on move
    // send if double click move, jump, or fire unless really too soon, or if newmove.delta big enough
    if ( !Level.bCapFramerate && !PendingMove.bPressedJump && !PendingMove.bDoubleJump 
		&& ((PendingMove.DoubleClickMove == DCLICK_None) || (PendingMove.DoubleClickMove == DCLICK_Active))
		&& ((PendingMove.Acceleration == NewAccel) || ((Normal(NewAccel) Dot Normal(PendingMove.Acceleration)) > 0.95)) 
		&& (PendingMove.Delta < NetMoveDelta - ClientUpdateTime) )
	{
		return;
	}
	else
	{
		ClientUpdateTime = PendingMove.Delta - NetMoveDelta;
		if ( SavedMoves == None )
			SavedMoves = PendingMove;
		else
			LastMove.NextMove = PendingMove;
		PendingMove = None;
	}

	// check if need to redundantly send previous move
	if ( OldMove != None )
	{
		// old move important to replicate redundantly
		OldTimeDelta = FMin(255, (Level.TimeSeconds - OldMove.TimeStamp) * 500);
		BuildAccel = 0.05 * OldMove.Acceleration + vect(0.5, 0.5, 0.5);
		OldAccel = (CompressAccel(BuildAccel.X) << 23) 
					+ (CompressAccel(BuildAccel.Y) << 15) 
					+ (CompressAccel(BuildAccel.Z) << 7);
		if ( OldMove.bRun )
			OldAccel += 128;
		if ( OldMove.bSprint )
			OldAccel += 64;
		if ( OldMove.bDoubleJump )
			OldAccel += 32;
		if ( OldMove.bPressedJump )
			OldAccel += 16;
		OldAccel += OldMove.DoubleClickMove;
	}

	// Send to the server
	ClientRoll = (Rotation.Roll >> 8) & 255;
	if ( NewMove.bPressedJump )
		bJumpStatus = !bJumpStatus;

	if ( Pawn == None )
		MoveLoc = Location;
	else
		MoveLoc = Pawn.Location;

    if ( (NewMove.Acceleration == vect(0,0,0)) && (NewMove.DoubleClickMove == DCLICK_None) && !NewMove.bDoubleJump )
		ShortServerMove
		(
			NewMove.TimeStamp, 
			MoveLoc, 
			NewMove.bRun,
			NewMove.bSprint,
			NewMove.bDuck,
			NewMove.bJump, 
			bJumpStatus, 
			ClientRoll,
			(32767 & (Rotation.Pitch/2)) * 32768 + (32767 & (Rotation.Yaw/2))
		);
	else
		ServerMove
		(
			NewMove.TimeStamp, 
			NewMove.Acceleration * 10, 
			MoveLoc, 
			NewMove.bRun,
			NewMove.bSprint,
			NewMove.bDuck,
			NewMove.bJump, 
			bJumpStatus, 
			NewMove.bDoubleJump,
			NewMove.DoubleClickMove, 
			ClientRoll,
			(32767 & (Rotation.Pitch/2)) * 32768 + (32767 & (Rotation.Yaw/2)),
			OldTimeDelta,
			OldAccel 
		);
}

function HandleWalking()
{
	if ( Pawn != None )
	{
		Pawn.SetWalking( (bRun != 0) && !Region.Zone.IsA('WarpZoneInfo') ); 
		Pawn.SetSprint( (bSprint != 0) ); 
	}
}

exec function ForceRestart()
{
	Level.Game.EndGame(None,"Forced");
}

exec function ServerQuit()
{
	ConsoleCommand("EXIT");
}

function ServerRestartGame()
{
}

function SetFOVAngle(float newFOV)
{
	FOVAngle = newFOV;
}
	 
function ClientFlash( float scale, vector fog )
{
    FlashScale = scale * vect(1,1,1);
    flashfog = 0.001 * fog;
}
   
function ClientAdjustGlow( float scale, vector fog )
{
	ConstantGlowScale += scale;
	ConstantGlowFog += 0.001 * fog;
}

function damageAttitudeTo(pawn Other, float Damage)
{
	if ( (Other != None) && (Other != Pawn) && (Damage > 0) )
		Enemy = Other;
}

function Typing( bool bTyping )
{
	bIsTyping = bTyping;
 	Pawn.bIsTyping = bIsTyping;
	if ( bTyping && (Pawn != None) && !Pawn.bTearOff )
		Pawn.ChangeAnimation();

}

//*************************************************************************************
// Normal gameplay execs
// Type the name of the exec function at the console to execute it

exec function Jump( optional float F )
{
	if ( Level.Pauser == PlayerReplicationInfo )
		SetPause(False);
	else
		bPressedJump = true;
}

// Send a voice message of a certain type to a certain player.
exec function Speech( name Type, int Index, string Callsign )
{
	if(PlayerReplicationInfo.VoiceType != None)
		PlayerReplicationInfo.VoiceType.static.PlayerSpeech( Type, Index, Callsign, Self );
}

exec function RestartLevel()
{
	if( Level.Netmode==NM_Standalone )
		ClientTravel( "?restart", TRAVEL_Relative, false );
}

exec function LocalTravel( string URL )
{
	if( Level.Netmode==NM_Standalone )
		ClientTravel( URL, TRAVEL_Relative, true );
}

// ------------------------------------------------------------------------
// Loading and saving

/* QuickSave()
Save game to slot 9
*/
exec function QuickSave()
{
	if ( (Pawn.Health > 0) 
		&& (Level.NetMode == NM_Standalone) )
	{
		ClientMessage(QuickSaveString);
		ConsoleCommand("SaveGame 9");
	}
}

/* QuickLoad()
Load game from slot 9
*/
exec function QuickLoad()
{
	if ( Level.NetMode == NM_Standalone )
		ClientTravel( "?load=9", TRAVEL_Absolute, false);
}

/* SetPause()
 Try to pause game; returns success indicator.
 Replicated to server in network games.
 */
function bool SetPause( BOOL bPause, optional int Key )
{
	bFire = 0;
	bAltFire = 0;
	return Level.Game.SetPause(bPause, self, Key);
}

/* Pause()
Command to try to pause the game.
*/
exec function Pause()
{
	if(Level.NetMode != NM_Standalone)
		return;

	// Pause if not already
	if(Level.Pauser == None)
		SetPause(true);
	else
		SetPause(false);
}

// Activate specific inventory item
exec function ActivateInventoryItem( class InvItem )
{
	local Powerups Inv;

	Inv = Powerups(Pawn.FindInventoryType(InvItem));
	if ( Inv != None )
		Inv.Activate();
}

// ------------------------------------------------------------------------
// Weapon changing functions

/* ThrowWeapon()
Throw out current weapon, and switch to a new weapon
*/
exec function ThrowWeapon()
{
    if ( (Pawn == None) || (Pawn.Weapon == None) )
		return;

    ServerThrowWeapon();
}

function ServerThrowWeapon()
{
    local Vector TossVel;

    if (Pawn.CanThrowWeapon())
    {
        TossVel = Vector(GetViewRotation());
        TossVel = TossVel * ((Pawn.Velocity Dot TossVel) + 500) + Vect(0,0,200);
        Pawn.TossWeapon(TossVel);
        ClientSwitchToBestWeapon();
    }
}

/* PrevWeapon()
- switch to previous inventory group weapon
*/
exec function PrevWeapon()
{
	if( Level.Pauser!=None )
		return;
	if ( Pawn.Weapon == None )
	{
		SwitchToBestWeapon();
		return;
	}
	if ( Pawn.PendingWeapon != None )
		Pawn.PendingWeapon = Pawn.Inventory.PrevWeapon(None, Pawn.PendingWeapon);
	else
		Pawn.PendingWeapon = Pawn.Inventory.PrevWeapon(None, Pawn.Weapon);

	if ( Pawn.PendingWeapon != None )
		Pawn.Weapon.PutDown();
}

/* NextWeapon()
- switch to next inventory group weapon
*/
exec function NextWeapon()
{
	if( Level.Pauser!=None )
		return;
	if ( Pawn.Weapon == None )
	{
		SwitchToBestWeapon();
		return;
	}
	if ( Pawn.PendingWeapon != None )
		Pawn.PendingWeapon = Pawn.Inventory.NextWeapon(None, Pawn.PendingWeapon);
	else
		Pawn.PendingWeapon = Pawn.Inventory.NextWeapon(None, Pawn.Weapon);

	if ( Pawn.PendingWeapon != None )
		Pawn.Weapon.PutDown();
}

exec function PipedSwitchWeapon(byte F)
{
	if ( (Pawn == None) || (Pawn.PendingWeapon != None) )
		return;

	SwitchWeapon(F);
}

// The player wants to switch to weapon group number F.
exec function SwitchWeapon (byte F )
{
	local weapon newWeapon;

	if ( (Level.Pauser!=None) || (Pawn == None) || (Pawn.Inventory == None) )
		return;

	newWeapon = Pawn.Inventory.WeaponChange(F,None);

	if ( newWeapon != None )
	{
		if ( Pawn.Weapon == None )
		{
			Pawn.PendingWeapon = newWeapon;
			Pawn.ChangedWeapon();
		}
		else if ( Pawn.Weapon != newWeapon ||
				  (Pawn.PendingWeapon != None && newWeapon != Pawn.PendingWeapon) )
		{
			Pawn.PendingWeapon = newWeapon;
			if ( !Pawn.Weapon.PutDown() )
				Pawn.PendingWeapon = None;
		}
	}

}

function bool ShouldAutoSwitch()
{
	return bAutoSwitch;
}

exec function GetWeapon(class<Weapon> NewWeaponClass )
{
	local Inventory Inv;
    local int Count;

	if ( (Pawn.Inventory == None) || (NewWeaponClass == None)
		|| ((Pawn.Weapon != None) && (Pawn.Weapon.Class == NewWeaponClass)) )
		return;

	for ( Inv=Pawn.Inventory; Inv!=None; Inv=Inv.Inventory )
    {
		if ( Inv.Class == NewWeaponClass )
		{
			Pawn.PendingWeapon = Weapon(Inv);
			if ( !Pawn.PendingWeapon.HasAmmo() )
			{
				ClientMessage( Pawn.PendingWeapon.ItemName$Pawn.PendingWeapon.MessageNoAmmo );
				Pawn.PendingWeapon = None;
				return;
			}
			Pawn.Weapon.PutDown();
			return;
		}
		Count++;
		if ( Count > 1000 )
			return;
    }
}
	
// The player wants to select previous item
exec function PrevItem()
{
	local Inventory Inv;
	local Powerups LastItem;

    if ( (Level.Pauser!=None) || (Pawn == None) )
		return;
	if (Pawn.SelectedItem==None) 
	{
		Pawn.SelectedItem = Pawn.Inventory.SelectNext();
		Return;
	}
	if (Pawn.SelectedItem.Inventory!=None) 
		for( Inv=Pawn.SelectedItem.Inventory; Inv!=None; Inv=Inv.Inventory ) 
		{
			if (Inv==None) Break;
			if ( Inv.IsA('Powerups') && Powerups(Inv).bActivatable) LastItem=Powerups(Inv);
		}
	for( Inv=Pawn.Inventory; Inv!=Pawn.SelectedItem; Inv=Inv.Inventory ) 
	{
		if (Inv==None) Break;
		if ( Inv.IsA('Powerups') && Powerups(Inv).bActivatable) LastItem=Powerups(Inv);
	}
	if (LastItem!=None) 
		Pawn.SelectedItem = LastItem;
}

// The player wants to active selected item
exec function ActivateItem()
{
	if( Level.Pauser!=None )
		return;
	if ( (Pawn != None) && (Pawn.SelectedItem!=None) ) 
		Pawn.SelectedItem.Activate();
}

// The player wants to fire.
exec function Fire( optional float F )
{
	if ( Level.Pauser == PlayerReplicationInfo )
	{
		SetPause(false);
		return;
	}
	if( bDemoOwner || (Pawn == None) )
		return;
    Pawn.Fire();
}

// The player wants to alternate-fire.
exec function AltFire( optional float F )
{
	if ( Level.Pauser == PlayerReplicationInfo )
	{
		SetPause(false);
		return;
	}
	if( bDemoOwner || (Pawn == None) )
		return;
	if( Pawn.Weapon!=None )
		Pawn.Weapon.AltFire();
}

// The player want to reload.
exec function Reload()
{
	if( Level.NetMode == NM_Client )
		ServerReload();

	if( Pawn.Weapon!=None )
		Pawn.Weapon.Reload();
}
function ServerReload()
{
	Reload();
}

// The player wants to use something in the level.
exec function Use()		{ TargetUse();	 }
exec function UnUse()	{ TargetUnuse(); }

function ServerUse()	{ TargetUse();	 }
function ServerUnuse()	{ TargetUnuse(); }

function TargetUse()
{
	local Actor HitActor;

	if( Level.NetMode == NM_Client )
	{
		ServerUse();
		return;
	}

	if ( Level.Pauser == PlayerReplicationInfo )
	{
		SetPause(false);
		return;
	}

	if( Pawn == None )
		return;

	HitActor = PlayerSees( UseDistance, UseLatency );
	if( HitActor? && HitActor.bUsable )
		UseTarget = HitActor;
	else
		UseTarget = None;

	if( UseTarget != None )
	{
		Pawn.NotifyUse( UseTarget );
		UseTarget.OnUse( Self );
	}
}

function TargetUnuse()
{
	if( Level.NetMode == NM_Client )
	{
		ServerUnuse();
		return;
	}

	if ( Level.Pauser == PlayerReplicationInfo )
	{
		SetPause(false);
		return;
	}

	if( UseTarget != None )
	{
		UseTarget.OnUnuse( Self );
		Pawn.NotifyUnuse( UseTarget );
	}
}

function ServerSetTeamEnergy( int TeamNumber, float NewEnergyLevel );

exec function Suicide()
{
	if ( (Pawn != None) && (Level.TimeSeconds - Pawn.LastStartTime > 1) )
    Pawn.KilledBy( Pawn );
}

exec function Name( coerce string S )
{
	SetName(S);
}

exec function SetName( coerce string S)
{
	ChangeName(S);
	UpdateURL("Name", S, true);
	SaveConfig();
}

function ChangeName( coerce string S )
{
    if ( Len(S) > 20 )
        S = left(S,20);
	ReplaceText(S, " ", "_");
    Level.Game.ChangeName( self, S, true );
}

exec function SwitchTeam()
{
	if ( (PlayerReplicationInfo.Team == None) || (PlayerReplicationInfo.Team.TeamIndex == 1) )
		ChangeTeam(0);
	else
		ChangeTeam(1);
}

exec function ChangeTeam( int N )
{
	local TeamInfo OldTeam;

	OldTeam = PlayerReplicationInfo.Team;
    Level.Game.ChangeTeam(self, N, true);
	if ( Level.Game.bTeamGame && (PlayerReplicationInfo.Team != OldTeam) )
    {
		if ( Pawn != None )
		Pawn.Died( None, class'DamageType', Pawn.Location );
	}
}

function ServerChangeClass( class<Pawn> PawnClass )
{
    Level.Game.SetPawnClass( Self, PawnClass );
}

exec function SwitchLevel( string URL )
{
	if( Level.NetMode==NM_Standalone || Level.netMode==NM_ListenServer )
		Level.ServerTravel( URL, false );
}

exec function ClearProgressMessages()
{
	local int i;

	for (i=0; i<ArrayCount(ProgressMessage); i++)
	{
		ProgressMessage[i] = "";
		ProgressColor[i] = class'Canvas'.Static.MakeColor(255,255,255);
	}
}

exec event SetProgressMessage( int Index, string S, color C )
{
	if ( Index < ArrayCount(ProgressMessage) )
	{
		ProgressMessage[Index] = S;
		ProgressColor[Index] = C;
	}
}

exec event SetProgressTime( float T )
{
	ProgressTimeOut = T + Level.TimeSeconds;
}

function interface bool ProgressEnabled()		{ return ProgressTimeOut >= Level.TimeSeconds; }
function interface string GetProgressMessageA()	{ return ProgressMessage[0]; }
function interface string GetProgressMessageB()	{ return ProgressMessage[1]; }
function interface string GetProgressMessageC()	{ return ProgressMessage[2]; }
function interface string GetProgressMessageD()	{ return ProgressMessage[3]; }
function interface color GetProgressColorA()	{ return ProgressColor[0]; }
function interface color GetProgressColorB()	{ return ProgressColor[1]; }
function interface color GetProgressColorC()	{ return ProgressColor[2]; }
function interface color GetProgressColorD()	{ return ProgressColor[3]; }

function Restart()
{
	Super.Restart();
	ServerTimeStamp = 0;
	TimeMargin = 0;
	EnterStartState();
	SetViewTarget(Pawn);
	bBehindView = Pawn.PointOfView();
	ClientRestart(Pawn);
}

function EnterStartState()
{
	local name NewState;

	if ( Pawn.PhysicsVolume.bWaterVolume )
	{
		if ( Pawn.HeadVolume.bWaterVolume )
			Pawn.BreathTime = Pawn.UnderWaterTime;
		NewState = Pawn.WaterMovementState;
	}
	else  
		NewState = Pawn.LandMovementState;

	if ( IsInState(NewState) )
		BeginState();
	else
		GotoState(NewState);
}

function ClientRestart( optional Pawn NewPawn )
{
	if( NewPawn != None )
{
		Pawn = NewPawn;
		Pawn.Controller = Self;
	}
	if ( (Pawn != None) && Pawn.bTearOff )
	{
		Pawn.Controller = None;
		Pawn = None;
	}
	if ( Pawn == None )
	{
		GotoState('WaitingForPawn');
		return;
	}
	Pawn.ClientRestart();
	SetViewTarget(Pawn);
	bBehindView = Pawn.PointOfView();
    CleanOutSavedMoves();
	EnterStartState();	
}

exec function BehindView( Bool B )
{
	if ( (Level.NetMode == NM_Standalone) || Level.Game.bAllowBehindView || PlayerReplicationInfo.bOnlySpectator || PlayerReplicationInfo.bAdmin || IsA('Admin') )
	{
	    bBehindView = B;
	    ClientSetBehindView(bBehindView);
    }
}
	
exec function ToggleBehindView()
{
	ServerToggleBehindview();
}

function ServerToggleBehindView()
{
	bBehindView = !bBehindView;
	ClientSetBehindView(bBehindView);
}

//=============================================================================
// functions.

// Just changed to pendingWeapon
function ChangedWeapon()
{
    if ( Pawn.Weapon != None )
    {
        Pawn.Weapon.LinkMesh(Pawn.Weapon.default.Mesh);
        LastPawnWeapon = Pawn.Weapon.Class;
    }
}

event TravelPostAccept()
{
	if ( Pawn.Health <= 0 )
		Pawn.Health = Pawn.Default.Health;
}

event PlayerTick( float DeltaTime )
{
	if ( bForcePrecache && (Level.TimeSeconds > ForcePrecacheTime) )
	{
		bForcePrecache = false;
		Level.FillPrecacheMaterialsArray();
		Level.FillPrecacheStaticMeshesArray();
	}		
	PlayerInput.PlayerInput(DeltaTime);
	if ( bUpdatePosition )
		ClientUpdatePosition();
	PlayerMove(DeltaTime);
}

function PlayerMove(float DeltaTime);

/* AdjustAim()

Used for
	warning potential targets (if any)
	player aiming help (mainly if AimingHelp != 0, but always some help!)
*/
function bool AdjustAim( class<Projectile> ProjectileClass, float ProjSpeed, vector ProjStart, float WeaponFireSpread, bool bLeadTarget, bool bWarnTarget, bool bTossed, bool bTrySplash )
{
	local vector FireDir, AimSpot, HitNormal, HitLocation, OldAim, AimOffset;
	local Actor BestTarget;
	local float BestAim, BestDist;
	local Actor HitActor;
	local bool bNoZAdjust, bLeading;
	local bool bFastHit; // set if instant hit or high speed projectile
	local vector CameraLoc;

	/*
#debugbegin
	if( DebugAIAttacks() )
	{
		DMTNS( "AdjustAim (player)" );
		DMTNS( " Target:           "	$ Target );
		DMTNS( " ProjClass:        "	$ ProjectileClass );
		DMTNS( " ProjSpeed:        "	$ ProjSpeed );
		DMTNS( " ProjStart:        "	$ class'Util'.static.GetVectorString(ProjStart) );
		DMTNS( " WeaponFireSpread: "	$ WeaponFireSpread );
		DMTNS( " bLeadTarget:      "	$ bLeadTarget );
		DMTNS( " bWarnTarget:      "	$ bWarnTarget );
		DMTNS( " bTossed:          "	$ bTossed );
		DMTNS( " bTrySplash:       "	$ bTrySplash );

		AddActor( Pawn, ColorCyan() );
		AddCylinder( ProjStart, 16, 16, ColorMagenta() );
		if( Target != None )
		{
			AddCylinder( Target.Location, 16, 16, ColorYellow() );
			AddArrow( ProjStart, Target.Location, ColorGreen() );
		}
	}
#debugend
	*/

	FireDir = vector(Rotation);
	bFastHit = ( ProjSpeed <= 0 || ProjSpeed > 10000 );

	// ==> warn target?

	if( bFastHit )
		HitActor = Trace( HitLocation, HitNormal, ProjStart + 10000 * FireDir, ProjStart, true );
	else 
		HitActor = Trace( HitLocation, HitNormal, ProjStart + 4000 * FireDir, ProjStart, true );

	if( HitActor != None && HitActor.bProjTarget )
	{
//!!MERGE		if( bWarnTarget && Pawn(HitActor) != None && Pawn(HitActor).Controller != None )
//!!MERGE			Pawn(HitActor).Controller.ShotFiredNotification( Pawn, FireDir, ProjectileClass, ProjSpeed );

		BestTarget = HitActor;
		bNoZAdjust = true;
		OldAim = HitLocation;
		BestDist = VSize(BestTarget.Location - Pawn.Location);
	}
	else
	{
		// adjust aim based on FOV
		BestAim = 0.95;
		if ( AimingHelp == 1 )
		{
			BestAim = 0.93;
			if( bFastHit )
				BestAim = 0.97; 
			if ( FOVAngle < DefaultFOV - 8 )
				BestAim = 0.99;
		}
		else
		{
			if( bFastHit )
				BestAim = 0.98; 
			if ( FOVAngle != DefaultFOV )
				BestAim = 0.995;
		}

//!!MERGE		BestTarget = PickTarget( BestAim, BestDist, FireDir, ProjStart );
		if( BestTarget != None )
		{
//!!MERGE			if( bWarnTarget && Pawn(BestTarget) != None && Pawn(BestTarget).Controller != None )
//!!MERGE				Pawn(BestTarget).Controller.ShotFiredNotification( Pawn, FireDir, ProjectileClass, ProjSpeed );

			OldAim = ProjStart + FireDir * BestDist;
		}
	}

	if( BestTarget == None || AimingHelp == 0 )
	{
/*
#debugbegin
		HitActor = Trace( HitLocation, HitNormal, ProjStart + 8192.0*vector(Rotation), ProjStart, true ); 
		if( Pawn(HitActor) != None )
			AddArrow( ProjStart, HitLocation, ColorGreen() );
		else
			AddArrow( ProjStart, HitLocation, ColorRed() );
#debugend
*/
		CameraLoc = Pawn.Location;
		CameraLoc.Z += Pawn.BaseEyeHeight;
		HitActor = Trace( HitLocation, HitNormal, CameraLoc + 10000 * FireDir, CameraLoc, false );
		if (!HitActor)
			AimRotation = Rotation;
		else
			AimRotation = rotator(HitLocation-ProjStart);
			
		return true;
	}

	// ==> extra aiming help - help with leading also
	if( !bFastHit )
	{
		BestDist = VSize( BestTarget.Location + BestTarget.Velocity * FMin(2, 0.02 + BestDist/ProjSpeed) - ProjStart ); 
		bLeading = true;
		FireDir = BestTarget.Location + BestTarget.Velocity * FMin(2, 0.02 + BestDist/ProjSpeed) - ProjStart;
		AimSpot = ProjStart + BestDist * Normal(FireDir);
		// if splash damage weapon, try aiming at feet - trace down to find floor
		if( bTrySplash && (BestTarget.Velocity != vect(0,0,0) || BestDist > 1500) )
		{
			HitActor = Trace(HitLocation, HitNormal, AimSpot - BestTarget.CollisionHeight * vect(0,0,2), AimSpot, false);
			if( HitActor != None && FastTrace(HitLocation + vect(0,0,4),ProjStart) )
			{
				AimRotation = rotator(HitLocation + vect(0,0,6) - ProjStart);
				return true;
			}
		}
	}
	else
	{
		FireDir = BestTarget.Location - ProjStart;
		AimSpot = ProjStart + BestDist * Normal(FireDir);
	}
	AimOffset = AimSpot - OldAim;

	// adjust Z of shooter if necessary
	if ( bNoZAdjust || (bLeading && Abs(AimOffset.Z) < BestTarget.CollisionHeight) )
		AimSpot.Z = OldAim.Z;
	else if ( AimOffset.Z < 0 )
		AimSpot.Z = BestTarget.Location.Z + 0.4 * BestTarget.CollisionHeight;
	else
		AimSpot.Z = BestTarget.Location.Z - 0.7 * BestTarget.CollisionHeight;

	if ( !bLeading )
	{
		// if not leading, add slight random error ( significant at long distances )
		if ( !bNoZAdjust )
		{
			AimRotation = rotator(AimSpot - ProjStart);
			if ( FOVAngle < DefaultFOV - 8 )
				AimRotation.Yaw = AimRotation.Yaw + 200 - Rand(400);
			else
				AimRotation.Yaw = AimRotation.Yaw + 375 - Rand(750);
			return true;
		}	
	}
	else if ( !FastTrace(ProjStart + 0.9 * BestDist * Normal(FireDir), ProjStart) )
	{
		FireDir = BestTarget.Location - ProjStart;
		AimSpot = ProjStart + BestDist * Normal(FireDir);
	}
		
	AimRotation =  rotator(AimSpot - ProjStart);
		
	return true;
}


function bool NotifyLanded(vector HitNormal)
{
	return bUpdating;
}

//=============================================================================
// Player Control

// Player view.
// Compute the rendering viewpoint for the player.
//

function AdjustView(float DeltaTime )
{
	// teleporters affect your FOV, so adjust it back down
	if ( FOVAngle != DesiredFOV )
	{
		if ( FOVAngle > DesiredFOV )
			FOVAngle = FOVAngle - FMax(7, 0.9 * DeltaTime * (FOVAngle - DesiredFOV)); 
		else 
			FOVAngle = FOVAngle - FMin(-7, 0.9 * DeltaTime * (FOVAngle - DesiredFOV)); 
		if ( Abs(FOVAngle - DesiredFOV) <= 10 )
			FOVAngle = DesiredFOV;
	}

	// adjust FOV for weapon zooming
	if ( bZooming )
	{	
		ZoomLevel += DeltaTime * 1.0;
		if (ZoomLevel > 0.9)
			ZoomLevel = 0.9;
		DesiredFOV = FClamp(90.0 - (ZoomLevel * 88.0), 1, 170);
	} 
}

function CalcBehindView(out vector CameraLocation, out rotator CameraRotation, float Dist)
{
	local vector View,HitLocation,HitNormal;
    local float ViewDist,RealDist;

	CameraRotation = Rotation;
	if ( bBlockCloseCamera )
		CameraLocation.Z += 12;

	View = vect(1,0,0) >> CameraRotation;

    // add view radius offset to camera location and move viewpoint up from origin (amb)
    RealDist = Dist;

    if( Trace( HitLocation, HitNormal, CameraLocation - Dist * vector(CameraRotation), CameraLocation,false,vect(10,10,10) ) != None )
		ViewDist = FMin( (CameraLocation - HitLocation) Dot View, Dist );
	else
		ViewDist = Dist;
    
    if ( !bBlockCloseCamera || !bValidBehindCamera || (ViewDist > 10 + FMax(ViewTarget.CollisionRadius, ViewTarget.CollisionHeight)) )
	{
		//Log("Update Cam ");
		bValidBehindCamera = true;
		OldCameraLoc = CameraLocation - ViewDist * View;
		OldCameraRot = CameraRotation;
	}
	else
	{
		//Log("Dont Update Cam "$bBlockCloseCamera@bValidBehindCamera@ViewDist);
		SetRotation(OldCameraRot);
	}

    CameraLocation = OldCameraLoc; 
    CameraRotation = OldCameraRot;
}

function CalcFirstPersonView( out vector CameraLocation, out rotator CameraRotation )
{
	// First-person view.
	CameraRotation = Rotation;
	CameraLocation = CameraLocation + Pawn.EyePosition() + ShakeOffset;
}

event AddCameraEffect(CameraEffect NewEffect,optional bool RemoveExisting)
{
	if(RemoveExisting)
		RemoveCameraEffect(NewEffect);

	CameraEffects.Length = CameraEffects.Length + 1;
	CameraEffects[CameraEffects.Length - 1] = NewEffect;
}

event RemoveCameraEffect(CameraEffect ExEffect)
{
	local int	EffectIndex;

	for(EffectIndex = 0;EffectIndex < CameraEffects.Length;EffectIndex++)
		if(CameraEffects[EffectIndex] == ExEffect)
		{
			CameraEffects.Remove(EffectIndex,1);
			return;
		}
}

exec function CreateCameraEffect(class<CameraEffect> EffectClass)
{
	AddCameraEffect(new EffectClass);
}

function rotator GetViewRotation()
{
	if ( bBehindView && (Pawn != None) )
		return Pawn.Rotation;
	return Rotation;
}

simulated function Actor PlayerSees( float MaxDistance, float MaxLatency )
{
	local vector TempLoc;
	local rotator TempRot;
	return PlayerSeesEx(MaxDistance,MaxLatency,TempLoc,TempRot);
}
simulated function Actor PlayerSeesEx( float MaxDistance, float MaxLatency, out vector CameraLocation, out rotator CameraRotation )
{
	local vector HitLocation, HitNormal;
	local actor Unused;

	if(	(LastViewTargetTime + MaxLatency < Level.TimeSeconds) ||
		((LastViewTargetActor == None) &&
		(MaxDistance > LastViewTargetTraceDistance)) )
	{
		LastViewTargetTime = Level.TimeSeconds;
		PlayerCalcView( Unused, LastViewTargetLocation, LastViewTargetRotation );
		LastViewTargetActor = Pawn.Trace( HitLocation, HitNormal, LastViewTargetLocation + MaxDistance * vector(LastViewTargetRotation), LastViewTargetLocation, true );
		LastViewTargetTraceDistance = MaxDistance;
		LastViewTargetActualDistance = VSize(HitLocation - LastViewTargetLocation);
		CameraLocation = LastViewTargetLocation;
		CameraRotation = LastViewTargetRotation;
		return LastViewTargetActor;
	}
	else
	if(	(LastViewTargetActor != None) &&
		(MaxDistance < LastViewTargetActualDistance) )
	{
		CameraLocation = LastViewTargetLocation;
		CameraRotation = LastViewTargetRotation;
		return None;
	}
	else
	{
		CameraLocation = LastViewTargetLocation;
		CameraRotation = LastViewTargetRotation;
		return LastViewTargetActor;
	}
}


event PlayerCalcView(out actor ViewActor, out vector CameraLocation, out rotator CameraRotation )
{
	local Pawn PTarget;

	// If desired, call the pawn's own special callview
	if( Pawn != None && Pawn.bSpecialCalcView )
	{
		// try the 'special' calcview. This may return false if its not applicable, and we do the usual.
		if( Pawn.SpecialCalcView(ViewActor, CameraLocation, CameraRotation) )
			return;
	}

	if ( (ViewTarget == None) || ViewTarget.bDeleteMe )
	{
        if ( bViewBot && (CheatManager != None) )
			CheatManager.ViewBot();
        else if ( (Pawn != None) && !Pawn.bDeleteMe )
			SetViewTarget(Pawn);
        else if ( RealViewTarget != None )
            SetViewTarget(RealViewTarget);
		else
			SetViewTarget(self);
	}

	ViewActor = ViewTarget;
	CameraLocation = ViewTarget.Location;

	if ( ViewTarget == Pawn )
	{
		if( bBehindView ) //up and behind
			CalcBehindView(CameraLocation, CameraRotation, CameraDist * Pawn.Default.CollisionRadius);
		else
			CalcFirstPersonView( CameraLocation, CameraRotation );
		return;
	}
	if ( ViewTarget == self )
	{
		if ( bCameraPositionLocked )
			CameraRotation = CheatManager.LockedRotation;
		else
			CameraRotation = Rotation;
		return;
	}

    if ( ViewTarget.IsA('Projectile') && !bBehindView ) 
    {
        CameraLocation += (ViewTarget.CollisionHeight) * vect(0,0,1);
        CameraRotation = Rotation;
        return;
    }

	CameraRotation = ViewTarget.Rotation;
	PTarget = Pawn(ViewTarget);
	if ( PTarget != None )
	{
		if ( Level.NetMode == NM_Client )
		{
			PTarget.SetViewRotation(TargetViewRotation);
            CameraRotation = BlendedTargetViewRotation;

			PTarget.EyeHeight = TargetEyeHeight;
			if ( PTarget.Weapon != None )
				PTarget.Weapon.PlayerViewOffset = TargetWeaponViewOffset;
		}
		else if ( PTarget.IsPlayerPawn() )
			CameraRotation = PTarget.GetViewRotation();
		if ( !bBehindView )
			CameraLocation += PTarget.EyePosition();
	}
	if ( bBehindView )
	{
		CameraLocation = CameraLocation + (ViewTarget.Default.CollisionHeight - ViewTarget.CollisionHeight) * vect(0,0,1);
		CalcBehindView(CameraLocation, CameraRotation, CameraDist * ViewTarget.Default.CollisionRadius);
	}
}

function int BlendRot(float DeltaTime, int BlendC, int NewC)
{
	if ( Abs(BlendC - NewC) > 32767 )
	{
		if ( BlendC > NewC )
			NewC += 65536;
		else
			BlendC += 65536;
	}
	if ( Abs(BlendC - NewC) > 4096 )
		BlendC = NewC;
	else
		BlendC = BlendC + (NewC - BlendC) * FMin(1,24 * DeltaTime);

	return (BlendC & 65535);
}

exec function TestShake(float Magnitude,optional float Duration){ ShakeView(Magnitude,Duration); }
// Replication note: only call when Level.NetMode != NM_DedicatedServer, or from a non-simulated function (otherwise you'll get duplicate calls).
simulated function ShakeView(float Magnitude,optional float Duration){ ClientShake(Magnitude,Duration); }
simulated private function ClientShake(float Magnitude,optional float Duration)
{
	if(Duration==0)
		Duration=0.5;

	ShakeMagX = Magnitude;
	ShakeMagY = Magnitude;

	if( FRand()>0.5 )
		ShakeMagX = -ShakeMagX;

	if( FRand()>0.5 )
		ShakeMagY = -ShakeMagY;

	ShakeTime = Duration;
	ShakeDuration = Duration;
	}
function ViewShake(float DeltaTime)
	{
	local float Pct;

	ShakeOffset=vect(0,0,0);

	ShakeTime -= DeltaTime;
	if( ShakeTime<=0 )
		return;
		
	Pct = ShakeTime / ShakeDuration;
		
	ShakeOffset.Y = Pct * ShakeMagX * sin( ShakeTime * ShakePeriodX * PI );
	ShakeOffset.Z = Pct * ShakeMagY * sin( ShakeTime * ShakePeriodY * PI );

	ShakeOffset.Y = FClamp( ShakeOffset.Y, -32.0, 32.0 );
	ShakeOffset.Z = FClamp( ShakeOffset.Z, -32.0, 32.0 );

	ShakeOffset = ShakeOffset >> Rotation;
/*
	ShakeOffset.X = Pct * ShakeMagX * sin( ShakeTime * ShakePeriodX * PI );
	ShakeOffset.Y = Pct * ShakeMagY * sin( ShakeTime * ShakePeriodY * PI );
	ShakeOffset.Z = Pct * ShakeMagZ * sin( ShakeTime * ShakePeriodZ * PI );
*/
}

function bool TurnTowardNearestEnemy();

function TurnAround()
{
	if ( !bSetTurnRot )
	{
		TurnRot180 = Rotation;
		TurnRot180.Yaw += 32768;
		bSetTurnRot = true;
	}
	
	DesiredRotation = TurnRot180;
	bRotateToDesired = ( DesiredRotation.Yaw != Rotation.Yaw );
}
					
function UpdateRotation(float DeltaTime, float maxPitch)
{
	local rotator newRotation, ViewRotation;

	if ( bInterpolating || ((Pawn != None) && Pawn.bInterpolating) )
	{
		ViewShake(deltaTime);
		return;
	}
	ViewRotation = Rotation;
	DesiredRotation = ViewRotation; //save old rotation
	if ( bTurnToNearest != 0 )
		TurnTowardNearestEnemy();
	else if ( bTurn180 != 0 )
		TurnAround();
	else
	{
		TurnTarget = None;
		bRotateToDesired = false;
		bSetTurnRot = false;
		ViewRotation.Yaw += 32.0 * DeltaTime * aTurn;
		ViewRotation.Pitch += 32.0 * DeltaTime * aLookUp;
	}
	ViewRotation.Pitch = ViewRotation.Pitch & 65535;
	If ((ViewRotation.Pitch > 18000) && (ViewRotation.Pitch < 49152))
	{
		If (aLookUp > 0) 
			ViewRotation.Pitch = 18000;
		else
			ViewRotation.Pitch = 49152;
	}

	SetRotation(ViewRotation);

	ViewShake(deltaTime);
		
	NewRotation = ViewRotation;
	NewRotation.Roll = Rotation.Roll;

	if ( !bRotateToDesired && (Pawn != None) && (!bFreeCamera || !bBehindView) )
		Pawn.FaceRotation(NewRotation, deltatime);
}

function ClearDoubleClick()
{
	if (PlayerInput != None)
		PlayerInput.DoubleClickTimer = 0.0;
}

//-----------------------------------------------------------------------------
//								  STATE CODE
//-----------------------------------------------------------------------------

// Player movement.
// Player Standing, walking, running, falling.
state PlayerWalking
{
ignores SeeEnemy, HearNoise, Bump;

	function bool NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		if ( NewVolume.bWaterVolume )
			GotoState(Pawn.WaterMovementState);
		return false;
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{
		local vector OldAccel;
		local bool OldCrouch;

		if ( Pawn == None )
			return;
		OldAccel = Pawn.Acceleration;
		Pawn.Acceleration = NewAccel;

		if ( bDoubleJump && (bUpdating || Pawn.CanDoubleJump()) )
			Pawn.DoDoubleJump(bUpdating);
        else if ( bPressedJump )
			Pawn.DoJump(bUpdating);
//NEW (mdf) allow crouching/uncrouching at any time
		if ( true )
/*OLD		
		if ( Pawn.Physics != PHYS_Falling )
*/
		{
			OldCrouch = Pawn.bWantsToCrouch;
			if (bDuck == 0)
				Pawn.ShouldCrouch(false);
			else if ( Pawn.bCanCrouch )
				Pawn.ShouldCrouch(true);
		}
	}

	function PlayerMove( float DeltaTime )
	{
		local vector X,Y,Z, NewAccel;
		local eDoubleClickDir DoubleClickMove;
		local rotator OldRotation, ViewRotation;
		local bool	bSaveJump;

		GetAxes(Pawn.Rotation,X,Y,Z);

		// Update acceleration.
		NewAccel = aForward*X + aStrafe*Y; 
		NewAccel.Z = 0;
		if ( VSize(NewAccel) < 1.0 )
			NewAccel = vect(0,0,0);
		DoubleClickMove = PlayerInput.CheckForDoubleClickMove(DeltaTime);
		
		GroundPitch = 0;	
		ViewRotation = Rotation;
		if ( Pawn.Physics == PHYS_Walking )
		{
			// tell pawn about any direction changes to give it a chance to play appropriate animation
			//if walking, look up/down stairs - unless player is rotating view
			if ( (bLook == 0) 
                && (((Pawn.Acceleration != Vect(0,0,0)) && bSnapToLevel) || !bKeyboardLook) )
			{
				if ( bLookUpStairs || bSnapToLevel )
				{
					GroundPitch = FindStairRotation(deltaTime);
					ViewRotation.Pitch = GroundPitch;
				}
				else if ( bCenterView )
				{
					ViewRotation.Pitch = ViewRotation.Pitch & 65535;
					if (ViewRotation.Pitch > 32768)
						ViewRotation.Pitch -= 65536;
					ViewRotation.Pitch = ViewRotation.Pitch * (1 - 12 * FMin(0.0833, deltaTime));
                    if ( Abs(ViewRotation.Pitch) < 200 )
						ViewRotation.Pitch = 0;	
				}
			}
		}	
		else
		{
			if ( !bKeyboardLook && (bLook == 0) && bCenterView )
			{
				ViewRotation.Pitch = ViewRotation.Pitch & 65535;
				if (ViewRotation.Pitch > 32768)
					ViewRotation.Pitch -= 65536;
				ViewRotation.Pitch = ViewRotation.Pitch * (1 - 12 * FMin(0.0833, deltaTime));
                if ( Abs(ViewRotation.Pitch) < 200 )
					ViewRotation.Pitch = 0;	
			}
		}
		Pawn.CheckBob(DeltaTime, Y);

		// Update rotation.
		SetRotation(ViewRotation);
		OldRotation = Rotation;
		UpdateRotation(DeltaTime, 1);
		bDoubleJump = false;

		if ( bPressedJump && Pawn.CannotJumpNow() )
		{
			bSaveJump = true;
			bPressedJump = false;
		}
		else
			bSaveJump = false;

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
		else
			ProcessMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
		bPressedJump = bSaveJump;
	}

	function BeginState()
	{
       	DoubleClickDir = DCLICK_None;
       	bPressedJump = false;
       	GroundPitch = 0;
		if ( Pawn != None )
		{
		if ( Pawn.Mesh == None )
			Pawn.SetMesh();
		Pawn.ShouldCrouch(false);
		if (Pawn.Physics != PHYS_Falling && Pawn.Physics != PHYS_Karma) // FIXME HACK!!!
			Pawn.SetPhysics(PHYS_Walking);
			if( Pawn.Weapon != None )
				Pawn.Weapon.BringUp();
		}
	}
	
	function EndState()
	{
		GroundPitch = 0;
		if ( Pawn != None )
		{
			if( bDuck==0 )
			Pawn.ShouldCrouch(false);
			if( Pawn.Weapon != None )
				Pawn.Weapon.PutDown();
		}
	}
}

// player is climbing ladder
state PlayerClimbing
{
ignores SeeEnemy, HearNoise, Bump;

	function bool NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		if ( NewVolume.bWaterVolume )
			GotoState(Pawn.WaterMovementState);
		else
			GotoState(Pawn.LandMovementState);
		return false;
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{
		local vector OldAccel;

		OldAccel = Pawn.Acceleration;
		Pawn.Acceleration = NewAccel;

		if ( bPressedJump )
		{
			Pawn.DoJump(bUpdating);
			if ( Pawn.Physics == PHYS_Falling )
				GotoState('PlayerWalking');
		}
	}

	function PlayerMove( float DeltaTime )
	{
		local vector X,Y,Z, NewAccel;
		local eDoubleClickDir DoubleClickMove;
		local rotator OldRotation, ViewRotation;
		local bool	bSaveJump;

		GetAxes(Rotation,X,Y,Z);

		// Update acceleration.
		if ( Pawn.OnLadder != None )
			NewAccel = aForward*Pawn.OnLadder.ClimbDir; 
		else
			NewAccel = aForward*X + aStrafe*Y;
		if ( VSize(NewAccel) < 1.0 )
			NewAccel = vect(0,0,0);
		
		ViewRotation = Rotation;

		// Update rotation.
		SetRotation(ViewRotation);
		OldRotation = Rotation;
		UpdateRotation(DeltaTime, 1);

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
		else
			ProcessMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
		bPressedJump = bSaveJump;
	}

	function BeginState()
	{
		Pawn.ShouldCrouch(false);
		bPressedJump = false;
	}
	
	function EndState()
	{
		if ( Pawn != None )
			Pawn.ShouldCrouch(false);
	}
}

// Player movement.
// Player Driving a Karma vehicle.
state PlayerDriving
{
ignores SeeEnemy, HearNoise, Bump;

/* Reset() 
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	if (DrivenVehicle?)
		DrivenVehicle.KDriverLeave(PassengerNumber, true); // Force the driver out of the car
    PawnDied(Pawn);
	Super.Reset();
	SetViewTarget(self);
	bBehindView = false;
	WaitDelay = Level.TimeSeconds + 2;
    FixFOV();
    if ( !PlayerReplicationInfo.bOnlySpectator )
		GotoState('PlayerWaiting');
}

// unpossessed a pawn (not because pawn was killed)
function UnPossess()
{
	if ( Pawn != None )
	{
		SetLocation(Pawn.Location);
		Pawn.RemoteRole = ROLE_SimulatedProxy;
		if (XMPVehicleBarrel(Pawn)? && KVehicle(XMPVehicleBarrel(Pawn).ConglomerateID).Turrets[XMPVehicleBarrel(Pawn).BarrelNumber]?)
		{
			KVehicle(XMPVehicleBarrel(Pawn).ConglomerateID).Turrets[XMPVehicleBarrel(Pawn).BarrelNumber].RemoteRole = ROLE_SimulatedProxy;
		} else 	if (KVehicle(Pawn)? && KVehicle(Pawn).Turrets[0]?)
			KVehicle(Pawn).Turrets[0].RemoteRole = ROLE_SimulatedProxy;
		Pawn.UnPossessed();
		CleanOutSavedMoves();  // don't replay moves previous to unpossession
		if ( Viewtarget == Pawn )
			SetViewTarget(self);
	}
	Pawn = None;
	GotoState('Spectating');
}

exec function Suicide()
{
	if ( (DrivenVehicle != None) && (DrivenVehicle.PassengerPawns[PassengerNumber] != None) )
	{
		DrivenVehicle.KDriverLeave(PassengerNumber, true);
		Pawn.KilledBy( Pawn );
	}
}

	function TargetUse()
	{
		if ( Level.Pauser == PlayerReplicationInfo )
		{
			SetPause(false);
			return;
		}

		if (DrivenVehicle != None && Role == ROLE_Authority)
			{
				if ( Level.TimeSeconds > (EntryTime + 1))
					DrivenVehicle.bGetOut[PassengerNumber] = 1;
				return;
			} else if ( DrivenVehicle != None )
				ServerUse();

	}
	
	function rotator GetAimRotation()
	{
		local Rotator OffsetRotation;
		OffsetRotation=DrivenVehicle.Turrets[PassengerNumber].barrel.Rotation;
		OffsetRotation.Yaw+=32767;
		OffsetRotation.Pitch= -(DrivenVehicle.Turrets[PassengerNumber].barrel.Rotation.Pitch);
		OffsetRotation.Roll= -(DrivenVehicle.Turrets[PassengerNumber].barrel.Rotation.Roll);
		return OffsetRotation;
	}

	function rotator GetViewRotation()
	{
		local Rotator OffsetRotation;
		if (DrivenVehicle != None && DrivenVehicle.Turrets[PassengerNumber].bArticulated)
		{
			OffsetRotation=DrivenVehicle.TurretRotation[PassengerNumber];
			OffsetRotation.Yaw+=32767;
			OffsetRotation.Pitch= -(DrivenVehicle.TurretRotation[PassengerNumber].Pitch);
			OffsetRotation.Roll= -(DrivenVehicle.TurretRotation[PassengerNumber].Roll);
			return OffsetRotation;
		}
		else
			return Rotation;
	}
	
	function UpdateRotation(float DeltaTime, float maxPitch)
	{
		local rotator ViewRotation;
		local int DeltaPitch;
		local int DeltaYaw;

		ViewRotation = Rotation;
		DesiredRotation = ViewRotation; //save old rotation

		TurnTarget = None;
		bRotateToDesired = false;
		bSetTurnRot = false;
		ViewRotation.Yaw += 32.0 * DeltaTime * aTurn;
		ViewRotation.Pitch += 32.0 * DeltaTime * aLookUp;

		ViewRotation.Pitch = ViewRotation.Pitch & 65535;
		If ((ViewRotation.Pitch > 18000) && (ViewRotation.Pitch < 49152))
		{
			If (aLookUp > 0) 
				ViewRotation.Pitch = 18000;
			else
				ViewRotation.Pitch = 49152;
		}
		if (DrivenVehicle != None)
		{
		DeltaPitch = 32.0 * DeltaTime * aLookup;
		DeltaPitch = Clamp (DeltaPitch, -DeltaTime*DrivenVehicle.Turrets[PassengerNumber].MaxRotation, DeltaTime*DrivenVehicle.Turrets[PassengerNumber].MaxRotation );
		DeltaYaw = 32.0 * DeltaTime * aTurn;
		DeltaYaw = Clamp (DeltaYaw, -DeltaTime*DrivenVehicle.Turrets[PassengerNumber].MaxRotation, DeltaTime*DrivenVehicle.Turrets[PassengerNumber].MaxRotation );
		
		ViewRotation.Roll=0;
		ViewRotation.Pitch=DeltaPitch;
		ViewRotation.Yaw=DeltaYaw;
		ViewRotation = Normalize(ViewRotation);
		DrivenVehicle.Turrets[PassengerNumber].RotationCenter=Normalize(DrivenVehicle.Turrets[PassengerNumber].RotationCenter);
		DeltaPitch = ViewRotation.Pitch;
		DeltaYaw = ViewRotation.Yaw;

		if (DrivenVehicle.Turrets[PassengerNumber].PitchConstraint == 65536)
			DeltaPitch = (TempTurretRotation.Pitch- DeltaPitch)% 65535;
		else
			DeltaPitch = Clamp ((TempTurretRotation.Pitch- DeltaPitch) %65535,
			(DrivenVehicle.Turrets[PassengerNumber].RotationCenter.Pitch -
			DrivenVehicle.Turrets[PassengerNumber].PitchConstraint),
			DrivenVehicle.Turrets[PassengerNumber].RotationCenter.Pitch +
			DrivenVehicle.Turrets[PassengerNumber].PitchConstraint );

		if (DrivenVehicle.Turrets[PassengerNumber].YawConstraint == 65536)
			DeltaYaw = (TempTurretRotation.Yaw+ DeltaYaw)%65535;
		else
			DeltaYaw = Clamp ((TempTurretRotation.Yaw+ DeltaYaw)%65535,
			(DrivenVehicle.Turrets[PassengerNumber].RotationCenter.Yaw -
			DrivenVehicle.Turrets[PassengerNumber].YawConstraint),
			DrivenVehicle.Turrets[PassengerNumber].RotationCenter.Yaw +
			DrivenVehicle.Turrets[PassengerNumber].YawConstraint );
		
		TempTurretRotation.Roll=0;
		TempTurretRotation.Pitch=DeltaPitch;
		TempTurretRotation.Yaw=DeltaYaw;
		//ViewRotation=Rotator(-Vector(TempTurretRotation));
		//ViewRotation.Roll=0;
		DrivenVehicle.MyTurretRotation=TempTurretRotation;
		}
		SetRotation(ViewRotation);

		ViewShake(deltaTime);
	}




	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{
	}

	//PWC 2003.04.14 Disabled vehicle firing to see if we can get regular firing working
    exec function Fire(optional float F)
    {
/*		if(DrivenVehicle != None)
		{
			if (DrivenVehicle.Turrets[PassengerNumber].barrel?)
			{
				DrivenVehicle.Turrets[PassengerNumber].barrel.fire();
			}
		}
*/
		if (Pawn !=None)
			Pawn.fire();
    }

    exec function AltFire(optional float F)
    {
/*		//local KVehicle DrivenVehicle;

		//DrivenVehicle = KVehicle(Pawn);
		if(DrivenVehicle != None)
		{

			DrivenVehicle.VehicleFire(PassengerNumber , true);
			DrivenVehicle.bVehicleIsAltFiring[PassengerNumber] = 1;
		}
*/
    }

	// Set the throttle, steering etc. for the vehicle based on the input provided
	function ProcessDrive(float InForward, float InStrafe, bool InJump, rotator DeltaRot)
	{
        if( DrivenVehicle == None )
        {
            log("PlayerDriving.PlayerMove: No Vehicle");
            return;
        }
        //log("Drive:"$InForward$" Steer:"$InStrafe);
		if( PassengerNumber == 0 )
		{
			if( InForward > 1 )
			{
				DrivenVehicle.Throttle = 1;
				DrivenVehicle.bThrottling=true;
			}
			else if( InForward < -1 )
			{
				DrivenVehicle.Throttle = -1;
				DrivenVehicle.bThrottling=true;
			}
			else
			{
				DrivenVehicle.Throttle = 0;
				DrivenVehicle.bThrottling=false;
			}

			if( InStrafe < -1 )
				DrivenVehicle.Steering = 1;
			else if( InStrafe > 1 )
				DrivenVehicle.Steering = -1;
			else
				DrivenVehicle.Steering = 0; 
		}

		DrivenVehicle.TurretRotation[PassengerNumber] = DeltaRot;
		DrivenVehicle.MyTurretRotation = DeltaRot;

	}

    function PlayerMove( float DeltaTime )
	{
		local Rotator TempRot;

		UpdateRotation(DeltaTime, 2);
		TempRot=TempTurretRotation;
			// Only servers can actually do the driving logic.
			if(Role < ROLE_Authority)
				ServerDrive(aForward, aStrafe, bPressedJump, TempRot);
			else
				ProcessDrive(aForward, aStrafe, bPressedJump, TempRot);

			// If the vehicle is being controlled here - set replicated variables.
			//DrivenVehicle = KVehicle(Pawn);
		if(DrivenVehicle != None)
		{

			if(bSprint == 1 && PassengerNumber == 0)
			{
				DrivenVehicle.DoSprint();
			}

			if(bFire == 0 && DrivenVehicle.bVehicleIsFiring[PassengerNumber]==1)
			{
				DrivenVehicle.VehicleCeaseFire(PassengerNumber, false);
				DrivenVehicle.bVehicleIsFiring[PassengerNumber] = 0;
			}
			
			if(bAltFire == 0 && DrivenVehicle.bVehicleIsAltFiring[PassengerNumber]==1)
			{
				DrivenVehicle.VehicleCeaseFire(PassengerNumber, true);
				DrivenVehicle.bVehicleIsAltFiring[PassengerNumber] = 0;
			}
		}
	}

	function BeginState()
	{
		//Pawn.weapon.putdown();
		CleanOutSavedMoves();
	}
	
	function EndState()
	{
		CleanOutSavedMoves();
		//Pawn.weapon.bringup();
	}
Begin:
}

exec function DumpStates()
{
	local Pawn P;
	local Controller C;
	foreach AllActors( class'Pawn', P )
		DM(P@P.GetStateName());
	foreach AllActors( class'Controller', C )
		DM(C@C.GetStateName());
	if( Role < ROLE_Authority )
		ServerDumpStates();
}

function ServerDumpStates()
{
	DumpStates();
}

//NEW
//PWC 2003.04.12
// Player Controlling a Stationary Turret
state PlayerTurreting
{
ignores SeeEnemy, HearNoise, Bump;

/* Reset() 
reset actor to initial state - used when restarting level without reloading.
*/
function Reset()
{
	if (DrivenVehicle?)
		DrivenVehicle.KDriverLeave(PassengerNumber, true); // Force the driver out of the car
    PawnDied(Pawn);
	Super.Reset();
	SetViewTarget(self);
	bBehindView = false;
	WaitDelay = Level.TimeSeconds + 2;
    FixFOV();
    if ( !PlayerReplicationInfo.bOnlySpectator )
		GotoState('PlayerWaiting');
}

// unpossessed a pawn (not because pawn was killed)
function UnPossess()
{
	if ( StationaryTurret(Pawn) != None )
	{
		SetLocation(Pawn.Location);
		Pawn.RemoteRole = ROLE_SimulatedProxy;
		if (StationaryTurret(Pawn).Barrel?)
			StationaryTurret(Pawn).Barrel.RemoteRole = ROLE_SimulatedProxy;
		Pawn.UnPossessed();
		CleanOutSavedMoves();  // don't replay moves previous to unpossession
		if ( Viewtarget == Pawn )
			SetViewTarget(self);
	}
	Pawn = None;
	GotoState('Spectating');
}

exec function Suicide()
{
	if (StationaryTurret(Pawn) != None) 
		StationaryTurret(Pawn).KDriverLeave(true);
		Pawn.KilledBy( Pawn );
}

	function UpdateRotation(float DeltaTime, float maxPitch)
	{
		local StationaryTurret DrivenTurret;
		local rotator ViewRotation;
		local int DeltaPitch;
		local int DeltaYaw;

		ViewRotation = Rotation;
		DesiredRotation = ViewRotation; //save old rotation
		DrivenTurret = StationaryTurret(Pawn);

		TurnTarget = None;
		bRotateToDesired = false;
		bSetTurnRot = false;
		ViewRotation.Yaw += 32.0 * DeltaTime * aTurn;
		ViewRotation.Pitch += 32.0 * DeltaTime * aLookUp;

		ViewRotation.Pitch = ViewRotation.Pitch & 65535;
		If ((ViewRotation.Pitch > 18000) && (ViewRotation.Pitch < 49152))
		{
			If (aLookUp > 0) 
				ViewRotation.Pitch = 18000;
			else
				ViewRotation.Pitch = 49152;
		}
		if (DrivenTurret != None)
		{
		DeltaPitch = 32.0 * DeltaTime * aLookup;
		DeltaPitch = Clamp (DeltaPitch, -DeltaTime*DrivenTurret.MaxRotation, DeltaTime*DrivenTurret.MaxRotation );
		DeltaYaw = 32.0 * DeltaTime * aTurn;
		DeltaYaw = Clamp (DeltaYaw, -DeltaTime*DrivenTurret.MaxRotation, DeltaTime*DrivenTurret.MaxRotation );
		
		ViewRotation.Roll=0;
		ViewRotation.Pitch=DeltaPitch;
		ViewRotation.Yaw=DeltaYaw;
		ViewRotation = Normalize(ViewRotation);
		DrivenTurret.RotationCenter=Normalize(DrivenTurret.RotationCenter);
		DeltaPitch = ViewRotation.Pitch;
		DeltaYaw = ViewRotation.Yaw;

		if (DrivenTurret.PitchConstraint == 65536)
			DeltaPitch = (TempTurretRotation.Pitch- DeltaPitch)% 65535;
		else
			DeltaPitch = Clamp ((TempTurretRotation.Pitch- DeltaPitch) %65535,
			(DrivenTurret.RotationCenter.Pitch -
			DrivenTurret.PitchConstraint),
			DrivenTurret.RotationCenter.Pitch +
			DrivenTurret.PitchConstraint );

		if (DrivenTurret.YawConstraint == 65536)
			DeltaYaw = (TempTurretRotation.Yaw+ DeltaYaw)%65535;
		else
			DeltaYaw = Clamp ((TempTurretRotation.Yaw+ DeltaYaw)%65535,
			(DrivenTurret.RotationCenter.Yaw -
			DrivenTurret.YawConstraint),
			DrivenTurret.RotationCenter.Yaw +
			DrivenTurret.YawConstraint );
		
		TempTurretRotation.Roll=0;
		TempTurretRotation.Pitch=DeltaPitch;
		TempTurretRotation.Yaw=DeltaYaw;
		ViewRotation=Rotator(-Vector(TempTurretRotation));
		ViewRotation.Roll=0;
		DrivenTurret.MyTurretRotation=TempTurretRotation;
		}
		SetRotation(ViewRotation);

		ViewShake(deltaTime);
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{

	}

	function rotator GetAimRotation()
	{
		local Rotator OffsetRotation;
		local StationaryTurret DrivenTurret;
		DrivenTurret = StationaryTurret(Pawn);
		OffsetRotation=DrivenTurret.barrel.Rotation;
		OffsetRotation.Yaw+=32767;
		OffsetRotation.Pitch= -(DrivenTurret.barrel.Rotation.Pitch);
		OffsetRotation.Roll= -(DrivenTurret.barrel.Rotation.Roll);
		return OffsetRotation;
	}

	function rotator GetViewRotation()
	{
		local Rotator OffsetRotation;
		local StationaryTurret DrivenTurret;
		DrivenTurret = StationaryTurret(Pawn);
		if (DrivenTurret != None && DrivenTurret.bArticulated)
		{
			OffsetRotation=DrivenTurret.TurretRotation;
			OffsetRotation.Yaw+=32767;
			OffsetRotation.Pitch= -(DrivenTurret.TurretRotation.Pitch);
			OffsetRotation.Roll= -(DrivenTurret.TurretRotation.Roll);
			return OffsetRotation;
		}
		else
			return Rotation;
	}

//PWC 2003.04.14 Disabled turret firing to get regular firing working
    exec function Fire(optional float F)
    {
		local StationaryTurret DrivenTurret;
		DrivenTurret = StationaryTurret(Pawn);

		if(DrivenTurret != None)
		{

				DrivenTurret.Fire();
		}
    }

    exec function AltFire(optional float F)
    {
	/*	local StationaryTurret DrivenTurret;

		DrivenTurret = StationaryTurret(Pawn);
		if(DrivenTurret != None)
		{
			DrivenTurret.TurretFire(true);
			DrivenTurret.bTurretIsAltFiring = true;
		}*/
    }

	function TargetUse()
	{
		local StationaryTurret DrivenTurret;
		DrivenTurret = StationaryTurret(Pawn);
		if ( Level.Pauser == PlayerReplicationInfo )
		{
			SetPause(false);
			return;
		}

		if (DrivenTurret != None && Role == ROLE_Authority)
        {
			DrivenTurret.bGetOut = true;
            return;
		} else if ( DrivenTurret != None )
			ServerUse();

}

	// Set the throttle, steering etc. for the Turret based on the input provided
	function ProcessDrive(float InForward, float InStrafe, bool InJump, rotator DeltaRot)
	{
		local StationaryTurret DrivenTurret;

		DrivenTurret = StationaryTurret(Pawn);
        if(DrivenTurret == None)
        {
            log("PlayerDriving.PlayerMove: No Turret");
            return;
        }

        //log("Drive:"$InForward$" Steer:"$InStrafe);

        if(InForward > 1)
            DrivenTurret.Throttle = 1;
        else if(InForward < -1)
            DrivenTurret.Throttle = -1;
        else
            DrivenTurret.Throttle = 0;

        if(InStrafe < -1)
            DrivenTurret.Steering = 1;
        else if(InStrafe > 1)
            DrivenTurret.Steering = -1;
        else
            DrivenTurret.Steering = 0;
            
		DrivenTurret.TurretRotation = DeltaRot;
		DrivenTurret.MyTurretRotation = DeltaRot;
	}

    function PlayerMove( float DeltaTime )
		{
		local StationaryTurret DrivenTurret;
		local Rotator TempRot;

		UpdateRotation(DeltaTime, 2);
		TempRot=TempTurretRotation;

		// Only servers can actually do the driving logic.
		if(Role < ROLE_Authority)
			ServerDrive(aForward, aStrafe, bPressedJump, TempRot);
		else
			ProcessDrive(aForward, aStrafe, bPressedJump, TempRot);

		// If the Turret is being controlled here - set replicated variables.
		DrivenTurret = StationaryTurret(Pawn);
		if(DrivenTurret != None)
			{
			if(bFire == 0 && DrivenTurret.bTurretIsFiring)
			{
				DrivenTurret.TurretCeaseFire(false);
				DrivenTurret.bTurretIsFiring = false;
			}
			
			if(bAltFire == 0 && DrivenTurret.bTurretIsAltFiring)
		{
				DrivenTurret.TurretCeaseFire(true);
				DrivenTurret.bTurretIsAltFiring = false;
			}
		}

	}

	function BeginState()
	{
		//Pawn.weapon.putdown();
		CleanOutSavedMoves();
	}
	
	function EndState()
	{
		CleanOutSavedMoves();
		//Pawn.weapon.bringup();
	}
Begin:
}
// END NEW

// Player movement.
// Player walking on walls
state PlayerSpidering
{
ignores SeeEnemy, HearNoise, Bump;

	event bool NotifyHitWall(vector HitNormal, actor HitActor)
	{
		Pawn.SetPhysics(PHYS_Spider);
		Pawn.SetBase(HitActor, HitNormal);	
		return true;
	}

	// if spider mode, update rotation based on floor					
	function UpdateRotation(float DeltaTime, float maxPitch)
	{
        local rotator ViewRotation;
		local vector MyFloor, CrossDir, FwdDir, OldFwdDir, OldX, RealFloor;

		if ( bInterpolating || Pawn.bInterpolating )
		{
			ViewShake(deltaTime);
			return;
		}

		TurnTarget = None;
		bRotateToDesired = false;
		bSetTurnRot = false;

		if ( (Pawn.Base == None) || (Pawn.Floor == vect(0,0,0)) )
			MyFloor = vect(0,0,1);
		else
			MyFloor = Pawn.Floor;

		if ( MyFloor != OldFloor )
		{
			// smoothly change floor
			RealFloor = MyFloor;
			MyFloor = Normal(6*DeltaTime * MyFloor + (1 - 6*DeltaTime) * OldFloor);
			if ( (RealFloor Dot MyFloor) > 0.999 )
				MyFloor = RealFloor;

			// translate view direction
			CrossDir = Normal(RealFloor Cross OldFloor);
			FwdDir = CrossDir Cross MyFloor;
			OldFwdDir = CrossDir Cross OldFloor;
			ViewX = MyFloor * (OldFloor Dot ViewX) 
						+ CrossDir * (CrossDir Dot ViewX) 
						+ FwdDir * (OldFwdDir Dot ViewX);
			ViewX = Normal(ViewX);
			
			ViewZ = MyFloor * (OldFloor Dot ViewZ) 
						+ CrossDir * (CrossDir Dot ViewZ) 
						+ FwdDir * (OldFwdDir Dot ViewZ);
			ViewZ = Normal(ViewZ);
			OldFloor = MyFloor;  
			ViewY = Normal(MyFloor Cross ViewX); 
		}

		if ( (aTurn != 0) || (aLookUp != 0) )
		{
			// adjust Yaw based on aTurn
			if ( aTurn != 0 )
				ViewX = Normal(ViewX + 2 * ViewY * Sin(0.0005*DeltaTime*aTurn));

			// adjust Pitch based on aLookUp
			if ( aLookUp != 0 )
			{
				OldX = ViewX;
				ViewX = Normal(ViewX + 2 * ViewZ * Sin(0.0005*DeltaTime*aLookUp));
				ViewZ = Normal(ViewX Cross ViewY);

				// bound max pitch
				if ( (ViewZ Dot MyFloor) < 0.707   )
				{
					OldX = Normal(OldX - MyFloor * (MyFloor Dot OldX));
					if ( (ViewX Dot MyFloor) > 0)
						ViewX = Normal(OldX + MyFloor);
					else
						ViewX = Normal(OldX - MyFloor);

					ViewZ = Normal(ViewX Cross ViewY);
				}
			}
			
			// calculate new Y axis
			ViewY = Normal(MyFloor Cross ViewX);
		}
		ViewRotation =  OrthoRotation(ViewX,ViewY,ViewZ);
		SetRotation(ViewRotation);
		ViewShake(deltaTime);
		Pawn.FaceRotation(ViewRotation, deltaTime );
	}

	function bool NotifyLanded(vector HitNormal)
	{
		Pawn.SetPhysics(PHYS_Spider);
		return bUpdating;
	}

	function bool NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		if ( NewVolume.bWaterVolume )
			GotoState(Pawn.WaterMovementState);
		return false;
	}
	
	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{
		local vector OldAccel;

		OldAccel = Pawn.Acceleration;
		Pawn.Acceleration = NewAccel;

		if ( bPressedJump )
			Pawn.DoJump(bUpdating);
	}

	function PlayerMove( float DeltaTime )
	{
		local vector NewAccel;
		local eDoubleClickDir DoubleClickMove;
		local rotator OldRotation, ViewRotation;
		local bool	bSaveJump;

		GroundPitch = 0;	
		ViewRotation = Rotation;

		if ( !bKeyboardLook && (bLook == 0) && bCenterView )
		{
			// FIXME - center view rotation based on current floor
		}
		Pawn.CheckBob(DeltaTime,vect(0,0,0));

		// Update rotation.
		SetRotation(ViewRotation);
		OldRotation = Rotation;
		UpdateRotation(DeltaTime, 1);

		// Update acceleration.
		NewAccel = aForward*Normal(ViewX - OldFloor * (OldFloor Dot ViewX)) + aStrafe*ViewY; 
		if ( VSize(NewAccel) < 1.0 )
			NewAccel = vect(0,0,0);

		if ( bPressedJump && Pawn.CannotJumpNow() )
		{
			bSaveJump = true;
			bPressedJump = false;
		}
		else
			bSaveJump = false;

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
		else
			ProcessMove(DeltaTime, NewAccel, DoubleClickMove, OldRotation - Rotation);
		bPressedJump = bSaveJump;
	}

	function BeginState()
	{
		if ( Pawn.Mesh == None )
			Pawn.SetMesh();
		OldFloor = vect(0,0,1);
		GetAxes(Rotation,ViewX,ViewY,ViewZ);
		DoubleClickDir = DCLICK_None;
		Pawn.ShouldCrouch(false);
		bPressedJump = false;
		if (Pawn.Physics != PHYS_Falling) 
			Pawn.SetPhysics(PHYS_Spider);
		GroundPitch = 0;
		Pawn.bCrawler = true;
		Pawn.SetCollisionSize(Pawn.Default.CollisionHeight,Pawn.Default.CollisionHeight);
	}
	
	function EndState()
	{
		GroundPitch = 0;
		if ( Pawn != None )
		{
			Pawn.SetCollisionSize(Pawn.Default.CollisionRadius,Pawn.Default.CollisionHeight);
			Pawn.ShouldCrouch(false);
			Pawn.bCrawler = Pawn.Default.bCrawler;
		}
	}
}
	
// Player movement.
// Player Swimming
state PlayerSwimming
{
ignores SeeEnemy, HearNoise, Bump;

	function bool WantsSmoothedView()
	{
		return ( !Pawn.bJustLanded );
	}

	function bool NotifyLanded(vector HitNormal)
	{
		if ( Pawn.PhysicsVolume.bWaterVolume )
			Pawn.SetPhysics(PHYS_Swimming);
		else
			GotoState(Pawn.LandMovementState);
		return bUpdating;
	}
	
	function bool NotifyPhysicsVolumeChange( PhysicsVolume NewVolume )
	{
		local actor HitActor;
		local vector HitLocation, HitNormal, checkpoint;

		if ( !NewVolume.bWaterVolume )
		{
			Pawn.SetPhysics(PHYS_Falling);
            if ( Pawn.Velocity.Z > 0 )
            {
			    if (Pawn.bUpAndOut && Pawn.CheckWaterJump(HitNormal)) //check for waterjump
			    {
				    Pawn.velocity.Z = FMax(Pawn.JumpZ,420) + 2 * Pawn.CollisionRadius; //set here so physics uses this for remainder of tick
				    GotoState(Pawn.LandMovementState);
			    }				
			    else if ( (Pawn.Velocity.Z > 160) || !Pawn.TouchingWaterVolume() )
				    GotoState(Pawn.LandMovementState);
			    else //check if in deep water
			    {
				    checkpoint = Pawn.Location;
				    checkpoint.Z -= (Pawn.CollisionHeight + 6.0);
				    HitActor = Trace(HitLocation, HitNormal, checkpoint, Pawn.Location, false);
				    if (HitActor != None)
					    GotoState(Pawn.LandMovementState);
				    else
				    {
					    Enable('Timer');
					    SetTimer(0.7,false);
				    }
			    }
		    }
        }
		else
		{
			Disable('Timer');
			Pawn.SetPhysics(PHYS_Swimming);
		}
		return false;
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{
		local vector X,Y,Z, OldAccel;
	
		GetAxes(Rotation,X,Y,Z);
		OldAccel = Pawn.Acceleration;
		Pawn.Acceleration = NewAccel;
		Pawn.bUpAndOut = ((X Dot Pawn.Acceleration) > 0) && ((Pawn.Acceleration.Z > 0) || (Rotation.Pitch > 2048));
		if ( !Pawn.PhysicsVolume.bWaterVolume ) //check for waterjump
			NotifyPhysicsVolumeChange(Pawn.PhysicsVolume);
	}

	function PlayerMove(float DeltaTime)
	{
		local rotator oldRotation;
		local vector X,Y,Z, NewAccel;
	
		GetAxes(Rotation,X,Y,Z);

		NewAccel = aForward*X + aStrafe*Y + aUp*vect(0,0,1); 
		if ( VSize(NewAccel) < 1.0 )
			NewAccel = vect(0,0,0);
	
		//add bobbing when swimming
		Pawn.CheckBob(DeltaTime, Y);

		// Update rotation.
		oldRotation = Rotation;
		UpdateRotation(DeltaTime, 2);

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, NewAccel, DCLICK_None, OldRotation - Rotation);
		else
			ProcessMove(DeltaTime, NewAccel, DCLICK_None, OldRotation - Rotation);
		bPressedJump = false;
	}

	function Timer()
	{
		if ( !Pawn.PhysicsVolume.bWaterVolume && (Role == ROLE_Authority) )
			GotoState(Pawn.LandMovementState);
	
		Disable('Timer');
	}
	
	function BeginState()
	{
		Disable('Timer');
		Pawn.SetPhysics(PHYS_Swimming);
	}
}

state PlayerFlying
{
ignores SeeEnemy, HearNoise, Bump;

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;

		GetAxes(Rotation,X,Y,Z);

		//NEW (mdf) more intuitive movement
		Pawn.Acceleration = (aForward*X + aStrafe*Y + aUp*vect(0,0,1));  
		/*OLD
		Pawn.Acceleration = aForward*X + aStrafe*Y; 
		*/
		if ( VSize(Pawn.Acceleration) < 1.0 )
			Pawn.Acceleration = vect(0,0,0);

		if ( bCheatFlying )
		{
			if( CheatFlyingSpeed ~= 0.0 )
				CheatFlyingSpeed = 1.0;

			Pawn.AirSpeed = CheatFlyingSpeed*Pawn.default.AirSpeed;
				
			aUp *= 2.0;

			if( Pawn.Acceleration == vect(0,0,0) )
				Pawn.Velocity = vect(0,0,0);
		}

		// Update rotation.
		UpdateRotation(DeltaTime, 2);
		
		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, Pawn.Acceleration, DCLICK_None, rot(0,0,0));
		else
			ProcessMove(DeltaTime, Pawn.Acceleration, DCLICK_None, rot(0,0,0));
	}
	
	function BeginState()
	{
		Pawn.SetPhysics(PHYS_Flying);
	}

	event EndState()
	{
		if( Pawn != None )
			Pawn.AirSpeed = Pawn.default.AirSpeed;
	}
}

function bool IsSpectating()
{
	return false;
}

state BaseSpectating
{
	function bool IsSpectating()
	{
		return true;
	}

	function ProcessMove(float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot)	
	{
		Acceleration = NewAccel;
        MoveSmooth(SpectateSpeed * Normal(Acceleration) * DeltaTime);
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;

		if ( (Pawn(ViewTarget) != None) && (Level.NetMode == NM_Client) )
		{
			if ( Pawn(ViewTarget).bSimulateGravity )
				TargetViewRotation.Roll = 0;
			BlendedTargetViewRotation.Pitch = BlendRot(DeltaTime, BlendedTargetViewRotation.Pitch, TargetViewRotation.Pitch & 65535);
			BlendedTargetViewRotation.Yaw = BlendRot(DeltaTime, BlendedTargetViewRotation.Yaw, TargetViewRotation.Yaw & 65535);
			BlendedTargetViewRotation.Roll = BlendRot(DeltaTime, BlendedTargetViewRotation.Roll, TargetViewRotation.Roll & 65535);
		}
		GetAxes(Rotation,X,Y,Z);
	
		Acceleration = 0.02 * (aForward*X + aStrafe*Y + aUp*vect(0,0,1));  

		UpdateRotation(DeltaTime, 1);

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, Acceleration, DCLICK_None, rot(0,0,0));
		else
			ProcessMove(DeltaTime, Acceleration, DCLICK_None, rot(0,0,0));
	}
}

state Scripting
{
	// FIXME - IF HIT FIRE, AND NOT bInterpolating, Leave script
	exec function Fire( optional float F )
	{
	}

	exec function AltFire( optional float F )
	{
		Fire(F);
	}
}

function ServerViewNextPlayer()
{
    local Controller C, Pick;
    local bool bFound, bRealSpec, bWasSpec;
	local TeamInfo RealTeam;

    bRealSpec = PlayerReplicationInfo.bOnlySpectator;
    bWasSpec = !bBehindView && (ViewTarget != Pawn) && (ViewTarget != self);
    PlayerReplicationInfo.bOnlySpectator = true;
    RealTeam = PlayerReplicationInfo.Team;
	
	// view next player
	for ( C=Level.ControllerList; C!=None; C=C.NextController )
	{
        if ( Level.Game.CanSpectate(self,true,C) )
		{
			if ( Pick == None )
                Pick = C;
			if ( bFound )
			{
                Pick = C;
				break;
			}	
			else
                bFound = ( (RealViewTarget == C) || (ViewTarget == C) );
		}
	}
    PlayerReplicationInfo.Team = RealTeam;
	SetViewTarget(Pick);
    ClientSetViewTarget(Pick);
    if ( (ViewTarget == self) || bWasSpec )
		bBehindView = false;
	else
		bBehindView = true; //bChaseCam;
    ClientSetBehindView(bBehindView);
    PlayerReplicationInfo.bOnlySpectator = bRealSpec;
}

function ServerViewSelf()
{
	bBehindView = false;
    SetViewTarget(self);
    ClientSetViewTarget(self);
	ClientMessage(OwnCamera, 'Event');
}

function LoadPlayers()
{
	local int i;
	
	if ( GameReplicationInfo == None )
		return;
		
	for ( i=0; i<GameReplicationInfo.PRIArray.Length; i++ )
		GameReplicationInfo.PRIArray[i].UpdatePrecacheMaterials();
}

function FindGoodView()
{
	local vector cameraLoc;
	local rotator cameraRot, ViewRotation;
	local int tries, besttry;
	local float bestdist, newdist;
	local int startYaw;
	local actor ViewActor;
	
	ViewRotation = Rotation;
	ViewRotation.Pitch = 56000;
	tries = 0;
	besttry = 0;
	bestdist = 0.0;
	startYaw = ViewRotation.Yaw;
	
	for (tries=0; tries<16; tries++)
	{
		cameraLoc = ViewTarget.Location;
		SetRotation(ViewRotation);
		PlayerCalcView(ViewActor, cameraLoc, cameraRot);
		newdist = VSize(cameraLoc - ViewTarget.Location);
		if (newdist > bestdist)
		{
			bestdist = newdist;	
			besttry = tries;
		}
		ViewRotation.Yaw += 4096;
	}
		
	ViewRotation.Yaw = startYaw + besttry * 4096;
	SetRotation(ViewRotation);
}

state AttractMode extends BaseSpectating
{
}

state Spectating extends BaseSpectating
{
	ignores SwitchWeapon, RestartLevel, ClientRestart, Suicide,
	 ThrowWeapon, NotifyPhysicsVolumeChange, NotifyHeadVolumeChange;

	exec function Fire( optional float F )
	{
		ServerViewNextPlayer();
	}

	// Return to spectator's own camera.
	exec function AltFire( optional float F )
	{
		bBehindView = false;
		ServerViewSelf();
	}

	function BeginState()
	{
		if ( Pawn != None )
		{
			SetLocation(Pawn.Location);
			UnPossess();
		}
		bCollideWorld = true;
	}

	function EndState()
	{
		PlayerReplicationInfo.bIsSpectator = false;		
		bCollideWorld = false;
	}
}

auto state PlayerWaiting extends BaseSpectating
{
ignores SeeEnemy, HearNoise, NotifyBump, TakeDamage, PhysicsVolumeChange, NextWeapon, PrevWeapon, SwitchToBestWeapon;

	exec function Jump( optional float F )
	{
	}

	exec function Suicide()
	{
	}

	function ChangeTeam( int N )
	{
        Level.Game.ChangeTeam(self, N, true);
	}

    function ServerRestartPlayer()
	{
		if ( Level.TimeSeconds < WaitDelay )
			return;
		if ( Level.NetMode == NM_Client )
			return;
		if ( Level.Game.bWaitingToStartMatch )
			PlayerReplicationInfo.bReadyToPlay = true;
		else
			Level.Game.RestartPlayer(self);
	}

	exec function Fire(optional float F)
	{
        LoadPlayers();
		ServerReStartPlayer();
	}
	
	exec function AltFire(optional float F)
	{
        Fire(F);
	}

	function EndState()
	{
		if ( Pawn != None )
			Pawn.SetMesh();
        if ( PlayerReplicationInfo != None )
		PlayerReplicationInfo.SetWaitingPlayer(false);
		bCollideWorld = false;
	}

	function BeginState()
	{
		if ( PlayerReplicationInfo != None )
			PlayerReplicationInfo.SetWaitingPlayer(true);
		bCollideWorld = true;
	}
}

state WaitingForPawn extends BaseSpectating
{
ignores SeeEnemy, HearNoise, KilledBy, SwitchWeapon;

	exec function Fire( optional float F )
	{
		AskForPawn();
	}

	exec function AltFire( optional float F )
	{
	}

	function LongClientAdjustPosition
	(
		float TimeStamp, 
		name newState, 
		EPhysics newPhysics,
		float NewLocX, 
		float NewLocY, 
		float NewLocZ, 
		float NewVelX, 
		float NewVelY, 
		float NewVelZ,
		Actor NewBase,
		float NewFloorX,
		float NewFloorY,
		float NewFloorZ
	)
	{
		if ( newState == 'GameEnded' )
			GotoState(newState);
	}

	function PlayerTick(float DeltaTime)
	{
		Global.PlayerTick(DeltaTime);

		if ( Pawn != None )
		{
			Pawn.Controller = self;
            Pawn.bUpdateEyeHeight = true;
			ClientRestart();
		}
        else if ( (TimerRate <= 0.0) || (TimerRate > 1.0) )
		{
			SetTimer(0.2,true);
			AskForPawn();
		}
	}

	function Timer()
	{
		AskForPawn();
	}

	function BeginState()
	{
		SetTimer(0.2, true);
        AskForPawn();
	}

	function EndState()
	{
		bBehindView = false;
		SetTimer(0.0, false);
	}
}

state GameEnded
{
ignores SeeEnemy, HearNoise, KilledBy, NotifyBump, HitWall, NotifyHeadVolumeChange, NotifyPhysicsVolumeChange, Falling, TakeDamage, Suicide;

	function ServerReStartPlayer()
	{
	}
	
	function bool IsSpectating()
	{
		return true;
	}
	
	exec function ThrowWeapon()
	{
	}

	function ServerReStartGame()
	{
		Level.Game.RestartGame();
	}

	exec function Fire( optional float F )
	{
		if ( Role < ROLE_Authority)
			return;
		if ( !bFrozen )
			ServerReStartGame();
		else if ( TimerRate <= 0 )
			SetTimer(1.5, false);
	}
	
	exec function AltFire( optional float F )
	{
		Fire(F);
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;
		local Rotator ViewRotation;

		GetAxes(Rotation,X,Y,Z);
		// Update view rotation.

		if ( !bFixedCamera )
		{
			ViewRotation = Rotation;
			ViewRotation.Yaw += 32.0 * DeltaTime * aTurn;
			ViewRotation.Pitch += 32.0 * DeltaTime * aLookUp;
			ViewRotation.Pitch = ViewRotation.Pitch & 65535;
			If ((ViewRotation.Pitch > 18000) && (ViewRotation.Pitch < 49152))
			{
				If (aLookUp > 0) 
					ViewRotation.Pitch = 18000;
				else
					ViewRotation.Pitch = 49152;
			}
			SetRotation(ViewRotation);
		}
		else if ( ViewTarget != None )
			SetRotation(ViewTarget.Rotation);

		ViewShake(DeltaTime);

		if ( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		else
			ProcessMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		bPressedJump = false;
	}

	function ServerMove
	(
		float TimeStamp, 
		vector InAccel, 
		vector ClientLoc,
		bool NewbRun,
		bool NewbSprint,
		bool NewbDuck,
		bool NewbJump, 
		bool NewbJumpStatus,
		bool NewbDoubleJump, 
		eDoubleClickDir DoubleClickMove, 
		byte ClientRoll, 
		int View,
		optional byte OldTimeDelta,
		optional int OldAccel
	)
	{
		Global.ServerMove(TimeStamp, InAccel, ClientLoc, NewbRun, NewbSprint, NewbDuck, NewbJump, NewbJumpStatus, NewbDoubleJump,
							DoubleClickMove, ClientRoll, (32767 & (Rotation.Pitch/2)) * 32768 + (32767 & (Rotation.Yaw/2)) );

	}

	function Timer()
	{
		bFrozen = false;
	}
	
	function LongClientAdjustPosition
	(
		float TimeStamp, 
		name newState, 
		EPhysics newPhysics,
		float NewLocX, 
		float NewLocY, 
		float NewLocZ, 
		float NewVelX, 
		float NewVelY, 
		float NewVelZ,
		Actor NewBase,
		float NewFloorX,
		float NewFloorY,
		float NewFloorZ
	)
	{   
	}
		 
	function BeginState()
	{
		local Pawn P;

		EndZoom();
        FOVAngle = DesiredFOV;
		bFire = 0;
		bAltFire = 0;
		if ( Pawn != None )
		{
			Pawn.Velocity = vect(0,0,0);
			Pawn.SetPhysics(PHYS_None);
			Pawn.AmbientSound = None;
			//!!MERGE Pawn.bSpecialHUD = false;
 			Pawn.bNoWeaponFiring = true;
			Pawn.SimAnim.AnimRate = 0;
			Pawn.bPhysicsAnimUpdate = false;
            Pawn.bWaitForAnim = false; //NEW (mdf)
			Pawn.StopAnimating();
            Pawn.SetCollision(true,false,false);
            StopFiring();
 			Pawn.bIgnoreForces = true;
		}
		//!!MERGE myHUD.bShowScores = true;
		bFrozen = true;
		if ( !bFixedCamera )
		{
			FindGoodView();
			bBehindView = true;
		}
        SetTimer(5, false);
		ForEach DynamicActors(class'Pawn', P)
		{
			if ( P.Role == ROLE_Authority )
				P.RemoteRole = ROLE_DumbProxy;
			P.SetCollision(true,false,false);
			P.AmbientSound = None;
 			P.bNoWeaponFiring = true;
			P.Velocity = vect(0,0,0);
			P.SetPhysics(PHYS_None);
            P.bPhysicsAnimUpdate = false;
            P.bWaitForAnim = false;
            P.StopAnimating();
            P.bIgnoreForces = true;
		}
	}

Begin:
}

state Dead
{
ignores SeeEnemy, HearNoise, KilledBy, SwitchWeapon, NextWeapon, PrevWeapon;

	function bool IsDead()
	{
		return true;
	}

	function ServerReStartPlayer()
	{
		Super.ServerRestartPlayer();
	}

	exec function Fire( optional float F )
	{
		if ( bFrozen )
		{
			if ( (TimerRate <= 0.0) || (TimerRate > 1.0) )
				bFrozen = false;
			return;
		}
        LoadPlayers();
		ServerReStartPlayer();
	}
	
	exec function AltFire( optional float F )
	{
			Fire(F);
	}

	function ServerMove
	(
		float TimeStamp, 
		vector Accel, 
		vector ClientLoc,
		bool NewbRun,
		bool NewbSprint,
		bool NewbDuck,
		bool NewbJump,
		bool NewbJumpStatus,
		bool NewbDoubleJump,
		eDoubleClickDir DoubleClickMove, 
		byte ClientRoll, 
		int View,
		optional byte OldTimeDelta,
		optional int OldAccel
	)
	{
		Global.ServerMove(
					TimeStamp,
					Accel, 
					ClientLoc,
					false,
					false,
					false,
					false,
					false,
					false,
					DoubleClickMove, 
					ClientRoll, 
					View);
	}

	function PlayerMove(float DeltaTime)
	{
		local vector X,Y,Z;
		local rotator ViewRotation;

		if ( !bFrozen )
		{
			if ( bPressedJump )
			{
				Fire(0);
				bPressedJump = false;
			}
			GetAxes(Rotation,X,Y,Z);
			// Update view rotation.
			ViewRotation = Rotation;
			ViewRotation.Yaw += 32.0 * DeltaTime * aTurn;
			ViewRotation.Pitch += 32.0 * DeltaTime * aLookUp;
			ViewRotation.Pitch = ViewRotation.Pitch & 65535;
			If ((ViewRotation.Pitch > 18000) && (ViewRotation.Pitch < 49152))
			{
				If (aLookUp > 0) 
					ViewRotation.Pitch = 18000;
				else
					ViewRotation.Pitch = 49152;
			}
			SetRotation(ViewRotation);
			if ( Role < ROLE_Authority ) // then save this move and replicate it
				ReplicateMove(DeltaTime, vect(0,0,0), DCLICK_None, rot(0,0,0));
		}
        else if ( (TimerRate <= 0.0) || (TimerRate > 1.0) )
			bFrozen = false;

		ViewShake(DeltaTime);
	}

	function Timer()
	{
		if (!bFrozen)
			return;
			
		bFrozen = false;
		bPressedJump = false;
	}

	function BeginState()
	{
		if ( (Pawn != None) && (Pawn.Controller == self) )
			Pawn.Controller = None;
		EndZoom();
		FOVAngle = DesiredFOV;
		Pawn = None;
		Enemy = None;
		bBehindView = true;
		bFrozen = true;
		bJumpStatus = false;
		bPressedJump = false;
		bBlockCloseCamera = true;
		bValidBehindCamera = false;
		FindGoodView();
		SetTimer(1.0, false); 
		StopForceFeedback();
		ClientPlayForceFeedback("Damage");  // jdf
		CleanOutSavedMoves();
	}

	function EndState()
	{
		bBlockCloseCamera = false;
		CleanOutSavedMoves();
		Velocity = vect(0,0,0);
		Acceleration = vect(0,0,0);
        if ( !PlayerReplicationInfo.bOutOfLives )
		bBehindView = false;
		bPressedJump = false;
		//!!MERGE myHUD.bShowScores = false;
}
Begin:
    Sleep(3.0);
//!!MERGE    myHUD.bShowScores = true;
}

//------------------------------------------------------------------------------

state CantMove
{
	//-------------------------------------------------------------------------

	event PlayerTick( float DeltaTime )
	{
		Global.PlayerTick( DeltaTime );

		if( bUpdatePosition )
			ClientUpdatePosition();

		PlayerMove( DeltaTime );
	}

	//-------------------------------------------------------------------------

	function PlayerMove( float DeltaTime )
	{
		local vector NewAccel;

		ViewShake( DeltaTime );

		// Update rotation.
		UpdateRotation( DeltaTime, 2 );

		// prevent movement		
		NewAccel = vect(0,0,0);

		if( Role < ROLE_Authority ) // then save this move and replicate it
			ReplicateMove( DeltaTime, NewAccel, DCLICK_None, Rot(0,0,0) );
		else if( Pawn != None )
			Pawn.Acceleration = NewAccel;
	}

	//-------------------------------------------------------------------------

	event BeginState()
	{
		bPressedJump = false;
	}

	//-------------------------------------------------------------------------

} // CantMove

//-----------------------------------------------------------------------------
// Needed to support teleport cheat.

simulated function DoUnTeleport();
state TeleportFixed extends PlayerWalking
{
	//--------------------------------------------------------------------------

	function ProcessMove( float DeltaTime, vector NewAccel, eDoubleClickDir DoubleClickMove, rotator DeltaRot );	

	//-------------------------------------------------------------------------

	simulated function DoUnTeleport()
	{
		if( SavedPhysics == PHYS_Flying )
		{
			GotoState( 'PlayerFlying' );
		}
		else if( Pawn.PhysicsVolume.bWaterVolume )
		{
			Pawn.SetPhysics( PHYS_Swimming );
			GotoState( Pawn.WaterMovementState );
		}
		else
		{
			GotoState( 'PlayerWalking');
		}

		SavedPhysics = PHYS_None;
	}

	//-------------------------------------------------------------------------

	event BeginState()
	{
		// save off old physics in case player using fly cheat
		if( SavedPhysics == PHYS_None )
			SavedPhysics = Pawn.Physics;
		Pawn.SetPhysics( PHYS_None );
	}
} // TeleportFixed

//------------------------------------------------------------------------------

state Wounded extends CantMove
{
	// implemented in derived classes
} // Wounded


//------------------------------------------------------------------------------
// Control options	
function ChangeStairLook( bool B )
{
	bLookUpStairs = B;
	if ( bLookUpStairs )
		bAlwaysMouseLook = false;
}

function ChangeAlwaysMouseLook(Bool B)
{
	bAlwaysMouseLook = B;
	if ( bAlwaysMouseLook )
		bLookUpStairs = false;
}

function bool CanRestartPlayer()
{
    return !PlayerReplicationInfo.bOnlySpectator;
}

event ServerChangeVoiceChatter( PlayerController Player, int IpAddr, int Handle, bool Add )
{
	if( (Level.NetMode == NM_DedicatedServer) || (Level.NetMode == NM_ListenServer) )
	{
		Level.Game.ChangeVoiceChatter( Player, IpAddr, Handle, Add );
	}
}

event ServerGetVoiceChatters( PlayerController Player )
{
	local int i;
	
	if( (Level.NetMode == NM_DedicatedServer) || (Level.NetMode == NM_ListenServer) )
	{
		for( i=0; i<Level.Game.VoiceChatters.Length; i++ )
		{
			if( Player != Level.Game.VoiceChatters[i].Controller )
			{
				Player.ClientChangeVoiceChatter( Level.Game.VoiceChatters[i].IpAddr, Level.Game.VoiceChatters[i].Handle, true );
			}
		}
	}
}

simulated function ClientChangeVoiceChatter( int IpAddr, int Handle, bool Add )
{
	ChangeVoiceChatter( IpAddr, Handle, Add );
}
	
simulated function ClientLeaveVoiceChat()
{
	LeaveVoiceChat();
}
	
native final function LeaveVoiceChat();
native final function ChangeVoiceChatter( int IpAddr, int Handle, bool Add );
	
//--------------------- Demo recording stuff

// Called on the client during client-side demo recording
simulated event StartClientDemoRec()
{
	// Here we replicate functions which the demo never saw.
	//DemoClientSetHUD( HUD, Scoreboard );
	
	// tell server to replicate more stuff to me
	bClientDemo = true;
	ServerSetClientDemo();
}

function ServerSetClientDemo()
{
	bClientDemo = true;
}

// Called on the playback client during client-side demo playback
simulated function DemoClientSetHUD(name NewHUD, name NewScoreboard)
{
	ClientSetHUD( NewHUD, NewScoreboard );
}

defaultproperties
{
	AimingHelp=0.0
	OrthoZoom=+40000.000000
	FlashScale=(X=1.000000,Y=1.000000,Z=1.000000)
	AnnouncerVolume=4
	FOVAngle=85.000
	DesiredFOV=85.000000
	DefaultFOV=85.000000
	bAlwaysMouseLook=true
	bAutoSwitch=true
	ViewingFrom="Now viewing from"
	OwnCamera="Now viewing from own camera"
	QuickSaveString="Quick Saving"
	NoPauseMessage="Game is not pauseable"
	bTravel=True
	bStasis=False
	NetPriority=3
	MaxTimeMargin=+0.35
	LocalMessageClass=class'LocalMessage'
	bIsPlayer=true
	bCanOpenDoors=true
	bCanDoSpecial=true
	Physics=PHYS_None
	EnemyTurnSpeed=45000
	CheatClass=class'Engine.CheatManager'
	InputClass=class'Engine.PlayerInput'
	CameraDist=+9.0
	bZeroRoll=true
	bDynamicNetSpeed=true
	// jdf ---
	bEnableWeaponForceFeedback=True
	bEnablePickupForceFeedback=True
	bEnableDamageForceFeedback=True
	bEnableGUIForceFeedback=True
	bForceFeedbackSupported=True
	// --- jdf
	ProgressTimeOut=8.0
	MaxResponseTime=0.7
	SpectateSpeed=+600.0
	DynamicPingThreshold=+400.0
	ClientCap=0
	PassengerNumber=-1
	UseDistance=350.0
	UseLatency=0.1
	ShakePeriodX=31.0
	ShakePeriodY=29.0
	LoadingMusic(0)="DutyCallsA"
	LoadingMusic(1)="DutyCallsB"
	LoadingMusic(2)="m09f_outro"
	LoadingMusic(3)="marsh_outro"
	LoadingMusic(4)="PA_AVALONB_Music"
	LoadingMusic(5)="PA_SHORT_Music"
	LoadingMusic(6)="PAHELL"
	LoadingMusic(7)="PDHELL2"
}
