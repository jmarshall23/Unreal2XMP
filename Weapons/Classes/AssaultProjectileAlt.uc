//=============================================================================
// AssaultProjectileAlt.uc
// $Author: Aleiby $
// $Date: 10/03/02 6:09p $
// $Revision: 7 $
//=============================================================================
class AssaultProjectileAlt extends Projectile;

#exec OBJ LOAD File=..\Particles\AssaultFX.u

var ParticleSalamander Sal;

simulated event PostBeginPlay()
{
	if (bPendingDelete)
		return;

	if (Level.NetMode == NM_DedicatedServer)
		return;

	Sal = ParticleSalamander( class'ParticleGenerator'.static.CreateNew( Self, ParticleSalamander'AssaultFX.ParticleSalamander0', Location ) );
	Sal.SetRotation( Rotation );
	Sal.SetBase( Self );
	Sal.Trigger( Self, Instigator );
}

simulated event Destroyed()
{
	if( Sal != None )
	{
		Sal.Destroy();
		Sal = None;
	}
	Super.Destroyed();
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	local vector Direction;

	if( (Role == ROLE_Authority) && Pawn(HitActor)==None )	// don't bounce off pawns.
	{
		Direction = Normal(MirrorVectorByNormal(Velocity,HitNormal));
		Spawn(class'AssaultExplosionAlt',Self,, HitLocation, rotator(Direction) );
	}

	Super.Explode(HitLocation,HitNormal,HitActor);
}

defaultproperties
{
	Speed=3000
	MaxSpeed=3000
	Damage=160
	MyDamageType=class'AssaultDamage'
	MomentumTransfer=40000
}
