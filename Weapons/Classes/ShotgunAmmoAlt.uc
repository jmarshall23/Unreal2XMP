//=============================================================================
// ShotgunAmmoAlt.uc -- ammo for the Shotgun
// $Author: Aleiby $
// $Date: 5/18/02 6:45p $
// $Revision: 2 $
//=============================================================================
class ShotgunAmmoAlt extends Ammunition;

defaultproperties
{
	ItemName="Shotgun shells"
	MaxAmmo=36
	AmmoAmount=6

	FireTime=1.18
	FireLastReloadTime=3.36
	FireLastDownTime=1.81
	FireLastRoundTime=0.57

	ProjectileClass=class'ShotgunProjectileAlt'
	ShakeMag=10.0
	ShakeTime=0.3
	FireSound=sound'WeaponsA.Shotgun.S_AltFire'
	FireLastRoundSound=sound'WeaponsA.Shotgun.S_AltFireLastRound'

	AnimFire=AltFire
	AnimFireLastReload=AltFireLastReload
	AnimFireLastDown=AltFireLastDown
	AnimFireLastRound=AltFireLastRound
}
