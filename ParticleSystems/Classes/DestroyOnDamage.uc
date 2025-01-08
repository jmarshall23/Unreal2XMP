//=============================================================================
// $Author: Aleiby $
// $Date: 5/30/02 10:25p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	DestroyOnDamage.uc
// Author:	Aaron R Leiby
// Date:	29 May 2002
//------------------------------------------------------------------------------
// Description:	Destroys particles within calls to HurtRadius.
//------------------------------------------------------------------------------
// How to use this class:
// + Radius defines how big the individually affect particles are in Unreal
//   units scaled by their size.
// + Types of damage to react to.  Leave empty to react to all damage types.
// + Use DestroyLifeSpan to delay the particle destruction by a specified
//   amount of time.  This is useful for making cascading effects on particles
//   that damage each other.
//------------------------------------------------------------------------------

class DestroyOnDamage extends Force
	native;

var() float Radius;		// Multiplied by particle size.
var() array< class<DamageType> > DamageTypes;
var() float DestroyLifeSpan;


//------------------------------------------------------------------------------
static final native function Notify( level Level, vector Origin, float DamageRadius, class<DamageType> DamageType );


defaultproperties
{
}

