//=============================================================================
// $Author: Mfox $
// $Date: 1/29/03 4:25p $
// $Revision: 25 $
//=============================================================================

class DynamicBlockingVolume extends BlockingVolume
	native;

//-----------------------------------------------------------------------------
//!!mdf-tbd: native function which mucks with vertices

native function SetFaces( array<Face> Faces );

/*
event Touch( Actor Other )
{
	DMTN( "Touched by: " $ Other );
	Super.Touch( Other );
}

event UnTouch( Actor Other )
{
	DMTN( "UnTouched by: " $ Other );
	Super.UnTouch( Other );
}
*/

defaultproperties
{
	bStatic=false
	bNoDelete=false
	Texture=None
	DrawType=DT_None
	bHidden=true
	bCollideActors=true
	bCollideWorld=true
	bBlockActors=true
	bBlockPlayers=true
	bCanMantle=false
	Physics=PHYS_None
	CollisionRadius=0.0 // so spawn will never fail
	CollisionHeight=0.0 // so spawn will never fail
	bBlockKarma=true
	RemoteRole=ROLE_None //generally want to manually spawn this on the client & manually call SetFaces there
}
