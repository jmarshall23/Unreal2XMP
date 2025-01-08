
class EnergyRifleAmmo extends Ammunition;

defaultproperties
{
	ItemName="Energy Rifle clip"
	MaxAmmo=120
	AmmoAmount=30

	FireTime=0.3
	FireLastReloadTime=2.8
	FireLastDownTime=2.07
	FireLastRoundTime=1.44

	bRapidFire=true

	ShakeMag=1.0
	ShakeTime=0.2

	FireSound=sound'WeaponsA.EnergyRifle.ER_Fire'
	FireLastRoundSound=sound'WeaponsA.EnergyRifle.ER_FireLastRound'
	FirePitch=(A=0.95,B=1.0)

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}

