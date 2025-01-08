//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	TarpCollider.uc
// Author:	Aaron R Leiby
// Date:	30 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class TarpCollider extends Force
	native;

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.AnchorForce';
}

defaultproperties
{
	Priority=5.000000
}

