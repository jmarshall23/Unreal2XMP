
class GrenadeLauncherAmmo extends Ammunition;

defaultproperties
{
	ItemName="Grenades"
	MaxAmmo=5
	AmmoAmount=1

	FireTime=1.33
	FireLastReloadTime=1.38
	FireLastDownTime=1.92
	FireLastRoundTime=1.38

	ProjectileClass=class'GrenadeProjectile'
	bSplashDamage=true
	bRecommendSplashDamage=true

	ShakeMag=4.0
	ShakeTime=0.1

	FireSound=sound'WeaponsA.GrenadeLauncher.FT_Fire'
	FireLastRoundSound=sound'WeaponsA.GrenadeLauncher.GL_FireLastRound'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}

