//=============================================================================
// $Author: Aleiby $
// $Date: 10/18/02 5:09p $
// $Revision: 4 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleDecalTemplate.uc
// Author:	Aaron R Leiby
// Date:	9 August 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ParticleDecalTemplate extends SpriteParticleTemplate
	native;

//#if 0 //ProjDecals
//
//var() bool bUseProjectors;
//
///*
//	Projector extentions
//*/
//
//// Projector blending operation.
//
//var(Projector) Projector.EProjectorBlending	MaterialBlendingOp,		// The blending operation between the material being projected onto and ProjTexture.
//											FrameBufferBlendingOp;	// The blending operation between the framebuffer and the result of the base material blend.
//
//// Projector properties.
//
////var(Projector) Material	ProjTexture;
//var(Projector) int		FOV;
//var(Projector) int		MaxTraceDistance;
//var(Projector) bool		bProjectBSP;
//var(Projector) bool		bProjectTerrain;
//var(Projector) bool		bProjectStaticMesh;
//var(Projector) bool		bProjectParticles;
//var(Projector) bool		bProjectActor;
//var(Projector) bool		bClipBSP;
//var(Projector) bool		bClipStaticMesh;
//var(Projector) bool		bProjectOnUnlit;
//var(Projector) bool		bGradient;
//var(Projector) bool		bProjectOnBackfaces;
//var(Projector) bool		bProjectOnAlpha;
//var(Projector) bool		bProjectOnParallelBSP;
//var(Projector) name		ProjectTag;
//var(Projector) name		ExcludedActorsTag;
//
//var(ProjActor) Projector ProjActor;
//#endif

defaultproperties
{
	ParticleType="ParticleDecal"
	DrawOrder=0.000000

	//// Projector settings.
	//MaterialBlendingOp=PB_None
	//FrameBufferBlendingOp=PB_Modulate
	//FOV=0
	//MaxTraceDistance=8
	//bProjectBSP=True
	//bProjectTerrain=True
	//bProjectStaticMesh=True
	//bProjectParticles=True
	//bProjectActor=True
	//bClipBSP=False
	//bClipStaticMesh=False
	//bProjectOnUnlit=False
	//bProjectOnBackfaces=False
}

