
class JuggernautFlamethrowerAmmo extends Ammunition;

defaultproperties
{
	bOnlyRelevantToOwner=false
	ItemName="Canister of napalm"
	MaxAmmo=9999
	AmmoAmount=9999

	FireTime=0.1
	FireLastReloadTime=1.533
	FireLastDownTime=1.066
	FireLastRoundTime=1.0

	bRapidFire=true

	ShakeMag=0.0	//screws with sound (doppler)
	ShakeTime=0.0

	bAmbientFireSound=true
	FireSound=sound'WeaponsA.Flamethrower.FT_FireLoop'
	FireEndSound=sound'WeaponsA.Flamethrower.FT_FireEnd'
	FireLastRoundSound=sound'WeaponsA.Flamethrower.FT_FireLastRound'

	AnimFire=Fire
	AnimFireEnd=FireEnd
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=OutOfAmmo
}

