//=============================================================================
// StationaryPawn.
//=============================================================================

class StationaryPawn extends Pawn
	abstract;

/*-----------------------------------------------------------------------------
NOTE: StationaryPawns (and subclasses) can add lots of unnecessary overhead if
not used carefully.

a) Set their SightRadius to 0 if the StationaryPawn won't react to other NPCs
   or set this as low as possible (e.g. 1024 for a turret which will only shoot
   at enemies which are within that range.

b) Set their SeenRadius as low as possible. e.g. for "aggressive" stationary
   pawns like turrets, set this to something just greater than the SP's
   SightRadius + splash damage radius so NPCs will react to these just outside 
   of that range (or less if NPCs shouldn't see these prior to the SP firing at
   them, at which point the SeenRadius could maybe be goosed up since the NPCs
   will now "know" about it).

c) Set their HearingThreshold to 0 if the SP won't react to hearing noises.
*/

var bool bHasAttack;		// whether the stationary pawn is capable of attacking
var bool bCoverActor;		// whether NPCs can use this actor for cover (hide behind it)

//-----------------------------------------------------------------------------

function bool IsMobile() { return false; }

//-----------------------------------------------------------------------------

function LaunchPawn( Pawn Other )
{
	// allow StationaryPawns to rest on other StationaryPawns (had this for U2 for some reason)?
	if( StationaryPawn( Other ) == None )
		Super.LaunchPawn( Other );
}

//-----------------------------------------------------------------------------

//mdf-tbd: function LaunchPawnDamage( Pawn Other ); // don't get damaged by Pawn landing on me

//-----------------------------------------------------------------------------
// Subclasses can override this if they consist of multiple actors.

function bool HitSelf( Actor HitActor )
{
	return( HitActor == Self );
}

//-----------------------------------------------------------------------------

simulated function AddVelocity( vector NewVelocity );

//-----------------------------------------------------------------------------

defaultproperties
{
	// assets
	DrawType=DT_Mesh

	// physical
	bStasis=false
	bCanPickupInventory=false
	MaxDesiredSpeed=0
	GroundSpeed=+00000.000000
	WaterSpeed=+00000.000000
	AccelRate=+00000.000000
	JumpZ=+00000.000000
	RotationRate=(Pitch=0,Yaw=0,Roll=0)
	RemoteRole=ROLE_DumbProxy
	Health=500

	// AI
	bHasAttack=false
	bCanTeleport=false
	ControllerClass=class'StationaryPawnController'
	Alertness=0.0
	HearingThreshold=0.0
	SightRadius=0.0 // only set if SP can react to seeing enemies
	Physics=PHYS_None
	bUseCylinderCollision=true
	bShowUseReticle=false
}