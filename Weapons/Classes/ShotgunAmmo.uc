//=============================================================================
// ShotgunAmmo.uc -- ammo for the Shotgun
// $Author: Aleiby $
// $Date: 5/18/02 6:45p $
// $Revision: 2 $
//=============================================================================
class ShotgunAmmo extends Ammunition;

defaultproperties
{
	ItemName="Shotgun shells"
	MaxAmmo=36
	AmmoAmount=6

	FireTime=0.93
	FireLastReloadTime=3.23
	FireLastDownTime=1.69
	FireLastRoundTime=1.46

	ProjectileClass=class'ShotgunProjectile'
	ShakeMag=12.0
	ShakeTime=0.25
	FireSound=sound'WeaponsA.Shotgun.S_Fire'
	FireLastRoundSound=sound'WeaponsA.Shotgun.S_FireLastRound'

	AnimFire=Fire
	AnimFireLastReload=FireLastReload
	AnimFireLastDown=FireLastDown
	AnimFireLastRound=FireLastRound
}
