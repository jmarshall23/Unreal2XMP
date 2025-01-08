//=============================================================================
// $Author: Sbrown $
// $Date: 2/06/02 8:47p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleSkeleton.uc
// Author:	Aaron R Leiby
// Date:	5 December 2001
//------------------------------------------------------------------------------
// Description:	Attaches particles to bones.
//------------------------------------------------------------------------------
// Notes: Only works with Golem (LegendMesh).
//------------------------------------------------------------------------------
// How to use this class: Set mesh or MeshOwner.
//------------------------------------------------------------------------------

class ParticleSkeleton extends ParticleGenerator
	placeable
	native;

////////////////////////
// Editable variables //
////////////////////////

// Fix ARL: Add mitosis functionality.

var() Actor MeshOwner;				// Set this if you want to use some other Actor's Mesh.
var() int NumParticles;				// Total number of particles to spawn (spread out across all bones).
var() range Rigidity;				// How quickly the particles move to their desired locations (negative numbers enforce linear rate).
var() bool bInitialize;				// Set to true to create new set of particles based on new template settings.

var() bool bRandomBoneSelection;	// Next bone index is randomly selected.
var() range BoneDist;				// Next bone index equals current plus BoneDist.
var() range BoneSelectionTime;		// Time between bone selections (per particle).

var   ParticleTemplate AnchorTemplate;


function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.AttractionForce';
	Dependant = class'ParticleSystems.BoneLocatorForce';
	Dependant = class'ParticleSystems.ParticleTemplate';
}


defaultproperties
{
	Texture=Texture'ParticleSystems.Icons.S_ParticleSkeleton'
	Style=STY_Translucent
	DrawScale=1.000000
	DefaultForces=()
}

