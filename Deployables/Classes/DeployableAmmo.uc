
class DeployableAmmo extends Ammunition;

function bool AddAmmo(int AmmoToAdd)
{
	SetAmmoAmount( AmmoAmount+AmmoToAdd );	// no clamping.
	return true;
}

simulated function bool UseAmmo( optional int AmountNeeded ) { return false; }
simulated function bool DeployableUseAmmo( optional int AmountNeeded ) { return Super.UseAmmo(AmountNeeded); }

defaultproperties
{
	ItemName="Deployable Item"
	MaxAmmo=9
	AmmoAmount=1

	FireTime=1.68
	FireLastReloadTime=1.68
	FireLastDownTime=1.68
	FireLastRoundTime=1.68

	bInstantHit=true
	bRepeatFire=false

	ShakeMag=0.0
	ShakeTime=0.0

	AnimFire=Fire
	AnimFireLastReload=Fire
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
