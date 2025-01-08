
class GrenadeProjectileIncendiary extends GrenadeProjectile;

#exec OBJ LOAD File=..\Particles\GL_cluster_fx.u

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local ParticleGenerator Particles;

	Super.Explode(HitLocation,HitNormal,HitActor);

	Particles = class'ParticleGenerator'.static.CreateNew( self, ParticleGenerator'GL_cluster_fx.ParticleSalamander0', HitLocation );
	Particles.SetRotation( rotator(HitNormal) );
	Particles.Trigger( self, Instigator );
	Particles.ParticleLifeSpan = Particles.GetMaxLifeSpan() + Particles.TimerDuration + Particles.RampUpTime + Particles.RampDownTime + 5.0;
}

defaultproperties
{
	ExplosionSound=sound'WeaponsA.GrenadeLauncher.GL_ExplodeIncendiary'
	ExplosionEffect=None
	DamageRadius=256.0
	Damage=50.0
	MomentumTransfer=9000
	MyDamageType=class'DamageTypeThermal'
}
