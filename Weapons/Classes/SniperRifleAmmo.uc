
class SniperRifleAmmo extends Ammunition;

defaultproperties
{
	ItemName="Sniper Rifle clip"
	MaxAmmo=15
	AmmoAmount=3

	FireTime=1.0
	FireLastReloadTime=2.53
	FireLastDownTime=2.44
	FireLastRoundTime=2.27

    bInstantHit=true
	Damage=75
	MomentumTransfer=25000
	MyDamageType=class'SniperDamage'
	TraceSpread=0.0

	ShakeMag=3.0
	ShakeTime=0.15

	FireSound=sound'WeaponsA.SniperRifle.SR_Fire'
	FireLastRoundSound=sound'WeaponsA.SniperRifle.SR_FireLastRound'

	BloodEffect=class'Blood'
	SparkEffect=class'Sparks'
	DirtEffect=class'Dirt'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}

