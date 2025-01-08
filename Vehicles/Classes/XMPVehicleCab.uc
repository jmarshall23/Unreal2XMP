class XMPVehicleCab extends Actor
	notplaceable;

function TakeDamage(int Damage, Pawn instigatedBy, Vector hitlocation, 
						Vector momentum, class<DamageType> damageType)
{
		ConglomerateID.TakeDamage(Damage, instigatedBy, hitlocation, momentum, damageType);
}

function OnUse (Actor User)
{
XMPVehicle(ConglomerateID).TryToDrive(Controller(User).Pawn, 0);
}

simulated event PostNetBeginPlay()
{
	super.PostNetBeginPlay();
	if (ConglomerateID? && (Base != ConglomerateID))
	{
		SetBase(ConglomerateID);
	}
	if (RelativeLocation != vect(0,0,0))
		SetRelativeLocation(vect(0,0,0));
}

simulated event postnetreceive()
{
	super.postnetreceive();
	if (ConglomerateID? && (Base != ConglomerateID))
	{
		SetBase(ConglomerateID);
	}
	if (RelativeLocation != vect(0,0,0))
		SetRelativeLocation(vect(0,0,0));
}

defaultproperties
{
	bProjTarget=true
	bUsable=true
	bCollideWorld=false
	bBlockActors=false
	bCollideActors=true	
	bBlockPlayers=false
	bBlockZeroExtentTraces=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'VehiclesM.RaptorCab_TD_001'
	bHardAttach=True
	DrawScale=1
	RemoteRole=ROLE_SimulatedProxy
	bNetInitialRotation=true
	bBlockKarma=false
	bIgnoreEncroachers=true
	bNetNotify=true
	//bAlwaysRelevant=True
}