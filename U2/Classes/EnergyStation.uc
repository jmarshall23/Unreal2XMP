class EnergyStation extends PowerStation
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	Skins(0)=Shader'XMPWorldItemsT.Items.Station_FX_Energy_001'
	Skins(2)=Shader'XMPWorldItemsT.Items.Station_FX_Energy_002'
	ParticleEffect=ParticleSalamander'Station_FX.ParticleSalamander1'
	EnergyUnits=999999
	TransferRate=25.000000
	StationStartUsingSound=Sound'U2A.Stations.EnergyStationActivate'
	StationStopUsingSound=Sound'U2A.Stations.EnergyStationDeactivate'
	StationInUseSound=Sound'U2A.Stations.EnergyStationAmbient'
	StationErrorSound=Sound'U2A.Stations.EnergyStationError'
	ItemName="Energy Station"
	Description="Energy Station"
}
