
class GrenadeProjectileFragment extends GrenadeProjectile;

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
		Spawn( class'GenericExplosionBlower' );
}

defaultproperties
{
	ExplosionSound=sound'WeaponsA.GrenadeLauncher.GL_ExplodeFragment'
	ExplosionEffect=ParticleSalamander'GL_Frag.ParticleSalamander2'
	Damage=200.0
	DamageRadius=320.0
	MomentumTransfer=90000
	MyDamageType=class'DamageTypePhysical'
	ShakeRadius=1024
	ShakeMagnitude=40
	ShakeDuration=0.5
}
