
class EnergyCannonAmmo extends Ammunition;

defaultproperties
{
	bOnlyRelevantToOwner=false
	ItemName="Energy Cannon clip"
	MaxAmmo=9999
	AmmoAmount=9999

	FireTime=0.1
	FireLastReloadTime=2.0
	FireLastDownTime=2.0
	FireLastRoundTime=2.0

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

