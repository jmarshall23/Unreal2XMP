//=============================================================================
// $Author: Aleiby $
// $Date: 9/27/02 2:24p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleAffector.uc
// Author:	Aaron R Leiby
// Date:	27 September 2002
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

// Note[aleiby]: Only subclassing ParticleGenerator because we need an outer for the Force.  (This is maybe bad.)
class ParticleAffector extends ParticleGenerator
	placeable
	native;

//------------------------------------------------------------------------------
simulated event PreBeginPlay()
{
	local int i;
	for(i=0;i<Forces.length;i++)
		Level.RegisterExternalForce( Forces[i] );
	Super.PreBeginPlay();
}

//------------------------------------------------------------------------------
simulated event Destroyed()
{
	local int i;
	for(i=0;i<Forces.length;i++)
		Level.UnRegisterExternalForce( Forces[i] );
	Super.Destroyed();
}


defaultproperties
{
	bHidden=true
	bDirectional=true
	DefaultForces=()
}

