
class RocketLauncherAmmoAlt extends Ammunition;

defaultproperties
{
	ItemName="Rockets"
	MaxAmmo=20
	AmmoAmount=5

	FireTime=1.08
	FireLastReloadTime=4.0
	FireLastDownTime=2.5
	FireLastRoundTime=2.17

	ProjectileClass=class'RocketProjectileAlt'
	bSplashDamage=true
	bRecommendSplashDamage=true
	ShakeMag=3.0
	ShakeTime=0.15
	FireSound=sound'WeaponsA.RocketLauncher.RL_AltFire'
	FireLastRoundSound=sound'WeaponsA.RocketLauncher.RL_AltFireLastRound'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
