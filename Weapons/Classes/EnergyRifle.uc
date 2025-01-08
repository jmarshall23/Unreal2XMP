
class EnergyRifle extends Weapon;

#exec OBJ LOAD File=..\Particles\EnergyFX.u

var ParticleGenerator FireA,FireB;
var ParticleGenerator MuzzleflashA,MuzzleflashB;

simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot );

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();

	if( bPendingDelete )
		return;

	FireA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'EnergyFX.ParticleSalamander0', Location );
	FireB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'EnergyFX.ParticleSalamander3', Location );

	if( Level.NetMode != NM_DedicatedServer )
	{
		MuzzleflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'EnergyFX.ParticleSalamander1', Location );
		AttachToBone(MuzzleflashA, '#Muzzleflash1');

		MuzzleflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'EnergyFX.ParticleSalamander1', Location );
		AttachToBone(MuzzleflashB, '#Muzzleflash1');
	}
}

// overridden since we don't actually have a projectile to get these values from.
function float GetProjSpeed( bool bAlt )
{
	if(!bAlt)	return 1200;
	else		return Super.GetProjSpeed(bAlt);
}

function AuthorityFire()
{
	local EnergyRifleAttachment A;
	A = EnergyRifleAttachment(ThirdPersonActor);
	if (A?) A.bAltFire = (AmmoIndex == 1);
	Super.AuthorityFire();
}

simulated function EverywhereFire()
{
	Super.EverywhereFire();
	if (AmmoIndex == 1)
		SendEvent("ER_AltFire");
}

simulated function PlayFiring()
{
	Super.PlayFiring();

	if (AmmoIndex == 0)
	{
		FireA.Trigger(self,Instigator);
		FireB.Trigger(self,Instigator);
	}

	if (MuzzleflashA?) MuzzleflashA.Trigger(self,Instigator);
	if (MuzzleflashB?) MuzzleflashB.Trigger(self,Instigator);
}
	
simulated state Firing
{
ignores Fire,AltFire;

	simulated event BeginState()
	{
		Super.BeginState();
		if (AmmoIndex == 0)
		{
			Enable('Tick');
			bUpdateLocation = false;
			SendEvent("ER_Fire");
		}
	}
	simulated event EndState()
	{
		Super.EndState();
		if (AmmoIndex == 0)
		{
			Disable('Tick');
			bUpdateLocation = true;
			SendEvent("ER_UnFire");
		}
	}
	simulated event Tick( float DeltaTime )
	{
		local EnergyRifleAttachment A;
		local vector EffectLocation;
		local rotator EffectRotation;
		local vector Dir,End,Start;
		local actor HitActor;
		local vector HitLocation,HitNormal;
		local bool bDelayTick;

		if (AmmoIndex != 0)
			return;

		SetLocation( Instigator.Location + Instigator.CalcDrawOffset(self) );
		SetRotation( Instigator.GetViewRotation() );

		EffectLocation = GetBoneCoords('#AltFireMuzzleflash').Origin;
		EffectRotation = GetBoneRotation('#AltFireMuzzleflash');

		Dir = vector(EffectRotation);
		End = EffectLocation;
		Start = End - (Dir * 64.0);
		HitActor = Trace(HitLocation,HitNormal,End,Start,true);
		if( HitActor? )
			EffectLocation = HitLocation - Dir; //(Dir * 6.0);

		A = EnergyRifleAttachment(ThirdPersonActor);
		if (A?)
		{
			A.EffectLocation = EffectLocation;
			A.EffectRotation = EffectRotation;
		}

		bDelayTick = (Instigator.IsLocallyControlled() && (Level.TimeSeconds - FireA.LastRenderTime) < 0.5);

		FireA.SetLocation( EffectLocation );
		FireA.SetRotation( EffectRotation );
		FireA.bForceDelayTick = bDelayTick;

		FireB.SetLocation( EffectLocation );
		FireB.SetRotation( EffectRotation );
		FireB.bForceDelayTick = bDelayTick;
	}
}

simulated function Destroyed()
{
	if (FireA?)
	{
		FireA.Destroy();
		FireA = None;
	}
	if (FireB?)
	{
		FireB.Destroy();
		FireB = None;
	}
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
	// text
    ItemName="Energy Rifle"
	IconIndex=7
	ItemID="ER"
	Crosshair="Crosshair_ER"

	// weapon data
	AutoSwitchPriority=8
	InventoryGroup=4
	GroupOffset=1
	PickupAmmoCount=150

	// assets
	Mesh=SkeletalMesh'WeaponsK.ER_FP'
	AttachmentClass=class'EnergyRifleAttachment'
	//!! FireSkins(0)=material'WeaponFXT.ER_Skin2FX'
	PlayerViewOffset=(X=22,Y=1,Z=-34)
	FireOffset=(X=0,Y=0,Z=0)
	SelectSound=sound'WeaponsA.EnergyRifle.ER_Select'
	ReloadSound=sound'WeaponsA.EnergyRifle.ER_Reload'
	ReloadUnloadedSound=sound'WeaponsA.EnergyRifle.ER_ReloadUnloaded'
	SoundVolume=255

	AmmoName(0)=class'EnergyRifleAmmo'
	AmmoName(1)=class'EnergyRifleAmmoAlt'

	// additional timing
	ReloadTime=2.33
	ReloadUnloadedTime=1.5
}

