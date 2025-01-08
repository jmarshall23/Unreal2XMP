//=============================================================================
// HackTrigger.uc
//=============================================================================

class HackTrigger extends Actor
	native
	placeable;

//-----------------------------------------------------------------------------

#exec Texture Import File=Textures\S_HackTrigger.pcx Name=S_HackTrigger Mips=Off MASKED=1

//-----------------------------------------------------------------------------

const MaxTeams = 4;

//-----------------------------------------------------------------------------

var() bool bKeepProgress;				// do not remove progress bookkeeping when hacked
var() bool bIgnoreAlreadyHacked;		// don't re-dispatch events when hack team re-uses us.
var() bool bTriggerTeam;				// whether to call SetTeam on all Event targets
var() float TotalHackSeconds;			// total time needed to hack the object to a team
var() string Description;				// shows up on the hud
var() float DecayRate;					// rate at which hacking progress decays
var() int Team;							// -1 for neutral
var() sound HackStartSound;				// sound when hacking begins
var() sound HackAmbientSound;			// sound while hacking
var() sound HackCompletedSound;			// sound when trigger is fully hacked
var() sound AlreadyHackedSound;			// sound when a fully hacked trigger is used by hacked team

var class<LocalMessage> HackGainedMessage;	// game message for hack team
var class<LocalMessage> HackLostMessage;	// game message for losing team

var float TeamProgress[MaxTeams];		// time this object has been hacked by various teams
var array<Controller> Users;			// list of players currently using me
var array<float> UserProgress;			// individual player progress (for scoring)

//-----------------------------------------------------------------------------

replication
{
	// Things the server should send to the client.
	unreliable if( bNetDirty && Role==ROLE_Authority )
        TeamProgress, Team;
}

//-----------------------------------------------------------------------------

event PostBeginPlay()
{
	Super.PostBeginPlay();
	SetTeam( GetTeam() );		// initialize team skins
}

//-----------------------------------------------------------------------------

function bool IsHacking( Controller C )
{
	return GetUserIndex(C) != -1;
}

//-----------------------------------------------------------------------------

function int GetUserIndex( Controller C )
{
	local int i;
	for( i=0; i < Users.Length; i++ )
		if( Users[i] == C )
			return i;
	return -1;
}

//-----------------------------------------------------------------------------

simulated function int GetUserTeam( Controller C )
{
	return Clamp( C.GetTeam(), 0, MaxTeams-1 );
}

//-----------------------------------------------------------------------------

function AddUser( Controller C )
{
	if( GetUserIndex( C ) < 0 )
	{
		Users.Insert(0,1);
		Users[0] = C;
		UserProgress.Insert(0,1);
		UserProgress[0] = 0.0;
	}
	else
		DM("ERROR -- attempted to add player" @ C @ "more than once to hack list for" @ Self );
}

//-----------------------------------------------------------------------------

function RemoveUser( Controller C )
{
	local int Index;
	Index = GetUserIndex( C );
	if( Index >= 0 && Index < Users.Length )
	{
		Users.Remove(Index,1);
		UserProgress.Remove(Index,1);
	}
}

//-----------------------------------------------------------------------------

function OnUse( Actor Other )
{
	local Controller C;

	C = Controller(Other);
	if( C? && C.ValidPawn(C.Pawn) )
	{
		if( Team == GetUserTeam(C) )
		{
			HandleAlreadyHacked();
		}
		else
		{
			AddUser( C );
			AmbientSound = HackAmbientSound;
			PlaySound( HackStartSound );
	//		Enable('Tick');
		}
	}
}

//-----------------------------------------------------------------------------

function OnUnuse( Actor Other )
{
	local int i, Count, OtherTeam;
	local Controller C;

	C = Controller(Other);
	if( C == None || GetUserIndex(C) < 0 )
		return;

	RemoveUser( C );

	if( Users.Length == 0 )
		AmbientSound = None;

	// count teammates using still using the trigger
	OtherTeam = GetUserTeam(C);
	for( i=0; i < Users.Length; i++ )
		if( OtherTeam == GetUserTeam(Users[i]) )
			Count++;

	// fire off untrigger event and reset progress only 
	// if the last member of the hack team unuses us.
	if( Count == 0 )
	{
		UnDispatch();
		if( !bKeepProgress )
			SetTeam( 255 );
	}
}

//-----------------------------------------------------------------------------

event Tick( float DeltaTime )
{
	local int i, UserTeam;
	local U2Pawn UserPawn;
	local float NeededSkill;
	local float Hackage;

	// Add hacking progress for all current users
	for( i=0; i < Users.Length; i++ )
	{
		UserPawn = U2Pawn(Users[i].Pawn);
		if( class'Pawn'.static.ValidPawn(UserPawn) )
		{
			NeededSkill = UserPawn.HackCost * UserPawn.HackRate * DeltaTime;
			if( UserPawn.HasNeededSkill( NeededSkill ) )
			{
				UserTeam = GetUserTeam( Users[i] );
				if( UserTeam != Team )
				{
					Hackage = UserPawn.HackRate * DeltaTime;
					TeamProgress[UserTeam] += Hackage;
					UserProgress[i] += Hackage;
					UserPawn.UseSkill( NeededSkill );
				}
			}
		}
		else Users.Remove(i,1);
	}

	// figure if any hackers have succeeded
	if( Users.Length > 0 )
		for( i=0; i < ArrayCount(TeamProgress); i++ )
			if( TeamProgress[i] > TotalHackSeconds )
				HandleHacked( i );

	// decay progress a little
	//!!FIXME turn this off when no more users or progress
	for( i=0; i < ArrayCount(TeamProgress); i++ )
	{
		TeamProgress[i] = TeamProgress[i] - DecayRate * DeltaTime;
		if( TeamProgress[i] < 0 ) 
			TeamProgress[i] = 0.0;
	}

	// if there are no more users and all progress has been lost, go to sleep
//	if( Users.Length == 0 )
//		Disable('Tick');

}

//-----------------------------------------------------------------------------

function HandleAlreadyHacked()
{
	if( !bIgnoreAlreadyHacked )
		Dispatch();
	PlaySound( AlreadyHackedSound );
}

//-----------------------------------------------------------------------------

function HandleHacked( int NewHackedTeam )
{
	local int i;
    local Controller C, PrimaryHacker, SecondaryHacker;
	local float PrimaryHackage, SecondaryHackage;

	if( HackLostMessage? )
	{
		for( C=Level.ControllerList; C!=None; C=C.nextController )
			if( C.IsRealPlayer() && C.GetTeam() == GetTeam() )
			{
                Level.Game.BroadcastLocalizedTeam( C, HackLostMessage );
				break;
			}
	}

	SetTeam( NewHackedTeam );

	// toss out the opposing team's users
	for( i=0; i < Users.Length; i++ )
		if( GetUserTeam( Users[i] ) != Team )
			Users.Remove(i,1);

	// only hacked team users left, find the h4XorS
	for( i=0; i < UserProgress.Length; i++ )
	{
		if( UserProgress[i] > PrimaryHackage )
		{
			SecondaryHackage = PrimaryHackage;
			SecondaryHacker = PrimaryHacker;
			PrimaryHackage = UserProgress[i];
			PrimaryHacker = Users[i];
		}
		else if( UserProgress[i] > SecondaryHackage )
		{
			SecondaryHackage = UserProgress[i];
			SecondaryHacker = Users[i];
		}
	}
	RegisterHackScore( PrimaryHacker, SecondaryHacker );

	// clear the progress table, we've been fully hacked
	for( i=0; i < ArrayCount(TeamProgress); i++ )
		TeamProgress[i] = 0;

	if( HackGainedMessage? )
		Level.Game.BroadcastLocalizedTeam( PrimaryHacker, HackGainedMessage );

	PlaySound( HackCompletedSound );

	AmbientSound = None;

	Dispatch();

	//DM("I've been hacked by team:" @ Team );
}

//-----------------------------------------------------------------------------

function Dispatch()
{
	TriggerEvent( Event, Self, Instigator );
}

//-----------------------------------------------------------------------------

function UnDispatch()
{
	UntriggerEvent( Event, Self, Instigator );
}

//-----------------------------------------------------------------------------

function RegisterHackScore( Controller PrimaryHacker, Controller SecondaryHacker )
{
	XMPGame(Level.Game).NotifyHackedOther( PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

simulated function int GetTeam() { return Team; }
function SetTeam( int NewTeam )
{
	local int i;
	local Actor A;

	Team = NewTeam;
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );

	if( bTriggerTeam && Event != '' )
	{
		// hack alert: set the team skins of use proxies (they are stored as netdependents)
		for( i=0; i < NetDependents.Length; i++ )
			NetDependents[i].SetTeam( NewTeam );

		// hack alert++: make team triggers have the same team
		foreach AllActors( class'Actor', A, Event )
			A.SetTeam( Team );
	}
}

//-----------------------------------------------------------------------------
// UI accessors

simulated function float GetTeamProgress( Controller C )
{
	if( Team == GetUserTeam(C) )
		return 1.0;
	else
		return TeamProgress[GetUserTeam(C)] / TotalHackSeconds;
}

//-----------------------------------------------------------------------------

interface function bool   HasUseBar( Controller User ) { return bUsable; }
interface function float  GetUsePercent( Controller User ) { if( bUsable ) return GetTeamProgress(User); else return 1.0; }
interface function int    GetUseIconIndex( Controller User ) { return 1; }
interface function string GetDescription( Controller User )
{
	local string HackState;

	if( bUsable )
	{
		if( GetUserTeam(User) == Team )
			HackState = "Owned";
		else
			HackState = "Hack:" @ GetTeamProgress(User) * 100.0;

		return Description $ ":" @ HackState;
	}
	else return "";
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	Description="HackTrigger"
	HackStartSound=Sound'U2A.Hacking.HackStart'
	HackAmbientSound=Sound'U2A.Hacking.HackAmbient'
	HackCompletedSound=Sound'U2A.Hacking.HackCompleted'
	AlreadyHackedSound=Sound'U2A.Hacking.AlreadyHacked'

	bUsable=true
	bStaticLighting=true
	bShadowCast=true
	bCollideActors=true
	bProjTarget=true
	bBlockNonZeroExtentTraces=false
	Team=255
	DecayRate=0.4
	TotalHackSeconds=3.0
	bKeepProgress=true
	bTriggerTeam=false
}
