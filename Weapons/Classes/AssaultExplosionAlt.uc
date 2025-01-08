//=============================================================================
// AssaultExplosionAlt.uc
// $Author: Mfox $
// $Date: 4/30/02 3:39p $
// $Revision: 5 $
//=============================================================================
class AssaultExplosionAlt extends Projectile;

#exec OBJ LOAD File=..\Particles\AssaultFX.u

simulated function PostBeginPlay()
{
	local ParticleGenerator P;

	Super.PostBeginPlay();
	if (bPendingDelete)
		return;

	// ** Restrict angle of reflection (2 -> 1.8 or so ... so particles never go into the ground)
	// ** Invert angle of reflection based on incidence (so particle never reflect <45 degrees... if V dot Hit.Normal < 0.5 ... DrawScale3D.X = -CS.X)
	P = class'ParticleGenerator'.static.CreateNew( self, ParticleRadiator'AssaultFX.ParticleRadiator0', Location + vector(Rotation) * 5 );
	P.SetRotation( Rotation );
	P.Trigger( self, Instigator );
	P.ParticleLifeSpan = P.GetMaxLifeSpan() + P.TimerDuration;

	//don't destroy ourselves
	//Super.ExplodeEx(Hit);
}

defaultproperties
{
	bAlwaysRelevant=true
	LifeSpan=5.0
	bCollideActors=false
}
