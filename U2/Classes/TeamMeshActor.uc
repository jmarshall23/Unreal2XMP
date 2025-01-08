//=============================================================================
// TeamMeshActor.
//=============================================================================

class TeamMeshActor extends StaticMeshActor
	native
	placeable;

// This class is native so that they can be spawned in the editor in native code (ie: for deploy points)

function SetTeam( int NewTeam )
{
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
}

defaultproperties
{
	bStatic=False
}