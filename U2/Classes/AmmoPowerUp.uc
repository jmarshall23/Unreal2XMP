class AmmoPowerUp extends PowerUp;

event PostBeginPlay()
{
	Super.PostBeginPlay();

	// add back in any weapons that went offline, but are now supported by team energy
	if( Pawn(Owner)? )
		Level.Game.AddDefaultWeapons( Pawn(Owner) );
}

function float CalcPowerUp( int CalcUnits )
{
	local float PowerUpUnits;
	// scale powerup speed by # of ammo types that need ammo
	PowerUpUnits = Super.CalcPowerUp( CalcUnits );
	PowerUpUnits = class'UtilGame'.static.ScaleAmmoResupplyRate( PowerupOwner, PowerUpUnits );
	return PowerUpUnits;
}

defaultproperties
{
	ItemName="Ammo Pickup"
}