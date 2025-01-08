class RaptorGrinder extends Actor
	notplaceable;

var	vector	LocationOffset;
var	vector	LocationOffset2;
var rotator MySpin;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorGrinder_TD_001'
	bHardAttach=True
	DrawScale=1
	LocationOffset=(X=-151.97, Y=29.19, Z=-33.91)
	LocationOffset2=(X=-151.97, Y=-29.88, Z=-33.91)
	RemoteRole=ROLE_None
}