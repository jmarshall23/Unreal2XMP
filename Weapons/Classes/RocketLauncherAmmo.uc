
class RocketLauncherAmmo extends Ammunition;

defaultproperties
{
	ItemName="Rockets"
	MaxAmmo=20
	AmmoAmount=5

	FireTime=1.08
	FireLastReloadTime=4.0
	FireLastDownTime=2.5
	FireLastRoundTime=2.17

	ProjectileClass=class'RocketProjectileHeavy'
	bSplashDamage=true
	bRecommendSplashDamage=true
	ShakeMag=14.0
	ShakeTime=0.3
	FireSound=sound'WeaponsA.RocketLauncher.RL_Fire'
	FireLastRoundSound=sound'WeaponsA.RocketLauncher.RL_FireLastRound'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
