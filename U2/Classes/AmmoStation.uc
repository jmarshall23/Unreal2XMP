class AmmoStation extends PowerStation
	placeable;

//-----------------------------------------------------------------------------

function ActivateStation( Pawn P )
{
	Super.ActivateStation( P );
	// add back in any weapons that went offline, but are now supported by team energy
	Level.Game.AddDefaultWeapons( P );
}

//-----------------------------------------------------------------------------

function float CalcPowerUp( int CalcUnits )
{
	local float PowerUpUnits;
	// scale power up speed by # of ammo types that need ammo
	PowerUpUnits = Super.CalcPowerUp( CalcUnits );
	PowerUpUnits = class'UtilGame'.static.ScaleAmmoResupplyRate( PowerupOwner, PowerUpUnits );
	return PowerUpUnits;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	Skins(0)=Shader'XMPWorldItemsT.Items.Station_FX_Supply_001'
	Skins(2)=Shader'XMPWorldItemsT.Items.Station_FX_Supply_002'
	ParticleEffect=ParticleSalamander'Station_FX.ParticleSalamander3'
	AmmoUnits=999999
	TransferRate=25.000000	// as a percent of total ammo
	StationStartUsingSound=Sound'U2A.Stations.AmmoStationActivate'
	StationStopUsingSound=Sound'U2A.Stations.AmmoStationDeactivate'
	StationInUseSound=Sound'U2A.Stations.AmmoStationAmbient'
	StationErrorSound=Sound'U2A.Stations.AmmoStationError'
	ItemName="Ammo Station"
	Description="Ammo Station"
}