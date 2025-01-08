
class GrenadeProjectileConcussion extends GrenadeProjectile;

#exec OBJ LOAD File=..\Particles\concussion_grenade_FX.u

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local ParticleGenerator Particles;

	local U2Pawn P;
	local float Incidence;
	local float MaxDist;
	local vector Diff;

	Super.Explode(HitLocation,HitNormal,HitActor);

	Particles = class'ParticleGenerator'.static.CreateNew( self, ParticleGenerator'concussion_grenade_FX.ParticleSalamander0', HitLocation + /*HitNormal*16.0*/vect(0,0,16) );
	//Particles.SetRotation( rotator(HitNormal) );
	Particles.SetRotation( rotator(vect(0,0,1)) );
	Particles.Trigger( Self, Instigator );
	Particles.ParticleLifeSpan = Particles.GetMaxLifeSpan() + Particles.TimerDuration + Particles.RampUpTime + Particles.RampDownTime + 5.0;

	if (Level.NetMode != NM_DedicatedServer)
		Spawn( class'GenericExplosionBlower' );

	if (Role == ROLE_Authority)
	{
		MaxDist = DamageRadius * 10.0;
		foreach VisibleCollidingActors( class'U2Pawn', P, MaxDist, HitLocation )
		{
			Diff = HitLocation - P.Location;
			Incidence = normal(Diff) dot vector(P.GetViewRotation());
			if (Incidence > 0)
				P.FlashbangTimer = Blend( 6.0, 0.0, ABlend( 0.0, (PI/2.0), acos(Incidence) ) ) * ABlend( MaxDist, 0.0, VSize(Diff) );
		}
	}
}

defaultproperties
{
	ExplosionSound=sound'WeaponsA.GrenadeLauncher.GL_ExplodeConcussion'
	ExplosionEffect=None
	Damage=15.0
	DamageRadius=180.0
	MomentumTransfer=200000
	bNoFalloff=true
}
