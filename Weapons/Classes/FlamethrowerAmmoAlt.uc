
class FlamethrowerAmmoAlt extends Ammunition;

defaultproperties
{
	ItemName="Canister of napalm"
	MaxAmmo=400
	AmmoAmount=80

	FireTime=0.1
	FireLastReloadTime=2.6
	FireLastDownTime=1.06
	FireLastRoundTime=0.66

	bRapidFire=true

	ShakeMag=0.0	//screws with sound (doppler)
	ShakeTime=0.0

	bAmbientFireSound=true
	FireSound=sound'WeaponsA.Flamethrower.FT_AltFire'
	FireLastRoundSound=sound'WeaponsA.Flamethrower.FT_AltFireLastRound'

	AnimFire=Fire
	AnimFireEnd=FireEnd
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
