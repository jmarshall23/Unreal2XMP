
class AssaultRifle extends Weapon;

#exec OBJ LOAD File=..\Particles\AssaultFX.u

var ParticleGenerator MuzzleflashA,MuzzleflashB;
var ParticleGenerator AltMuzzleflash;
var ParticleGenerator VentflashA,VentflashB,VentflashC;
var ParticleGenerator ReloadDustA,ReloadDustB,ReloadDustC;

var float Angle, DesiredAngle;
var float LastTimeSeconds;

// HUD accessors.
function interface float GetCrosshairAngle90(){ return Angle + 90; }
function interface float GetCrosshairAngle210(){ return Angle + 210; }
function interface float GetCrosshairAngle330(){ return Angle + 330; }

// Smooth interpolation (since HandleTargetDetails doesn't get called every frame).
simulated function UpdateCrosshair( Canvas Canvas )
{
	local float DeltaSeconds, Pct;
	Super.UpdateCrosshair(Canvas);
	DeltaSeconds = Level.TimeSeconds - LastTimeSeconds;
	LastTimeSeconds = Level.TimeSeconds;
	Pct = FMin( DeltaSeconds * 4.0, 1.0 );
	Angle += (DesiredAngle - Angle) * Pct;
}

simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot )
{
	if( A!=None && LevelInfo(A) == None )
		DesiredAngle = VSize(A.Location - ViewLoc) * 0.3;
}

simulated function SetupAttachments()
{
	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		MuzzleflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander2', Location );
		AttachToBone(MuzzleflashA, '#Muzzleflash');

		MuzzleflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander3', Location );
		AttachToBone(MuzzleflashB, '#Muzzleflash');

		AltMuzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander2', Location );
		AttachToBone(AltMuzzleflash, '#AltFireMuzzleflash');

		VentflashA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander4', Location );
		AttachToBone(VentflashA, '#Vent1');

		VentflashB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander4', Location );
		AttachToBone(VentflashB, '#Vent2');

		VentflashC = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander4', Location );
		AttachToBone(VentflashC, '#Vent3');

		ReloadDustA = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander6', Location );
		AttachToBone(ReloadDustA, '#Vent1');

		ReloadDustB = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander6', Location );
		AttachToBone(ReloadDustB, '#Vent2');

		ReloadDustC = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'AssaultFX.ParticleSalamander6', Location );
		AttachToBone(ReloadDustC, '#Vent3');
	}
}

simulated function UpdateAttachments()
{
	if (AmmoIndex==0)
	{
		if (MuzzleflashA?) MuzzleflashA.Trigger(self,Instigator);
		if (MuzzleflashB?) MuzzleflashB.Trigger(self,Instigator);
		if (VentflashA?) VentflashA.Trigger(self,Instigator);
		if (VentflashB?) VentflashB.Trigger(self,Instigator);
		if (VentflashC?) VentflashC.Trigger(self,Instigator);
	}
	else
	{
		if (AltMuzzleflash?) AltMuzzleflash.Trigger(self,Instigator);
		if (VentflashA?) VentflashA.Trigger(self,Instigator);
		if (VentflashB?) VentflashB.Trigger(self,Instigator);
		if (VentflashC?) VentflashC.Trigger(self,Instigator);
	}
}

simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	SetupAttachments();
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
	if (AltMuzzleflash?)
	{
		AltMuzzleflash.Destroy();
		AltMuzzleflash = None;
	}
	if (VentflashA?)
	{
		VentflashA.Destroy();
		VentflashA = None;
	}
	if (VentflashB?)
	{
		VentflashB.Destroy();
		VentflashB = None;
	}
	if (VentflashC?)
	{
		VentflashC.Destroy();
		VentflashC = None;
	}
	if (ReloadDustA?)
	{
		ReloadDustA.Destroy();
		ReloadDustA = None;
	}
	if (ReloadDustB?)
	{
		ReloadDustB.Destroy();
		ReloadDustB = None;
	}
	if (ReloadDustC?)
	{
		ReloadDustC.Destroy();
		ReloadDustC = None;
	}
	Super.Destroyed();
}

simulated function PlayReloading()
{
	Super.PlayReloading();

	if (ReloadDustA?) ReloadDustA.Trigger(self,Instigator);
	if (ReloadDustB?) ReloadDustB.Trigger(self,Instigator);
	if (ReloadDustC?) ReloadDustC.Trigger(self,Instigator);
}

// Follows, is a big nasty hack just to get viewshake to work correctly with the AR's unusual requirements.

simulated function PlayFiring()
{
	if (AmmoIndex==0)
		HandleLastRound();

	Super.PlayFiring();

	UpdateAttachments();
}

function HandleTraceImpact( Actor Other, vector HitLocation, vector HitNormal )
{
	local AssaultAttachment TP;
	Super.HandleTraceImpact(Other,HitLocation,HitNormal);
	TP = AssaultAttachment(ThirdPersonActor);
	if (TP?)
		TP.HitLocation = HitLocation;
}

simulated function HandleLastRound()
{
	local bool bLastRound;
	local pawn Pawn;
	local playercontroller P;

	bLastRound = AmmoType.ReloadRequired(1);
	if( bLastRound )
	{
		// turn off view shake.
		Pawn = Pawn(Owner);
		if( Pawn!=None )
		{
			P = PlayerController(Pawn.Controller);
			if( P!=None )
				P.ShakeView(7.0,0.2);
		}

		AmbientSound = default.AmbientSound;
	}
}

simulated state Firing
{
ignores Fire;
	simulated event EndState()
	{
		local bool bLastRound;
		local pawn Pawn;
		local playercontroller P;

		Super.EndState();

		if (AmmoIndex!=0)
			return;

		bLastRound = AmmoType.ReloadRequired(1);
		if( !bLastRound )	// note: conditional is inverted here.
		{
			// turn off view shake.
			Pawn = Pawn(Owner);
			if( Pawn!=None )
			{
				P = PlayerController(Pawn.Controller);
				if( P!=None )
					P.ShakeView(7.0,0.2);
			}
		}
	}
}

defaultproperties
{
	// text
    ItemName="Assault Rifle"
	IconIndex=3
	ItemID="AR"
	Crosshair="Crosshair_AR"

	// weapon data
	AutoSwitchPriority=15
	InventoryGroup=1
	GroupOffset=1
	//!! WeaponAnimationType=AT_Large
	PickupAmmoCount=300
	SoundRadius=400
	TraceDist=4800 // 100 yards

	// assets
	Mesh=SkeletalMesh'WeaponsK.AR_FP'
	AttachmentClass=class'AssaultAttachment'
	//!! FireSkins(0)=material'WeaponFXT.AR_Skin2FX'
	PlayerViewOffset=(X=20,Y=4,Z=-30)
	FireOffset=(X=40.0,Y=12.0,Z=-32.0)
	SelectSound=sound'WeaponsA.AssaultRifle.AR_Select'
	ReloadSound=sound'WeaponsA.AssaultRifle.AR_Reload'
	ReloadUnloadedSound=sound'WeaponsA.AssaultRifle.AR_ReloadUnloaded'

	AmmoName(0)=class'AssaultRifleAmmo'
	AmmoName(1)=class'AssaultRifleAmmoAlt'

	// additional timing
	ReloadTime=2.16
	ReloadUnloadedTime=1.83
}

