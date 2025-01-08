
class GrenadeProjectileToxic extends GrenadeProjectile;

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
		Spawn( class'GenericExplosionBlowerSmall' );
}

defaultproperties
{
	ExplosionSound=sound'WeaponsA.GrenadeLauncher.GL_ExplodeToxic'
	ExplosionEffect=ParticleSalamander'GL_Toxic.ParticleSalamander0'
	bParticlesDoDamage=true
}
