//=============================================================================
// $Author: Aleiby $
// $Date: 3/08/02 9:51a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	MeshAnchor.uc
// Author:	Aaron R Leiby
// Date:	12 September 2000
//------------------------------------------------------------------------------
// Description:	Locks a particle's position to an Actor's mesh's vertex.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class MeshAnchor extends Force
	native;

var()			Actor			RelativeActor;
var() native	ParticleHandle	ParticleA;
var()			int				VertexIndex;

defaultproperties
{
	Priority=9.000000
}

