class InventoryAttachment extends Actor
	native
	nativereplication;

function InitFor(Inventory I)
{
	Instigator = I.Instigator;
}
		
defaultproperties
{
	bOnlyDrawIfAttached=true
	NetUpdateFrequency=10
	DrawType=DT_Mesh
	RemoteRole=ROLE_SimulatedProxy
	bAcceptsProjectors=True
	bUseLightingFromBase=True
    bOnlyDirtyReplication=true
}