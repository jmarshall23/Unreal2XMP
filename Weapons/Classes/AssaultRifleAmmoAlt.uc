//=============================================================================
// AssaultRifleAmmoAlt.uc -- ammo for the Assault Rifle
// $Author: Aleiby $
// $Date: 5/18/02 6:45p $
// $Revision: 2 $
//=============================================================================
class AssaultRifleAmmoAlt extends Ammunition;

defaultproperties
{
	ItemName="Assault Rifle clip"
	MaxAmmo=225
	AmmoAmount=75
	AmmoUsed=5

	FireTime=1.0
	FireLastReloadTime=2.8
	FireLastDownTime=1.6
	FireLastRoundTime=1.1
	ProjectileClass=class'AssaultProjectileAlt'
	TraceSpread=3.0
	ShakeMag=10.0
	ShakeTime=0.3
	FireSound=sound'WeaponsA.AssaultRifle.AR_AltFire'
	FireLastRoundSound=sound'WeaponsA.AssaultRifle.AR_AltFireLastRound'

	AnimFire=AltFire
	AnimFireLastReload=AltFireLastReload
	AnimFireLastDown=AltFireLastDown
	AnimFireLastRound=AltFireLastRound
}
