class WingedCannonAmmo extends Ammunition;

defaultproperties
{
	bOnlyRelevantToOwner=false
	ItemName="Heavy Machinegun Clip"
	MaxAmmo=9999
	AmmoAmount=9999

	FireTime=0.1
	FireLastReloadTime=1.8333
	FireLastDownTime=1.0
	FireLastRoundTime=0.615

	bInstantHit=false
	bRapidFire=true
	//Damage=12
	MomentumTransfer=2000
	MyDamageType=class'WingedCannonDamage'
	TraceSpread=3.0
	ShakeMag=1.0
	ShakeTime=0.25
//	bAmbientFireSound=true
//	FireSound=sound'WeaponsA.AssaultRifle.AR_Fire'
//	FireEndSound=sound'WeaponsA.AssaultRifle.AR_FireEnd'
//	FireLastRoundSound=sound'WeaponsA.AssaultRifle.AR_FireLastRound'

	AnimFire=Fire
	AnimFireEnd=FireEnd
	AnimFireLastReload=ReloadUnloaded
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=OutOfAmmo
}
