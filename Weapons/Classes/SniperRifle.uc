
class SniperRifle extends Weapon;

#exec OBJ LOAD File=..\Particles\SniperFX.u

var bool bZoomed, bWasZoomed;
var() float ZoomFOV;

var vector RingDir;
var float LastTimeSeconds;
var() float RingDelayFactor;
var float Angle, Radius;
var Point RingLocation;

var() Sound ZoomInSound;
var() Sound ZoomOutSound;
var() Sound AdjustZoomSound;

var ParticleGenerator Muzzleflash;

//------------------------------------------------------------------------------
simulated function PostBeginPlay()
{
	Super.PostBeginPlay();
	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		Muzzleflash = class'ParticleGenerator'.static.CreateNew( self, ParticleSalamander'SniperFX.ParticleSalamander0', Location );
		AttachToBone(Muzzleflash, '#Muzzleflash');
	}
}

//------------------------------------------------------------------------------
simulated function Destroyed()
{
	if (Muzzleflash?)
	{
		Muzzleflash.Destroy();
		Muzzleflash = None;
	}
	Super.Destroyed();
}

//------------------------------------------------------------------------------
simulated function PlayFiring()
{
	Super.PlayFiring();
	if (AmmoIndex==0 && !bZoomed)
		if (Muzzleflash?) Muzzleflash.Trigger(Self,Instigator);
}

//------------------------------------------------------------------------------
simulated event RenderOverlays( canvas Canvas )
{
	if (bZoomed)	// don't draw weapon model while zoomed.
	{
		UpdateCrosshair(Canvas);
		return;
	}

	Super.RenderOverlays(Canvas);
}

//------------------------------------------------------------------------------
simulated function HandleTargetDetails( Actor A, Canvas Canvas, vector ViewLoc, rotator ViewRot )
{
	local vector ScreenLoc;
	local float DeltaSeconds;
	local float Pct;

	if (bZoomed)
	{
		// Update delayed ring.
		ViewRot = Instigator.GetViewRotation();
		DeltaSeconds = Level.TimeSeconds - LastTimeSeconds;
		LastTimeSeconds = Level.TimeSeconds;
		Pct = FMin( 1.0, RingDelayFactor * DeltaSeconds );
		RingDir = normal(RingDir + ((normal(vector(ViewRot))-RingDir)*Pct));
		ScreenLoc = (RingDir*50) << ViewRot;
		RingLocation.X = ScreenLoc.Y;
		RingLocation.Y = -ScreenLoc.Z;

		// Update big ring.
		Angle = Instigator.Controller.FOVAngle;

		// Update zoom bars.
		Radius = (Instigator.Controller.FOVAngle/120 * 196) - 2;
	}
}

interface function float GetCrosshairAngle0(){ return Angle + 0; }
interface function float GetCrosshairAngle24(){ return Angle + 24; }
interface function float GetCrosshairAngle48(){ return Angle + 48; }
interface function float GetCrosshairAngle72(){ return Angle + 72; }
interface function float GetCrosshairAngle96(){ return Angle + 96; }
interface function float GetCrosshairAngle120(){ return Angle + 120; }
interface function float GetCrosshairAngle144(){ return Angle + 144; }
interface function float GetCrosshairAngle168(){ return Angle + 168; }
interface function float GetCrosshairAngle192(){ return Angle + 192; }
interface function float GetCrosshairAngle216(){ return Angle + 216; }
interface function float GetCrosshairAngle240(){ return Angle + 240; }
interface function float GetCrosshairAngle264(){ return Angle + 264; }
interface function float GetCrosshairAngle288(){ return Angle + 288; }
interface function float GetCrosshairAngle312(){ return Angle + 312; }
interface function float GetCrosshairAngle336(){ return Angle + 336; }
interface function float GetCrosshairRadius(){ return Radius; }
interface function Point GetCrosshairRingLocation(){ return RingLocation; }

simulated function PlaySelect()
{
	Super.PlaySelect();
	if (bZoomed)
		SendEvent("Rotate");
}

simulated state Firing
{
ignores Fire; //don't ignore AltFire, so that zooming effects will work
	
	//this is because the parent firing state basically sticks in a dummy 'AltFire' with its use of ignores,
	//which we need to override to call the global.altfire
	simulated function AltFire()
	{
		Global.AltFire();
	}
}

//override so altfiring state is not used, and so that we don't use ammo or switch weapons
simulated function AltFire()
{
	if (bZoomed)
		UnZoom();
	else
		Zoom();
}

simulated function NotifyCutsceneBegin()
{
	if (bZoomed)
		UnZoom(true);
}

//prevents weapon switching for the sniper rifle
simulated function Weapon PrevWeapon(Weapon CurrentChoice, Weapon CurrentWeapon)
{
	if (bZoomed) return None;
	return Super.PrevWeapon(CurrentChoice, CurrentWeapon);
}
simulated function Weapon NextWeapon(Weapon CurrentChoice, Weapon CurrentWeapon)
{
	if (bZoomed) return None;
	return Super.NextWeapon(CurrentChoice, CurrentWeapon);
}

simulated function SetPlayerFOV( float NewFOV )
{
	assert(bZoomed);

	ZoomFOV = FClamp( NewFOV, 1.0, 60.0 );

	if (PlayerController(Instigator.Controller)?)
	{
		PlayerController(Instigator.Controller).DesiredFOV = ZoomFOV;
	}
}

exec simulated function ZoomIn()
{
	if( bZoomed )
	{
		if( ZoomFOV > 20 )
		{
			// 90 --> 20 by 10
			SetPlayerFOV( ZoomFOV - 10 );
		}
		else if( ZoomFOV > 5 )
		{
			// 20 --> 5 by 5
			SetPlayerFOV( ZoomFOV - 5 );
		}
		else
		{
			// 5 --> 1 by 2
			SetPlayerFOV( ZoomFOV - 2 );
		}
		PlaySound( AdjustZoomSound );
	}
}

exec simulated function ZoomOut()
{
	if( bZoomed )
	{
		if( ZoomFOV >= 20 )
		{
			// 20 --> 90 by 10
			SetPlayerFOV( ZoomFOV + 10 );
		}
		else if( ZoomFOV >= 5 )
		{
			// 5 --> 20 by 5
			SetPlayerFOV( ZoomFOV + 5 );
		}
		else
		{
			// 1 --> 5 by 2
			SetPlayerFOV( ZoomFOV + 2 );
		}
		PlaySound( AdjustZoomSound );
	}	
}

simulated function Zoom()
{
	//only execute this code on the local machine and server ... not the third-person players
	if ( !bZoomed && Instigator.Controller != None )
	{
		bZoomed = true;
		SetPlayerFOV( ZoomFOV );
		SendEvent("Rotate");
		PlaySound( ZoomInSound );
	}	
}

simulated function UnZoom( optional bool bForce )
{
	//only execute this code on the local machine and server ... not the third-person players
	if ( bZoomed && Instigator.Controller != None )
	{
		bZoomed = false;
		if( PlayerController(Instigator.Controller) != None )
		{
			PlayerController(Instigator.Controller).DesiredFOV = PlayerController(Instigator.Controller).DefaultFOV;
			if( bForce )
				PlayerController(Instigator.Controller).FOVAngle = PlayerController(Instigator.Controller).DefaultFOV;
		}
		SendEvent("UnRotate");
		PlaySound( ZoomOutSound );
	}	
}

simulated state Unloading
{
	simulated event BeginState()
	{
		Super.BeginState();
		bWasZoomed = bZoomed;
		UnZoom();
	}
	simulated event EndState()
	{
		Super.EndState();
		if (bWasZoomed)
			Zoom();
	}
}

simulated state Reloading
{
	simulated event BeginState()
	{
		Super.BeginState();
		bWasZoomed = bZoomed;
		UnZoom();
	}
	simulated event EndState()
	{
		Super.EndState();
		if (bWasZoomed)
			Zoom();
	}
}

simulated state DownWeapon
{
ignores Fire, AltFire;

	simulated event BeginState()
	{
		UnZoom();
		Super.BeginState();
	}
}

function HandleTraceImpact( Actor Other, vector HitLocation, vector HitNormal )
{
	local SniperAttachment TP;
	Super.HandleTraceImpact(Other,HitLocation,HitNormal);
	TP = SniperAttachment(ThirdPersonActor);
	if (TP?)
		TP.HitLocation = HitLocation;
}


defaultproperties
{
	// text
    ItemName="Sniper Rifle"
	IconIndex=14
	ItemID="SR"
	Crosshair="Crosshair_SR"

	// weapon data
	AutoSwitchPriority=16
	InventoryGroup=1
	GroupOffset=1
	PickupAmmoCount=18
	TraceDist=28800 // 600 yards
	BobDamping=0.85

	// zoom data
	ZoomFOV=30
	ZoomInSound=sound'WeaponsA.SniperRifle.SR_ScopeOn'
	ZoomOutSound=sound'WeaponsA.SniperRifle.SR_ScopeOff'
	AdjustZoomSound=sound'WeaponsA.SniperRifle.SR_Zoom'
	RingDelayFactor=6.0

	// assets
	Mesh=SkeletalMesh'WeaponsK.SR_FP'
	AttachmentClass=class'SniperAttachment'
	//!! FireSkins(0)=material'WeaponFXT.SR_Skin2FX'
	PlayerViewOffset=(X=6,Y=1,Z=-24)
	FireOffset=(X=0,Y=0,Z=0)
	SelectSound=sound'WeaponsA.SniperRifle.SR_Select'
	ReloadSound=sound'WeaponsA.SniperRifle.SR_Reload'
	ReloadUnloadedSound=sound'WeaponsA.SniperRifle.SR_ReloadUnloaded'

	AmmoName(0)=class'SniperRifleAmmo'
	AmmoName(1)=class'SniperRifleAmmoAlt'

	// additional timing
	ReloadTime=1.875
	ReloadUnloadedTime=1.33
}

