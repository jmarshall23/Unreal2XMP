//=============================================================================
// $Author: Mfox $
// $Date: 11/04/02 7:24p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleDamage.uc
// Author:	Aaron R Leiby
// Date:	22 January 2001
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class: 
//------------------------------------------------------------------------------

class ParticleDamage extends ActorCollisionNotifier;


var()	int					Damage;			// per particle
var()	float				Momentum;
var()	vector				AdditionalMomentum;
var()	class<DamageType>	DamageType;


//-----------------------------------------------------------------------------
event NotifyPenetratingActor( Actor Other, int NumParticles, vector Origin )
{
	Other.TakeDamage( Damage*NumParticles, Outer.Instigator, Origin, Normal(Origin-Outer.Location)*Momentum, DamageType );
}


defaultproperties
{
	Damage=1
	DamageType=class'DamageType'
}