
class GrenadeLauncher extends Weapon;

var() float TimedGrenadeChargeThreshold;
var float BeginChargeTime;
var int NumTicks;
var bool bLastGrenadeTimed;
var bool bCharging;

replication
{
	reliable if( Role<ROLE_Authority )
		StopCharging;
}

function ServerFire(optional bool bAltFire)
{
	Super.ServerFire(bAltFire);
	if( IsInState('Charging') )
		bCharging=true;
}
function StopCharging()
{
	bCharging=false;
}

function AuthorityFire(){}
simulated function EverywhereFire()
{
	GotoState('Charging');
}

simulated state Charging
{
ignores Fire,AltFire;

	simulated event BeginState()
	{
		BeginChargeTime = Level.TimeSeconds;
		NumTicks = 0;
		Enable('Tick');
	}

	simulated event EndState()
	{
		Disable('Tick');
	}

	simulated event Tick( float DeltaSeconds )
	{
		NumTicks++;

		//Global.Tick(DeltaSeconds);
		if( !((Role == ROLE_Authority && bCharging) || ((AmmoIndex == 0) && Instigator.PressingFire()) || ((AmmoIndex == 1) && Instigator.PressingAltFire())) )
		{
			StopCharging();

			if (Role == ROLE_Authority)
				Super.AuthorityFire();
			Super.EverywhereFire();
		}

	}

	function Projectile ProjectileFire()
	{
		local Projectile P;
		P = Global.ProjectileFire();
		if( P != None ) // in case tried to spawn outside level etc.
		{
			if( Level.TimeSeconds - BeginChargeTime >= TimedGrenadeChargeThreshold )
				if( NumTicks > 1 )	// don't punish people with slow framerates by not allowing them to fire contact grenades.
					P.bBounce = true;

			bLastGrenadeTimed = P.bBounce;
		}

		return P;
	}
}

function bool PreSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	return true;
}

function PostSetAimingParameters( bool bAltFire, bool bInstant, float FireSpread, class<projectile> ProjClass, bool bWarnTarget, bool bTrySplash )
{
	Super.PreSetAimingParameters( bAltFire, bInstant, FireSpread, ProjClass, bWarnTarget, bTrySplash );
}

defaultproperties
{
	// text
    ItemName="Grenade Launcher"
	IconIndex=11
	ItemID="GL"
	Crosshair="Crosshair_GL"

	// weapon data
	AutoSwitchPriority=13
	InventoryGroup=3
	GroupOffset=1
	PickupAmmoCount=6

	// assets
	Mesh=SkeletalMesh'WeaponsK.GL_FP'
	AttachmentClass=class'GrenadeLauncherAttachment'
	//!! FireSkins(0)=material'WeaponFXT.GL_Skin2FX'
	PlayerViewOffset=(X=20,Y=6,Z=-30)
	FireOffset=(X=0,Y=0,Z=0)
	SelectSound=sound'WeaponsA.GrenadeLauncher.GL_Select'
	ReloadSound=sound'WeaponsA.GrenadeLauncher.GL_Reload'
	ReloadUnloadedSound=sound'WeaponsA.GrenadeLauncher.GL_ReloadUnloaded'

	AmmoName(0)=class'GrenadeLauncherAmmo'
	AmmoName(1)=class'GrenadeLauncherAmmo'

	// additional timing
	ReloadTime=1.38
	ReloadUnloadedTime=1.38

	TimedGrenadeChargeThreshold=0.5
}

