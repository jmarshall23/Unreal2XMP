
class LaserTripMineDeployable extends DeployableInventory;

//-----------------------------------------------------------------------------

var() float MaxLaserLength;			// max length of the laser before it must hit something to properly deploy
var vector LaserStartPt;			// computed starting point of laser
var vector LaserEndPt;				// computed end point of laser
var actor LaserStartActor;			// actor hit by the starting part of the laser
var actor LaserEndActor;			// actor hit by the ending part of the laser

//-----------------------------------------------------------------------------
// figure out where to mount the laser trip mine

function bool CalcDeploySpot( out vector DeployLocation, out rotator DeployRotation )
{
	local vector X, Y, Z;
	local Actor HitActor;
	local vector StartTrace, EndTrace, Extents, HitLocation, HitNormal;
	local rotator ViewRotation;

	if( Instigator? && Instigator.Controller? )
	{
		ViewRotation = Instigator.GetViewRotation();
		StartTrace = Instigator.Location;
		StartTrace.Z += Instigator.BaseEyeHeight;

		EndTrace = StartTrace + class'PlayerController'.default.UseDistance * vector(ViewRotation);

		Extents.X = DeployClass.default.CollisionRadius;
		Extents.Y = DeployClass.default.CollisionRadius;
		Extents.Z = DeployClass.default.CollisionHeight;

		HitActor = Trace( HitLocation, HitNormal, EndTrace, StartTrace, true, Extents );
		//AddArrow( StartTrace, EndTrace, ColorGreen() );
		//AddCylinder( HitLocation, Extents.X, Extents.Z, ColorYellow() );

		if( HitActor? )
		{
			GetAxes( ViewRotation, X, Y, Z );	// use the "up" (Z) direction of the pawn to orient the mine

			X = HitNormal;
			Y = Z cross X;
			Z = X cross Y;

			DeployLocation = HitLocation;
			DeployRotation = OrthoRotation(X,Y,Z);
			LaserStartActor = HitActor;
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

function bool CanDeploy( vector DeployLocation, rotator DeployRotation )
{
	local Actor HitActor;
	local vector StartTrace, EndTrace, HitLocation, HitNormal;

	if( !Super.CanDeploy( DeployLocation, DeployRotation ) )
		return false;

	// check if there's something the laser can hit
	StartTrace = DeployLocation + vector(DeployRotation)*DeployClass.default.CollisionRadius;
	EndTrace = StartTrace + vector(DeployRotation)*MaxLaserLength;
	//AddArrow( StartTrace, EndTrace, ColorCyan() );

	HitActor = Trace( HitLocation, HitNormal, EndTrace, StartTrace, true );

	if( HitActor != None )
	{
		LaserStartPt = StartTrace;
		LaserEndPt = HitLocation;
		LaserEndActor = HitActor;
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------

function PostDeploy( actor DeployedActor, bool bAltActivate )
{
	local LaserTripMines Mines;

	Mines = LaserTripMines(DeployedActor);
	if( Mines? )
	{
		Mines.StartPt = LaserStartPt;
		Mines.EndPt = LaserEndPt;
		Mines.EndActor = LaserEndActor;			// pass down the actor that the laser end hit
		Mines.SetBase( LaserStartActor );		// set base of mine to mounting actor
		Mines.bHardAttach = true;
		Mines.SetTeam( Owner.GetTeam() );
		Mines.Instigator = Instigator;
		//AddArrow( LaserStart, LaserEnd, ColorRed() );
	}

	DeployedActor.SetPhysics( PHYS_None );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	ItemName="Laser Trip Mines"
	ItemID="LM"
	IconIndex=19

	DeployClass=class'LaserTripMines'
	PickupAmmoCount=5

	AmmoName(0)=class'LaserTripMineAmmo'
	AmmoName(1)=class'LaserTripMineAmmo'

	Mesh=SkeletalMesh'WeaponsK.TripMine'

	DeploySkillCost=0.0050
	bCanIntersectDUs=true
	bCanIntersectPawns=true

	InventoryGroup=4
	GroupOffset=4

	ActivateSound=Sound'U2XMPA.LaserTripMines.LM_Activate'
	SelectSound=Sound'U2XMPA.LaserTripMines.LM_Select'
	DeployFailedSound=Sound'U2XMPA.LaserTripMines.LM_Failed'

	MaxLaserLength=1500.0
}
