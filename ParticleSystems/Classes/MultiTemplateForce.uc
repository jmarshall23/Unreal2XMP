//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	MultiTemplateForce.uc
// Author:	Aaron R Leiby
// Date:	20 Novenber 2001
//------------------------------------------------------------------------------
// Description:	Used to attached particles to a guiding particle.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class MultiTemplateForce extends Force
	native;

var() Array<ParticleTemplate> AttachedTemplates;
var() Array<RangeVector> TemplateOffsets;

//------------------------------------------------------------------------------
native event AttachTemplate( ParticleTemplate T );


defaultproperties
{
	Priority=0.000000
}

