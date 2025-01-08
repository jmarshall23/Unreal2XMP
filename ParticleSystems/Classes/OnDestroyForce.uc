//=============================================================================
// $Author: Aleiby $
// $Date: 5/30/02 1:05a $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	OnDestroyForce.uc
// Author:	Aaron R Leiby
// Date:	20 Novenber 2001
//------------------------------------------------------------------------------
// Description:	Used to spawn additional particles on destruction of another particle.
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class OnDestroyForce extends Force
	native;

var() Array<ParticleTemplate> OnDestroyTemplates;

var() bool bInheritVelocity;

var() sound Sound;
var() range Volume,Radius,Pitch;

var(HurtRadius) float DamageAmount;
var(HurtRadius) float DamageRadius;
var(HurtRadius) class<DamageType> DamageType;
var(HurtRadius) float Momentum;

//------------------------------------------------------------------------------
native event AddDestroyTemplate( ParticleTemplate T );


defaultproperties
{
}

