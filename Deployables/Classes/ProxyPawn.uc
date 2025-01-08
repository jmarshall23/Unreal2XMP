/*-----------------------------------------------------------------------------
Used to provide a target for AI controlled Pawns (NPCs, turrets etc.). Pretty
much totally inert -- it is assumed that the PawnProxy will be placed inside 
some (non-Pawn) actor which does actual damage handling (and takes care to
create / clean up the PawnProxy as needed).  When spawned, owner should call 
Initialize to set collision size and SetTeam to set the ProxyPawn's team.
*/

class ProxyPawn extends StationaryPawn;

//-----------------------------------------------------------------------------

function Initialize( float NewCollisionRadius, float NewCollisionHeight )
{
	SetCollisionSize( NewCollisionRadius, NewCollisionHeight );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bCollideActors=false
	bCollideWorld=false
	bBlockActors=false
	bBlockPlayers=false
	Physics=PHYS_None
	bActorShadows=false
	DrawType=DT_None
	Mesh=None
	bHidden=false //not hidden so NPCs will attack it
	RemoteRole=ROLE_None // should never need to exist on clients (AI handled server-side)
	ControllerClass=class'StationaryPawnControllerDefensive'
	CollisionHeight=0.0
	CollisionRadius=0.0
}
