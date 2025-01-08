class XMPVehicleFactory extends KVehicleFactory;

var		Rotator					VehicleRotator;
var()	class<XMPVehicle>		VehicleClass;
var		float					SinceLastChecked;
var	(XMPVehicleFactory)	float					RespawnDelay;
var	(XMPVehicleFactory) int		TeamNumber;
var	(XMPVehicleFactory) int		VehicleCost;

function Tick(float Delta)
{
	local XMPVehicle CreatedVehicle;
	local Vector OffsetLocation;
	super.Tick(Delta);
	if (VehicleCount < 1)
	{
		SinceLastChecked+=Delta;
		if (SinceLastChecked > RespawnDelay)
		{
			SinceLastChecked=0;
//			if (XMPGame(Level.Game).IsTeamFeatureOnline( 'XMPVehicle', TeamNumber ))
//			{
				if(VehicleClass != None)
				{
					VehicleRotator = Rotation;
					VehicleRotator.Yaw += 32767;
					VehicleRotator.Pitch *= -1;
					VehicleRotator.Roll *= -1;
					OffsetLocation = Location;
					OffsetLocation.Z -= VehicleClass.default.WheelVert;
					OffsetLocation.Z += VehicleClass.default.FrontTireClass.default.CollisionHeight;
					CreatedVehicle = spawn(VehicleClass, , , OffsetLocation , VehicleRotator);
					if (CreatedVehicle?)
					{	
						VehicleCount++;
						CreatedVehicle.ParentFactory = self;
						//XMPGame(Level.Game).DrainTeamEnergy( TeamNumber, VehicleCost, 'VehicleResurrection' );
						Disable( 'Tick' );
					}
					else
						SinceLastChecked = RespawnDelay - 2;
				}
//			}
		}
	} 
}

defaultproperties
{
	VehicleCost=0.0050
	VehicleClass=class'Vehicles.XMPVehicle'
	MaxVehicleCount=1
	VehicleRotator=(Pitch=0,Yaw=0,Roll=0)
	RespawnDelay=60
}