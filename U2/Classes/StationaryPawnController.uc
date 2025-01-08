//=============================================================================
// StationaryPawnController.uc
// Created by: Sam Brown
// Created on: 4/22/2002
// $Author: Mfox $
// $Date: 9/03/02 9:34p $
// $Revision: 4 $
//=============================================================================

class StationaryPawnController extends AIController;

//-----------------------------------------------------------------------------

function MaybeInheritEnemy( Pawn Other, Pawn NewEnemy, optional bool bCanSee );

//-----------------------------------------------------------------------------

auto state Active
{
	ignores EnemyNotVisible, HearNoise, NotifyLanded, NotifyPhysicsVolumeChange, NotifyHeadVolumeChange, NotifyLanded, NotifyHitWall, NotifyBump;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bAdjustFromWalls=false
	PlayerReplicationInfoClass=Class'Engine.PlayerReplicationInfo'
	bShouldPossess=true //NEW (mdf)
	bShowSelf=true
}
