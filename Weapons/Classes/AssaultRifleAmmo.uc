//=============================================================================
// AssaultRifleAmmo.uc -- ammo for the Assault Rifle
// $Author: Aleiby $
// $Date: 5/18/02 6:45p $
// $Revision: 2 $
//=============================================================================
class AssaultRifleAmmo extends Ammunition;

defaultproperties
{
	ItemName="Assault Rifle clip"
	MaxAmmo=225
	AmmoAmount=75

	FireTime=0.1
	FireLastReloadTime=2.166
	FireLastDownTime=1.125
	FireLastRoundTime=0.625
	bInstantHit=true
	bRapidFire=true
	Damage=20
	MomentumTransfer=2000
	MyDamageType=class'AssaultDamage'
	TraceSpread=6.0
	ShakeMag=3.0
	ShakeTime=1.5
	bAmbientFireSound=true
	FireSound=sound'WeaponsA.AssaultRifle.AR_Fire'
	FireEndSound=sound'WeaponsA.AssaultRifle.AR_FireEnd'
	FireLastRoundSound=sound'WeaponsA.AssaultRifle.AR_FireLastRound'

	BloodEffect=class'Blood'
	SparkEffect=class'Sparks'
	DirtEffect=class'Dirt'

//	AnimFire='Fire' -- name import code appears broken.
	AnimFire=Fire
	AnimFireEnd=FireEnd
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
