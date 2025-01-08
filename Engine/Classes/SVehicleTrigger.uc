class SVehicleTrigger extends AIScript
	notplaceable;

function UsedBy( Pawn user )
{
	// Enter vehicle code
	SVehicle(Owner).TryToDrive(User);
}

defaultproperties
{
	bHardAttach=True
    bHidden=False
	bCollideActors=false
    bStatic=false
    CollisionRadius=+0080.000000
	CollisionHeight=+0400.000000
	bCollideWhenPlacing=False
	bOnlyAffectPawns=True
    RemoteRole=ROLE_None
}