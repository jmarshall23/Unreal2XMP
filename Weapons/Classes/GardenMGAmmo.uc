class GardenMGAmmo extends Ammunition;

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

	bRapidFire=true
	MomentumTransfer=2000
	MyDamageType=class'GardenMGDamage'
	TraceSpread=3.0
	ShakeMag=0.5
	ShakeTime=0.25
	AnimFire=Fire
	AnimFireEnd=FireEnd
	AnimFireLastReload=ReloadUnloaded
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=OutOfAmmo
}
