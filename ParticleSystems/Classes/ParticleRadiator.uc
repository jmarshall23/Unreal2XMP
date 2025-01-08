//=============================================================================
// $Author: Chargrove $
// $Date: 8/19/02 7:35p $
// $Revision: 4 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleRadiator.uc
// Author:	Aaron R Leiby
// Date:	28 March 2000
//------------------------------------------------------------------------------
// Description:	Emits particles from the surface of the specified mesh.
//------------------------------------------------------------------------------
// Notes: Only works with Golem (LegendMesh).
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ParticleRadiator extends ParticleGenerator
	placeable
	native;

////////////////////////
// Editable variables //
////////////////////////

var() float Volume;				// Number of particles emitted per second.
var   float ParticleTimer;		// Used internally to collect "emission residue" <-- fancy term for rounding errors between ticks.

var() float VolumeScalePct;		// Percentage of the visibility radius/height used to scale the volume of the particle sprayer.
								// A number from 0.0 to 1.0.  (Scaling begins at VolumeScalePct*VisibilityRadius units from
								// the generator, and is linearly interpolated until VisibilityRadius units from the generator.)
								// Scaling goes from Volume to 0 (not Volume to MinVolume).

var() float MinVolume;			// Minimum volume that Level->Engine-Client->ParticleDensity is allowed to scale us to.

var() bool  bLOSClip;			// Clip using line of sight check?
var() bool  bStopIfHidden;		// Don't create new particles if MeshOwner.bHidden.

var() Actor MeshOwner;			// Set this if you want to use some other Actor's Mesh.
var() int LODLevel;				// Level of mesh LOD to use.

var() enum  TriSelect
{
	TS_Random,					// Randomly pick a particle from the ParticleTemplates array.
	TS_Linear,					// Cyclically iterate through the ParticleTemplates array.
	TS_Indexed,					// Loops through the TriIndices array to use as indices.
	TS_RandIndex,				// Randomly selects an index from the TriIndices array to use.

} TriSelectMethod;

var() array<int> TriIndices;	// Order of indices to use when using TS_Indexed.

var   int   iTri;				// Used internally for triangle selection.
var   int   iIndex;				// Used internally for indexed triangle selection.

var() bool  bOrgiSpawn;			// Spawn particles at the origin of the selected triangle?  (otherwise randomly distribute them across the surface of the triangle)

var() string AttachName;		// Name of attachment on mesh instance to use instead of main mesh, if applicable

//------------------------------------------------------------------------------
simulated event TurnOn()
{
	iTri = 0;
	iIndex = 0;

	Super.TurnOn();

	ParticleTimer = 1.0 / Volume;	// force first particle to spit out on initial tick after turning on.
}


defaultproperties
{
	Texture=Texture'ParticleSystems.Icons.S_ParticleRadiator'
	Style=STY_Translucent
	DrawScale=1.000000
	bOrgiSpawn=false
	TriSelectMethod=TS_Random
}

