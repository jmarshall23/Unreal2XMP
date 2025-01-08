//=============================================================================
// ArtifactPickup.uc
//=============================================================================
class ArtifactPickup extends Pickup
	abstract;

//-----------------------------------------------------------------------------

#exec OBJ LOAD File=..\Particles\Artifact_Carry_FX.u

//-----------------------------------------------------------------------------

var() int TeamNumber;				// track which team currently has it registered
var int ID;							// unique ID for tracking by the game
var bool bShouldSendHome;			// whether to let the artifact be picked up or sent home
var Pawn PawnToRecoverArtifact;		// pawn that sent the artifact home (none if timed out)
var material FlashySkin[2];
var ParticleGenerator PigPen;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && (Role==ROLE_Authority) )
		TeamNumber;
}

//-----------------------------------------------------------------------------

simulated event PreBeginPlay()
{
	Super.PreBeginPlay();
	if( Role < ROLE_Authority )
		WaitForTeam();
}

//-----------------------------------------------------------------------------

event PostBeginPlay()
{
	Super.PostBeginPlay();
	// tossed artifacts from players don't need a new id
	if ( Level.bStartup )
		ID = XMPGame( Level.Game ).GetArtifactID( Self );
}

//-----------------------------------------------------------------------------

simulated event Destroyed()
{
	if( Role == ROLE_Authority )
	{
		if( bShouldSendHome )
			SendHome();
	}

	if (PigPen?)
	{
		PigPen.ParticleDestroy();
		PigPen = None;
	}

	Super.Destroyed();
}

//-----------------------------------------------------------------------------

simulated function int GetTeam() { return TeamNumber; }
function SetTeam( int NewTeam )
{ 
	TeamNumber = NewTeam;
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
}

//-----------------------------------------------------------------------------

function Inventory SpawnCopy( Pawn Other )
{
	local Inventory Inv;
	Inv = Super.SpawnCopy( Other );
	if( Artifact(Inv) != None )	// pass artifact ID on to inventory item
		Artifact(Inv).ID = ID;
	return Inv;
}

//-----------------------------------------------------------------------------

function AnnouncePickup( Pawn Receiver )
{
//!!MERGE - use LocalizedMessages?
//	if( Receiver != None && Receiver.IsRealPlayer() )
//		class'UIConsole'.static.BroadcastStatusMessage( Self, PickupMessage );

	XMPGame(Level.Game).NotifyArtifactPickedup( ID, Receiver );

	Super.AnnouncePickup( Receiver );
}

//-----------------------------------------------------------------------------

function SendHome()
{
	XMPGame(Level.Game).NotifyArtifactSentHome( ID, PawnToRecoverArtifact );
}

//-----------------------------------------------------------------------------

simulated function CreateSmokeTrail()
{
	if( TeamNumber == 255 )
		return;

	if (TeamNumber == 0)
		PigPen = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Artifact_Carry_FX.ParticleSalamander1', Location );
	else
		PigPen = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'Artifact_Carry_FX.ParticleSalamander2', Location );
	PigPen.bOn = false;
	PigPen.IdleTime=0;
	PigPen.SetOwner(Self);
	PigPen.bHardAttach=true;
	PigPen.SetBase(Self);
}

//-----------------------------------------------------------------------------

simulated event Landed( vector HitNormal )
{
	Super.Landed( HitNormal );
	if( PigPen? )
	{
		PigPen.bOn = true;
		PigPen.Trigger(Self,Instigator);
	}
}

//-----------------------------------------------------------------------------
// Common touch code for both FallingPickup and Pickup states

function HandleTouch( actor Other )
{
	local Inventory Copy;

	// If toucher already has an artifact, and he's not stealing it,
	// let him return it back home

	//Log( Self @ "ID:" @ ID @ "team:" @ XMPGame(Level.Game).GetArtifactTeam(ID) );
	//Log( "Touched by:" @ Other @ "team:" @ Other.GetTeam() );

	// Toucher is on the same team as artifact -- send it home
	if( XMPGame(Level.Game).GetArtifactTeam(ID) == Other.GetTeam() )
	{
		//Log( "Artifact:" @ ID @ "sent home by:" @ Other );
		PawnToRecoverArtifact = Pawn(Other);
		Destroy();
	}
	else
	{
		// Toucher is on the stealing team
		if( Pawn(Other)? && !Pawn(Other).PlayerReplicationInfo.bHasArtifact )
		{
			// doesn't have artifact, let him pick it up
			//Log("Letting:" @ Other @ "pickup artifact:" @ ID );
			bShouldSendHome = false;

			// copied from Pickup.Touch
			Copy = SpawnCopy(Pawn(Other));
			AnnouncePickup(Pawn(Other));
            SetRespawn(); 
            if ( Copy != None )
			Copy.PickupFunction(Pawn(Other));
		}
	}
}

//-----------------------------------------------------------------------------

auto state Pickup
{
	function Touch( actor Other )
	{
		if( !ValidTouch(Other) || bDeleteMe )
			return;

		HandleTouch( Other );
	}
}

//-----------------------------------------------------------------------------

state FallingPickup
{
	function Touch( actor Other )
	{
		if( !ValidTouch(Other) || bDeleteMe )
			return;

		HandleTouch( Other );
	}

	// Make sure no pawn already touching (while touch was disabled in sleep).
	function CheckTouching()
	{
		local Pawn P;
		ForEach TouchingActors(class'Pawn', P)
			Touch(P);
	}

	function Timer()
	{
	}

	function BeginState()
	{
	}

}

//-----------------------------------------------------------------------------

simulated function WaitForTeam()
{
	if( bPendingDelete )
		return;

	if( TeamNumber != 255 )
	{
		CreateSmokeTrail();
		return;
	}

	AddTimer('WaitForTeam', 0.1, false);
}

//-----------------------------------------------------------------------------

function DumpArtifacts()
{
	DMTN("Artifact ID:" @ ID @ "Team:" @ GetTeam() );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	TeamNumber=255
	FadeTimeout=30;
	bShouldSendHome=true
	bUnlit=true
}
