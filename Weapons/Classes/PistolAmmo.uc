
class PistolAmmo extends Ammunition;

defaultproperties
{
	ItemName="Pistol clip"
	MaxAmmo=36
	AmmoAmount=9

	FireTime=0.6
	FireLastReloadTime=2.4
	FireLastDownTime=2.44
	FireLastRoundTime=2.1

    bInstantHit=true
	Damage=40
	MomentumTransfer=4000
	MyDamageType=class'PistolDamage'
	TraceSpread=0.0

	ShakeMag=1.0
	ShakeTime=0.1

	FireSound=sound'WeaponsA.Pistol.P_Fire'
	FireLastRoundSound=sound'WeaponsA.Pistol.P_FireLastRound'

	BloodEffect=class'Blood'
	SparkEffect=class'Sparks'
	DirtEffect=class'Dirt'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}

