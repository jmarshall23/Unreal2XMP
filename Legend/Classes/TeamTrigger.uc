//=============================================================================
// TeamTrigger: triggers for all except pawns with matching team
//=============================================================================
class TeamTrigger extends Trigger;

var() byte Team;
var() bool bTimed;
var() bool bTriggerSameTeam;

function PostBeginPlay()
{
	Super.PostBeginPlay();
	if ( bTimed )
		SetTimer(2.5, true);
}

function Timer()
{
	local Controller P;

	for ( P=Level.ControllerList; P!=None; P=P.NextController )
		if ( (P.Pawn != None) && (abs(Location.Z - P.Pawn.Location.Z) < CollisionHeight + P.CollisionHeight)
			&& (VSize(Location - P.Pawn.Location) < CollisionRadius) )
			Touch(P.Pawn);
	SetTimer(2.5, true);
}

function bool IsRelevant( actor Other )
{
	local bool bSameTeam;

	if( !bInitiallyActive || !Level.Game.bTeamGame || (Other.Instigator == None) )
		return false;
	
	bSameTeam = Level.Game.IsOnTeam(Other.Instigator.Controller, Team);
	if( bTriggerSameTeam ^^ bSameTeam )			// XOR
		return false;

	return Super.IsRelevant(Other);
}

function TakeDamage( int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
	if ( (InstigatedBy != None) && Level.Game.bTeamGame
		&& !Level.Game.IsOnTeam(InstigatedBy.Controller, Team) )
		Super.TakeDamage(Damage, instigatedBy, HitLocation, Momentum, DamageType);
}

simulated function int GetTeam()
{
	return Team;
}

function SetTeam( int NewTeam )
{
	local Actor A;

	Team = NewTeam;
	CheckTouchList();

	// Team Skins: pass on set team notification to trigger actors
	foreach AllActors( class'Actor', A, Event )
		A.SetTeam( NewTeam );
}

defaultproperties
{
	bTriggerSameTeam=true
	Team=255
}