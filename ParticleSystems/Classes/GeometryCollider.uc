//=============================================================================
// $Author: Aleiby $
// $Date: 10/18/02 5:09p $
// $Revision: 4 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	GeometryCollider.uc
// Author:	Aaron R Leiby
// Date:	18 June 2000
//------------------------------------------------------------------------------
// Description:	Uses Trace to collide with level geometry.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class GeometryCollider extends Force
	native;

var() bool  bCollideActors;
var() bool	bDiffuse;
var() bool  bCollisionNotify;
var() bool  bDeleteOnContact;
var() bool  bStopOnContact;
var() bool	bIgnoreInstigator;

var() Range Elasticity;		// Fix ARL: Make this a fixed per-particle value rather than randomly chosen on each bounce?

var() int   NotificationsPerTick;
var() float MaxNotificationTime;
var   float NotificationTimer;

var() float MinIncidence;	// one minus cosine theta

var() Array<ParticleTemplate> ImpactEffects;

var() float	ImpactPercent;

var() sound Sound;
var() range Volume,Radius,Pitch;

/*
	Projector extentions
*/

struct native ProjSettings
{
	// Projector blending operation.

	var() Projector.EProjectorBlending	MaterialBlendingOp,		// The blending operation between the material being projected onto and ProjTexture.
										FrameBufferBlendingOp;	// The blending operation between the framebuffer and the result of the base material blend.

	// Projector properties.

	var() Material	ProjTexture;
	var() int		FOV;
	var() int		MaxTraceDistance;
	var() bool		bProjectBSP;
	var() bool		bProjectTerrain;
	var() bool		bProjectStaticMesh;
	var() bool		bProjectParticles;
	var() bool		bProjectActor;
	var() bool		bClipBSP;
	var() bool		bClipStaticMesh;
	var() bool		bProjectOnUnlit;
	var() bool		bGradient;
	var() bool		bProjectOnBackfaces;
	var() bool		bProjectOnAlpha;
	var() bool		bProjectOnParallelBSP;
	var() name		ProjectTag;
	var() name		ExcludedActorsTag;
	var() range		Size;
	var() range		Alpha;
};

var(Decals) array<ProjSettings> ProjectorSettings;

struct CheckResult	//!!MERGE (belongs in Actor)
{
	var() Object	Next;		//IteratorList*
	var() Actor		Actor;
	var() vector	Location;   // Location of the hit in coordinate system of the returner.
	var() vector	Normal;     // Normal vector in coordinate system of the returner. Zero=none.
	var() Primitive	Primitive;  // Actor primitive which was hit, or NULL=none.
	var() float		Time;       // Time until hit, if line check.
	var() int		Item;       // Primitive data item which was hit, INDEX_NONE=none.
	var() Material	Material;	// Material cached by LineCheck()
};

//------------------------------------------------------------------------------
native event AddImpactEffect( ParticleTemplate T );

//------------------------------------------------------------------------------
event CollisionNotification( ParticleHandle P, CheckResult Hit );


defaultproperties
{
	Priority=7.000000
	Elasticity=(A=0.750000,B=0.750000)
	ImpactPercent=1.000000
	bIgnoreInstigator=true
}

