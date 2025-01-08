//=============================================================================
// $Author: Aleiby $
// $Date: 4/29/02 1:35a $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	Force.uc
// Author:	Aaron R Leiby
// Date:	9 March 2000
//------------------------------------------------------------------------------
// Description:	Base class of all forces that can act upon particles.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class Force extends Object within ParticleGenerator
	abstract
	native;

var() float                   Priority;						// Used to determine which order the forces
															// attached to a specific ParticleGenerator
															// get evaluated in.  Example: Velocity needs 
															// to be updated before location, and location
															// needs to be updated before position.

var() name                    GroupName;					// Used by some ParticleGenerators so they can
															// pick an choose which Forces they can be 
															// affected by.

var() bool                    bIterateAll;					// Should we iterate all the System's templates, or
															// just those specified in our Templates list?

var() editinline Array<ParticleTemplate> AffectedTemplates;	// Templates that this Force affects the particles of.

////////////////
// Interfaces //
////////////////

native event AddTemplate	( ParticleTemplate T );
native event RemoveTemplate	( ParticleTemplate T );

native event AddExternalTemplate	( ParticleTemplate T );
native event RemoveExternalTemplate	( ParticleTemplate T );


defaultproperties
{
	bIterateAll=true
}

