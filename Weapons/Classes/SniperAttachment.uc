
class SniperAttachment extends U2WeaponAttachment;

#exec OBJ LOAD File=..\Particles\SniperTracer.u

var PulseLineGenerator Tracer;
var vector HitLocation,LastHitLocation;

replication
{
	unreliable if( bNetDirty && !bNetOwner && (Role==ROLE_Authority) )
		HitLocation;
}

simulated event ThirdPersonEffects()
{
    if (Level.NetMode != NM_DedicatedServer && Instigator? && !Instigator.IsLocallyControlled())
	{
		if (!Tracer)
		{
			Tracer = PulseLineGenerator( class'ParticleGenerator'.static.CreateNew( self, PulseLineGenerator'SniperTracer.PulseLineGenerator1', Location ) );
			//AttachToBone(Tracer, '#Muzzleflash');
			if (Tracer.Connections.length!=1)
				Tracer.Connections.length = 1;
			switch(Instigator.GetTeam())
			{
			case 0: Tracer.BeamColor = ColorRed(); break;
			case 1: Tracer.BeamColor = ColorBlue(); break;
			}
		}

		if (HitLocation != LastHitLocation)		// make sure we got a new location update.
		{
			LastHitLocation = HitLocation;
			Tracer.SetLocation( GetBoneCoords('#Muzzleflash').Origin );
			Tracer.SetEndpoints( Tracer.Location, HitLocation );
			Tracer.Trigger(Self,Instigator);
		}
	}

    Super.ThirdPersonEffects();
}

simulated function Destroyed()
{
	if (Tracer?)
	{
		Tracer.Destroy();
		Tracer = None;
	}
	Super.Destroyed();
}

defaultproperties
{
	Mesh=Mesh'Weapons3rdPK.SR_TP'
}

