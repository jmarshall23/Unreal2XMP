class XMPGameMessage extends GameMessage;

var localized string OfflineRedRocketTurrets, OfflineBlueRocketTurrets;
var localized string OfflineRedFieldGenerators,	OfflineBlueFieldGenerators;
var localized string OfflineRedLandMines, OfflineBlueLandMines;
var localized string OfflineRedLaserTripMines, OfflineBlueLaserTripMines;
var localized string OfflineRedAutoTurrets, OfflineBlueAutoTurrets;
var localized string OfflineRedSupplyPacks, OfflineBlueSupplyPacks;
var localized string OfflineRedIconNodes, OfflineBlueIconNodes;
var localized string OfflineRedMannedTurrets, OfflineBlueMannedTurrets;
var localized string OfflineRedSupplyStations, OfflineBlueSupplyStations;
var localized string OfflineRedVehicles, OfflineBlueVehicles;

var localized string WarningRedRocketTurrets, WarningBlueRocketTurrets;
var localized string WarningRedFieldGenerators, WarningBlueFieldGenerators;
var localized string WarningRedLandMines, WarningBlueLandMines;
var localized string WarningRedLaserTripMines, WarningBlueLaserTripMines;
var localized string WarningRedAutoTurrets, WarningBlueAutoTurrets;
var localized string WarningRedSupplyPacks, WarningBlueSupplyPacks;
var localized string WarningRedIconNodes, WarningBlueIconNodes;
var localized string WarningRedMannedTurrets, WarningBlueMannedTurrets;
var localized string WarningRedSupplyStations, WarningBlueSupplyStations;
var localized string WarningRedVehicles, WarningBlueVehicles;

var localized string OnlineRedRocketTurrets, OnlineBlueRocketTurrets;
var localized string OnlineRedFieldGenerators,	OnlineBlueFieldGenerators;
var localized string OnlineRedLandMines, OnlineBlueLandMines;
var localized string OnlineRedLaserTripMines, OnlineBlueLaserTripMines;
var localized string OnlineRedAutoTurrets, OnlineBlueAutoTurrets;
var localized string OnlineRedSupplyPacks, OnlineBlueSupplyPacks;
var localized string OnlineRedIconNodes, OnlineBlueIconNodes;
var localized string OnlineRedMannedTurrets, OnlineBlueMannedTurrets;
var localized string OnlineRedSupplyStations, OnlineBlueSupplyStations;
var localized string OnlineRedVehicles, OnlineBlueVehicles;

var sound OnlineSound, WarningSound, OfflineSound;

static function string GetString( optional int Switch,
								  optional PlayerReplicationInfo RelatedPRI_1, 
								  optional PlayerReplicationInfo RelatedPRI_2,
								  optional Object OptionalObject )
{
	switch (Switch)
	{
	case 0:		return default.OfflineRedSupplyPacks;
	case 1:		return default.OfflineRedMannedTurrets;
	case 2:		return default.OfflineRedRocketTurrets;
	case 3:		return default.OfflineRedLandMines;
	case 4:		return default.OfflineRedFieldGenerators;
	case 5:		return default.OfflineRedLaserTripMines;
	case 6:		return default.OfflineRedAutoTurrets;
	case 7:		return default.OfflineRedVehicles;
	case 8:		return default.OfflineRedSupplyStations;
	case 9:		return default.OfflineRedIconNodes;

	case 10:	return default.OfflineBlueSupplyPacks;
	case 11:	return default.OfflineBlueMannedTurrets;
	case 12:	return default.OfflineBlueRocketTurrets;
	case 13:	return default.OfflineBlueLandMines;
	case 14:	return default.OfflineBlueFieldGenerators;
	case 15:	return default.OfflineBlueLaserTripMines;
	case 16:	return default.OfflineBlueAutoTurrets;
	case 17:	return default.OfflineBlueVehicles;
	case 18:	return default.OfflineBlueSupplyStations;
	case 19:	return default.OfflineBlueIconNodes;
					   
	case 20:	return default.WarningRedSupplyPacks;
	case 21:	return default.WarningRedMannedTurrets;
	case 22:	return default.WarningRedRocketTurrets;
	case 23:	return default.WarningRedLandMines;
	case 24:	return default.WarningRedFieldGenerators;
	case 25:	return default.WarningRedLaserTripMines;
	case 26:	return default.WarningRedAutoTurrets;
	case 27:	return default.WarningRedVehicles;
	case 28:	return default.WarningRedSupplyStations;
	case 29:	return default.WarningRedIconNodes;

	case 30:	return default.WarningBlueSupplyPacks;
	case 31:	return default.WarningBlueMannedTurrets;
	case 32:	return default.WarningBlueRocketTurrets;
	case 33:	return default.WarningBlueLandMines;
	case 34:	return default.WarningBlueFieldGenerators;
	case 35:	return default.WarningBlueLaserTripMines;
	case 36:	return default.WarningBlueAutoTurrets;
	case 37:	return default.WarningBlueVehicles;
	case 38:	return default.WarningBlueSupplyStations;
	case 39:	return default.WarningBlueIconNodes;

	case 40:	return default.OnlineRedSupplyPacks;
	case 41:	return default.OnlineRedMannedTurrets;
	case 42:	return default.OnlineRedRocketTurrets;
	case 43:	return default.OnlineRedLandMines;
	case 44:	return default.OnlineRedFieldGenerators;
	case 45:	return default.OnlineRedLaserTripMines;
	case 46:	return default.OnlineRedAutoTurrets;
	case 47:	return default.OnlineRedVehicles;
	case 48:	return default.OnlineRedSupplyStations;
	case 49:	return default.OnlineRedIconNodes;

	case 50:	return default.OnlineBlueSupplyPacks;
	case 51:	return default.OnlineBlueMannedTurrets;
	case 52:	return default.OnlineBlueRocketTurrets;
	case 53:	return default.OnlineBlueLandMines;
	case 54:	return default.OnlineBlueFieldGenerators;
	case 55:	return default.OnlineBlueLaserTripMines;
	case 56:	return default.OnlineBlueAutoTurrets;
	case 57:	return default.OnlineBlueVehicles;
	case 58:	return default.OnlineBlueSupplyStations;
	case 59:	return default.OnlineBlueIconNodes;
	}

	return "";
}

static function color GetColor(
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2
	)
{
	if( Switch < 20 )
		return ColorRed();
	else if( Switch < 40 )
		return ColorYellow();
	else
		return ColorGreen();
}

static function ClientReceive( 
	PlayerController P,
	optional int Switch,
	optional PlayerReplicationInfo RelatedPRI_1, 
	optional PlayerReplicationInfo RelatedPRI_2,
	optional Object OptionalObject
	)
{
	local sound NotifySound;
	if( Switch < 20 )
		NotifySound = Default.OfflineSound;
	else if( Switch < 40 )
		NotifySound = Default.WarningSound;
	else
		NotifySound = Default.OnlineSound;
	class'Console'.static.ShowStatusMessage( P, GetString(Switch,RelatedPRI_1,RelatedPRI_2,OptionalObject), Default.Lifetime, Default.MessageFont, GetColor(Switch,RelatedPRI_1,RelatedPRI_2), NotifySound,,, Default.MessageHolder, Default.MessageLabel );
}

defaultproperties
{
	MessageHolder="EnergyMessageHolder"

	OnlineSound=sound'U2XMPA.EnergyNotify.EnergyOnline'
	WarningSound=sound'U2XMPA.EnergyNotify.EnergyWarning'
	OfflineSound=sound'U2XMPA.EnergyNotify.EnergyOffline'

	OfflineRedRocketTurrets="Red team's Rocket Turrets are offline"
	OfflineRedFieldGenerators="Red team's FieldGenerators are offline"
	OfflineRedLandMines="Red team's LandMines are offline"
	OfflineRedLaserTripMines="Red team's LaserTripMines are offline"
	OfflineRedAutoTurrets="Red team's AutoTurrets are offline"
	OfflineRedSupplyPacks="Red team's Supply Packs are offline"
	OfflineRedIconNodes="Red team can't register artifacts"
	OfflineRedMannedTurrets="Red team's Manned Turrets are offline"
	OfflineRedSupplyStations="Red team's Supply Stations are offline"
	OfflineRedVehicles="Red team's Vehicles are offline"
	OfflineBlueRocketTurrets="Blue team's Rocket Turrets are offline"
	OfflineBlueFieldGenerators="Blue team's FieldGenerators are offline"
	OfflineBlueLandMines="Blue team's LandMines are offline"
	OfflineBlueLaserTripMines="Blue team's LaserTripMines are offline"
	OfflineBlueAutoTurrets="Blue team's AutoTurrets are offline"
	OfflineBlueSupplyPacks="Blue team's Supply Packs are offline"
	OfflineBlueIconNodes="Blue team can't register artifacts"
	OfflineBlueMannedTurrets="Blue team's Manned Turrets are offline"
	OfflineBlueSupplyStations="Blue team's Supply Stations are offline"
	OfflineBlueVehicles="Blue team's Vehicles are offline"
									
	WarningRedRocketTurrets="Red team's Rocket Turrets are in danger"
	WarningRedFieldGenerators="Red team's FieldGenerators are in danger"
	WarningRedLandMines="Red team's LandMines are in danger"
	WarningRedLaserTripMines="Red team's LaserTripMines are in danger"
	WarningRedAutoTurrets="Red team's AutoTurrets are in danger"
	WarningRedSupplyPacks="Red team's Supply Packs are in danger"
	WarningRedIconNodes="Red team's Icon Node is in danger"
	WarningRedMannedTurrets="Red team's Manned Turrets are in danger"
	WarningRedSupplyStations="Red team's Supply Stations are in danger"
	WarningRedVehicles="Red team's Vehicles are in danger"
	WarningBlueRocketTurrets="Blue team's Rocket Turrets are in danger"
	WarningBlueFieldGenerators="Blue team's FieldGenerators are in danger"
	WarningBlueLandMines="Blue team's LandMines are in danger"
	WarningBlueLaserTripMines="Blue team's LaserTripMines are in danger"
	WarningBlueAutoTurrets="Blue team's AutoTurrets are in danger"
	WarningBlueSupplyPacks="Blue team's Supply Packs are in danger"
	WarningBlueIconNodes="Blue team's IconNode is in danger"
	WarningBlueMannedTurrets="Blue team's Manned Turrets are in danger"
	WarningBlueSupplyStations="Blue team's Supply Stations are in danger"
	WarningBlueVehicles="Blue team's Vehicles are in danger"
									
	OnlineRedRocketTurrets="Red team's Rocket Turrets are online"
	OnlineRedFieldGenerators="Red team's FieldGenerators are online"
	OnlineRedLandMines="Red team's LandMines are online"
	OnlineRedLaserTripMines="Red team's LaserTripMines are online"
	OnlineRedAutoTurrets="Red team's AutoTurrets are online"
	OnlineRedSupplyPacks="Red team's Supply Packs are online"
	OnlineRedIconNodes="Red team can register artifacts"
	OnlineRedMannedTurrets="Red team's Manned Turrets are online"
	OnlineRedSupplyStations="Red team's Supply Stations are online"
	OnlineRedVehicles="Red team's Vehicles are online"
	OnlineBlueRocketTurrets="Blue team's Rocket Turrets are online"
	OnlineBlueFieldGenerators="Blue team's FieldGenerators are online"
	OnlineBlueLandMines="Blue team's LandMines are online"
	OnlineBlueLaserTripMines="Blue team's LaserTripMines are online"
	OnlineBlueAutoTurrets="Blue team's AutoTurrets are online"
	OnlineBlueSupplyPacks="Blue team's Supply Packs are online"
	OnlineBlueIconNodes="Blue team can register artifacts"
	OnlineBlueMannedTurrets="Blue team's Manned Turrets are online"
	OnlineBlueSupplyStations="Blue team's Supply Stations are online"
	OnlineBlueVehicles="Blue team's Vehicles are online"
}