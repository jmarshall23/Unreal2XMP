
class PistolAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\PistolEffects.u

var ParticleGenerator MuzzleflashA,MuzzleflashB;

simulated event ThirdPersonEffects()
{
    if (Level.NetMode != NM_DedicatedServer)
	{
		if (!MuzzleflashA)
		{
			MuzzleflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'PistolEffects.ParticleSalamander1', Location );
			AttachToBone(MuzzleflashA, '#Muzzleflash');
		}
		if (!MuzzleflashB)
		{
			MuzzleflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'PistolEffects.ParticleSalamander16', Location );
			AttachToBone(MuzzleflashB, '#Muzzleflash');
		}

		MuzzleflashA.Trigger(Self,Instigator);
		MuzzleflashB.Trigger(Self,Instigator);
    }

    Super.ThirdPersonEffects();
}

simulated function Destroyed()
{
	if (MuzzleflashA?)
	{
		MuzzleflashA.Destroy();
		MuzzleflashA = None;
	}
	if (MuzzleflashB?)
	{
		MuzzleflashB.Destroy();
		MuzzleflashB = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	Mesh=Mesh'Weapons3rdPK.P_TP'
}

