class MortarAmmo extends Ammunition;

defaultproperties
{
	bOnlyRelevantToOwner=false
	ItemName="Mortar shells"
	MaxAmmo=9999
	AmmoAmount=9999

	FireTime=3
	FireLastReloadTime=3.230
	FireLastDownTime=1.692
	FireLastRoundTime=1.0

	ProjectileClass=class'MortarProjectile'
	ShakeMag=24.0
	ShakeTime=0.5
	//FireSound=sound'WeaponsA.Shotgun.S_Fire'
	//FireLastRoundSound=sound'WeaponsA.Shotgun.S_FireLastRound'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
