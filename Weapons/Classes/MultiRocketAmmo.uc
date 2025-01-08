class MultiRocketAmmo extends Ammunition;

simulated function bool UseAmmo( optional int AmountNeeded )
{
return true;
}

defaultproperties
{
	bOnlyDirtyReplication=false
	bOnlyRelevantToOwner=false
	bAlwaysRelevant=true

	ItemName="Mini Rockets"
	MaxAmmo=9999
	AmmoAmount=9999

	FireTime=0.18
	FireLastReloadTime=3.230
	FireLastDownTime=1.692
	FireLastRoundTime=1.0

	ProjectileClass=class'MultiRocketProjectile'
	ShakeMag=3.0
	ShakeTime=0.25
	FireSound=sound'WeaponsA.RocketLauncher.RL_Fire'
	//FireLastRoundSound=sound'WeaponsA.Shotgun.S_FireLastRound'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
