//=============================================================================
// $Author: Mfox $
// $Date: 4/30/02 12:27p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleBlower.uc
// Author:	Aaron R Leiby
// Date:	28 July 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

// Note[aleiby]: Only subclassing ParticleGenerator because we need an outer for the Force.  (This is maybe bad.)
class ParticleBlower extends ParticleGenerator
	placeable
	native;

var() float  Spread;			// Cone of influence (centered on Rotation).

var() float  MaxSpeed;			// Don't adjust the velocity of particles going faster than this.

var() float  DestroyRadius;		// Destroy all affected particles that come within this radius.

var() float  RadiusMax;			// Effective range. (Max)
var() float  RadiusMin;			// Effective range. (Min)

var() float  AccelMax;			// Acceleration at MaxRadius.
var() float  AccelMin;			// Acceleration at MinRadius.

// Fix ARL: Account for center falloff.

var BlowerForce Blower;

//------------------------------------------------------------------------------
simulated event PreBeginPlay()
{
	Blower = new(Self)class'BlowerForce';
	Level.RegisterExternalForce( Blower );
	AddForce( Blower );

	Super.PreBeginPlay();
}

//------------------------------------------------------------------------------
simulated event Destroyed()
{
	if( Blower != None )
	{
		Level.UnRegisterExternalForce( Blower );
		RemoveForce( Blower );
		Blower.Delete();
		Blower = None;
	}

	Super.Destroyed();
}

//------------------------------------------------------------------------------
function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.BlowerForce';
}


defaultproperties
{
	bHidden=true
	bDirectional=true
}

