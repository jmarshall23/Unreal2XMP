//=============================================================================
// ArtifactHolder.uc
//=============================================================================

class ArtifactHolder extends Decoration
	abstract;

//-----------------------------------------------------------------------------

var() int TeamIndex;						// number of team to which the holder belongs
var() bool bStored;							// whether the holder currently has an artifact
var string Description;						// Base description of holder shown on hud
var class<Pickup>  PickupClass;				// what class of pickup to spawn when used
var class<Inventory> InventoryClass;		// artifact inventory class this holder can handle
var IconNode IconNode;						// reference to Artifact Machine(TM)
var int ArtifactID;							// Unique identifier

//-----------------------------------------------------------------------------

replication
{
	// Things the server should send to the client.
	unreliable if( bNetDirty && Role==ROLE_Authority )
        bStored;
}

//-----------------------------------------------------------------------------

simulated function int GetTeam() { return TeamIndex; }
function SetTeam( int NewTeam )
{
	TeamIndex = NewTeam;
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
}

//-----------------------------------------------------------------------------

function int ArtifactStolen( Pawn Other )
{
	local Inventory Inv;

	if( InventoryClass != None )
	{
		Inv = Other.Spawn( InventoryClass );
		if( Artifact(Inv) != None )
		{
			// pass artifact info on to inventory item
			Artifact(Inv).Id = ArtifactId;
			Inv.GiveTo( Other );
		}
	}

	bStored = false;
	bHidden = true;
	//Log( "Artifact #" @ ArtifactID @ "removed from" @ Self );
	return ArtifactId;
}

//-----------------------------------------------------------------------------

function bool CanBeStored( class<Artifact> ArtifactClass, int InArtifactID )
{
	return( !bStored && 
			InventoryClass == ArtifactClass && 
			(ArtifactID == InArtifactID || ArtifactID == -1) );
}

//-----------------------------------------------------------------------------

function bool IsArtifactStored()
{
	return( bStored );
}

//-----------------------------------------------------------------------------

function Store( int ID )
{
	ArtifactID = ID;
	bStored = true;
	bHidden = !bStored;
	//Log( "Artifact #" @ ID @ "stored in" @ Self );
}

//-----------------------------------------------------------------------------
// Use reticle

interface function string GetDescription( Controller User )
{
	if( bStored ) return Description;
	else return "";
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bCollideActors=true
	bProjTarget=true
	bStatic=false
	bTravel=false
	bHidden=true
	bStored=false
	bStaticLighting=true
	bShadowCast=true
	bUseCylinderCollision=true
	CollisionRadius=25
	CollisionHeight=50
	PrePivot=(Z=40)
	TeamIndex=255
	ArtifactID=-1;
}
