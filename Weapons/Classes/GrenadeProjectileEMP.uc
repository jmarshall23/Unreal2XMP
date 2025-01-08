
class GrenadeProjectileEMP extends GrenadeProjectile;

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local Actor Explosion;

	Super.Explode(HitLocation,HitNormal,HitActor);

	if (Level.NetMode != NM_DedicatedServer)
	{
		Explosion = Spawn( class'GrenadeEffectEMP', self,, Location, rotator(HitNormal) );
		Explosion.Instigator = Instigator;
		Explosion.RemoteRole = ROLE_None;

		Spawn( class'GenericExplosionBlower' );
	}
}

defaultproperties
{
	ExplosionSound=sound'WeaponsA.GrenadeLauncher.GL_ExplodeEMP'
	Damage=140.0
	DamageRadius=320.0
	bNoFalloff=true
	MomentumTransfer=0
	MyDamageType=class'DamageTypeEMP'
}
