//=============================================================================
// Artifact.uc
//=============================================================================

class Artifact extends Inventory
	abstract;

//-----------------------------------------------------------------------------

var int	   ID;							// unique ID number for tracking by the game
var string ArtifactIconName;			// UI event name for display artifact icon on hud
var sound  ActivateSound;
var sound  DeActivateSound;

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && (Role==ROLE_Authority) )
		ID;
}

//-----------------------------------------------------------------------------

function int GetTeam()
{
	return XMPGame(Level.Game).GetArtifactTeam(ID);
}

//-----------------------------------------------------------------------------

function bool HandlePickupQuery( Pickup Item )
{
	// let the pickup decide whether it's either gonna get re-stolen or sent home
	return false;
}

//-----------------------------------------------------------------------------

function NotifyAddInventory( Pawn Other )
{
	AttachToPawn(Other);
	Other.PlayerReplicationInfo.SetArtifact(Self);
	Super.NotifyAddInventory(Other);
}

//-----------------------------------------------------------------------------

simulated function ClientAddInventory( Pawn Other )
{
	Other.PlayOwnedSound( ActivateSound, SLOT_None );
	class'UIConsole'.static.SendEvent("ArtifactTrayShow");
	class'UIConsole'.static.SendEvent( ArtifactIconName );
}

//-----------------------------------------------------------------------------

function NotifyDeleteInventory( Pawn Other )
{
	DetachFromPawn(Other);
	if( Other.PlayerReplicationInfo? )
		Other.PlayerReplicationInfo.SetArtifact(None);
	Super.NotifyDeleteInventory(Other);
}

//-----------------------------------------------------------------------------

simulated function ClientDeleteInventory( Pawn Other )
{
	Other.PlayOwnedSound( DeActivateSound );
	class'UIConsole'.static.SendEvent("ArtifactTrayHide");
}

//-----------------------------------------------------------------------------

function DumpArtifacts()
{
	DMTN("ID:" @ ID @ "Team:" @ XMPGame(Level.Game).GetArtifactTeam(ID) );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bTravel=False
	bAlwaysRelevant=True
}
