class HealthStation extends PowerStation
	placeable;

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	Skins(0)=Shader'XMPWorldItemsT.Items.Station_FX_Health_001'
	Skins(2)=Shader'XMPWorldItemsT.Items.Station_FX_Health_002'
	ParticleEffect=ParticleSalamander'Station_FX.ParticleSalamander2'
	HealthUnits=999999
	TransferRate=25.000000
	StationStartUsingSound=Sound'U2A.Stations.HealthStationActivate'
	StationStopUsingSound=Sound'U2A.Stations.HealthStationDeactivate'
	StationInUseSound=Sound'U2A.Stations.HealthStationAmbient'
	StationErrorSound=Sound'U2A.Stations.HealthStationError'
	ItemName="Health Station"
	Description="Health Station"
}