//=============================================================================
// ShotgunProjectile.uc
// $Author: Aleiby $
// $Date: 10/03/02 6:09p $
// $Revision: 7 $
//=============================================================================
class ShotgunProjectile extends Projectile;

simulated function PostBeginPlay()
{
	local ParticleGenerator P;

	Super.PostBeginPlay();
	if (bPendingDelete)
		return;

	P = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'ShotgunFX.ParticleSalamander2', Location );
	P.SetRotation( Rotation );
	P.Trigger( self, Instigator );
	P.ParticleLifeSpan = P.GetMaxLifeSpan() + P.TimerDuration;
}

defaultproperties 
{
	bCollideActors=false
	bCollideWorld=false
	LifeSpan=1.0
	Speed=800
	MaxSpeed=800
}

