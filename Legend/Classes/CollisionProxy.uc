//=============================================================================
// CollisionProxy.uc
//=============================================================================

class CollisionProxy extends Actor;

//-----------------------------------------------------------------------------

var Actor TouchTarget;

//-----------------------------------------------------------------------------

function CP_SetCollisionSize( float NewCollisionRadius, float NewCollisionHeight )
{
	SetCollisionSize( NewCollisionRadius, NewCollisionHeight );
}

//-----------------------------------------------------------------------------

function CP_SetCollision( optional bool bNewColActors, optional bool bNewBlockActors, optional bool bNewBlockPlayers )
{
	SetCollision( bNewColActors, bNewBlockActors, bNewBlockPlayers );
}

//-----------------------------------------------------------------------------

function CP_SetLocation( vector NewLocation )
{
	SetLocation( NewLocation );
}

//-----------------------------------------------------------------------------

function CP_SetTouchTarget( Actor NewTouchTarget )
{
	TouchTarget = NewTouchTarget;
}

//-----------------------------------------------------------------------------

event Touch( Actor Other )
{
	if( TouchTarget != None )
		TouchTarget.Touch( Other );
}

//-----------------------------------------------------------------------------

event UnTouch( Actor Other )
{
	if( TouchTarget != None )
		TouchTarget.UnTouch( Other );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bHidden=True
	RemoteRole=ROLE_None
	CollisionRadius=1.000000
	CollisionHeight=1.000000
	bCollideActors=false
	bCollideWorld=false
	bBlockActors=false
	bBlockPlayers=false
	bProjTarget=false
}

