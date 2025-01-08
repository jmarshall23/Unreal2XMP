
class EnergyRifleAmmoAlt extends Ammunition;

defaultproperties
{
	ItemName="Energy Rifle clip"
	MaxAmmo=120
	AmmoAmount=30
	AmmoUsed=10

	FireTime=2.0
	FireLastReloadTime=2.265
	FireLastDownTime=2.25
	FireLastRoundTime=1.44

	ProjectileClass=class'EMPProjectile'
	bSplashDamage=true
	bRecommendSplashDamage=true

	ShakeMag=15.0
	ShakeTime=0.8

	FireSound=sound'WeaponsA.EnergyRifle.ER_AltFire'
	FireLastRoundSound=sound'WeaponsA.EnergyRifle.ER_AltFireLastRound'

	AnimFire=AltFire
	AnimFireLastReload=AltFireLastReload
	AnimFireLastDown=AltFireLastDown
	AnimFireLastRound=AltFireLastRound
}

