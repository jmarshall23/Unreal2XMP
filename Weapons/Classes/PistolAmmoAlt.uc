
class PistolAmmoAlt extends Ammunition;

defaultproperties
{
	ItemName="Pistol clip"
	MaxAmmo=36
	AmmoAmount=9
	AmmoUsed=3

	FireTime=1.1
	FireLastReloadTime=2.4
	FireLastDownTime=2.43
	FireLastRoundTime=2.1

    bInstantHit=true
	Damage=150
	MomentumTransfer=8000
	MyDamageType=class'PistolDamage'
	TraceSpread=0.15

	ShakeMag=1.5
	ShakeTime=0.15

	FireSound=sound'WeaponsA.Pistol.P_AltFire'
	FireLastRoundSound=sound'WeaponsA.Pistol.P_AltFireLastRound'

	BloodEffect=class'Blood'
	SparkEffect=class'Sparks'
	DirtEffect=class'Dirt'

	AnimFire=AltFire
	AnimFireLastReload=AltFireLastReload
	AnimFireLastDown=AltFireLastDown
	AnimFireLastRound=AltFireLastRound
}

