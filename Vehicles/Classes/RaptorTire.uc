class RaptorTire extends XMPVehicleTire;

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorTire_TD_001'

    Begin Object Class=KarmaParamsRBFull Name=KParams0
        KInertiaTensor(0)=1.8
        KInertiaTensor(1)=0
        KInertiaTensor(2)=0
        KInertiaTensor(3)=1.8
        KInertiaTensor(4)=0
        KInertiaTensor(5)=1.8
		KAngularDamping=0
		KLinearDamping=0
        bHighDetailOnly=False
        bClientOnly=False
		bKDoubleTickRate=True
        Name="KParams0"
    End Object
    KParams=KarmaParams'KParams0'

	DrawScale=1

	CollisionHeight=34
	CollisionRadius=34
}