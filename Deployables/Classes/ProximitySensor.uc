/*-----------------------------------------------------------------------------

*/

class ProximitySensor extends DeployedUnit
	placeable;

var()	float		TimeToTrack;
var()	float		ActionRate;					// refire rate (should come from weapon firerate but not working)
var()   bool		bAmbientActionSound;		//should the FireSound/AltFireSound be played as an ambient sound?
var()	float		TrackingRangeMultiplier;
var		Sound		TrackingSound;
var		Sound		ActionSound;
var		Sound		ActiveAlertSound;

//-----------------------------------------------------------------------------

function NotifyDeployed()
{	
	ProximitySensorController(Controller).NotifyDeployed();
	Super.NotifyDeployed();
}

//-----------------------------------------------------------------------------

function SetEnabled( bool bVal )
{
	bEnabled = bVal;
	if( Controller != None )
	{
		if( bEnabled )
			ProximitySensorController(Controller).Activate();
		else
			ProximitySensorController(Controller).Deactivate();
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	// assets
	DrawType=DT_StaticMesh
	//StaticMesh=StaticMesh'343M.Assets.ProximitySensor' //mdf-tbd:
	//DrawScale=2.350000
	StaticMesh=StaticMesh'XMPM.Items.FieldGenerator'
	SpamTexture0=Texture'JLSSGrey'
	SpamTexture1=Texture'JLSSGrey'
	SpamTexture255=Texture'JLSSGrey'
	
	TrackingSound=Sound'U2XMPA.ProximitySensor.ProximitySensorTracking'
	bAmbientActionSound=false
	ActionSound=Sound'U2XMPA.ProximitySensor.ProximitySensorAlert'
//	ActiveAlertSound=''
	ShutdownSound=Sound'U2XMPA.ProximitySensor.ProximitySensorShutdown'
	DisabledSound=Sound'U2XMPA.ProximitySensor.ProximitySensorDisabled'
	EnabledSound=Sound'U2XMPA.ProximitySensor.ProximitySensorEnabled'
	DeploySound=Sound'U2XMPA.ProximitySensor.ProximitySensorActivate'
	ExplosionClass=class'ExplosionProximitySensor'
	
	bEnabled=true
	Description="Proximity Sensor"
	AmbientNoiseSound=Sound'U2XMPA.ProximitySensor.ProximitySensorAmbient'
	SoundRadius=100
	TransientSoundRadius=700
	bDirectional=true
	PickupMessage="You picked up a Proximity Sensor."

	// physical
	Health=300
	CollisionHeight=70.000000
	CollisionRadius=8.00000
	DrawScale=0.110000
	PrePivot=(Z=-3.500000)

	//AI
	ControllerClass=class'ProximitySensorController'
	SightRadius=1024.0
	TrackingRangeMultiplier=1.5
	HearingThreshold=0.0
	Alertness=0.0
	TimeToTrack=1.000000
	ActionRate=1.500000
	BaseEyeHeight=48
//	DamageFilterClass=class'DamageFilterPsionic'
//	DamageFilterClass=class'DamageFilterDeployedUnit'

	InventoryType=class'ProximitySensorDeployable'
}
