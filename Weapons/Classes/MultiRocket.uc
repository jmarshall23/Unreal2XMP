
class MultiRocket extends TurretWeapon;

var int RocketIndex;
var Vector RocketLocation[6];

function Projectile ProjectileFire()
{
	local Projectile SpawnedProjectile;
	if (RocketIndex >= 6)
		RocketIndex=0;
	AdjustedAim = Instigator.GetAimRotation();
	ProjectileFireStartLocation = Instigator.Location + -1*((XMPVehicleBarrel(Instigator).FiringOffset+RocketLocation[RocketIndex]) >> Instigator.Rotation);
	SpawnedProjectile = AmmoType.SpawnProjectile(ProjectileFireStartLocation,AdjustedAim);
	RocketIndex++;
	return SpawnedProjectile;
}

defaultproperties
{
	// text
    ItemName="MultiRocket"
	IconIndex=12
	ItemID="S"
	Crosshair="Crosshair_Pistol"

	// weapon data
	AutoSwitchPriority=10
	InventoryGroup=1
	GroupOffset=1
	PickupAmmoCount=9999

	// assets
	AttachmentClass=class'MultiRocketAttachment'
	PlayerViewOffset=(X=20,Y=4,Z=-32)
	FireOffset=(X=42.0,Y=9.0,Z=-34.0)
	AmmoName(0)=class'MultiRocketAmmo'
	EnergyCost=0.001;
	RocketLocation(0)=(X=0,Y=-35,Z=5)
	RocketLocation(1)=(X=0,Y=35,Z=5)
	RocketLocation(2)=(X=0,Y=-45,Z=0)
	RocketLocation(3)=(X=0,Y=45,Z=0)
	RocketLocation(4)=(X=0,Y=-35,Z=-5)
	RocketLocation(5)=(X=0,Y=35,Z=-5)

}

