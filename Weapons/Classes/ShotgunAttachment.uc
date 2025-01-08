
class ShotgunAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\ShotgunFX.u

var ParticleGenerator Muzzleflash;

simulated event ThirdPersonEffects()
{
    if (Level.NetMode != NM_DedicatedServer)
	{
		if (!Muzzleflash)
		{
			Muzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'ShotgunFX.ParticleSalamander0', Location );
			AttachToBone(Muzzleflash, '#Muzzleflash');
		}
		Muzzleflash.Trigger(Self,Instigator);
    }

    Super.ThirdPersonEffects();
}

simulated function Destroyed()
{
	if (Muzzleflash?)
	{
		Muzzleflash.Destroy();
		Muzzleflash = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	Mesh=Mesh'Weapons3rdPK.S_TP'
}

