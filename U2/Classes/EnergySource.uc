//=============================================================================
// EnergySource.uc
//=============================================================================

class EnergySource extends HackSwitch
	placeable;

//-----------------------------------------------------------------------------

var float EnergyRate;				// energy added to team per second

//-----------------------------------------------------------------------------

function Dispatch()
{
	local int i;
	local Controller Hacker;

	for( i=0; i < Users.Length; i++ )
		if( GetUserTeam( Users[i] ) == Team )
			Hacker = Users[i];
	
	Super.Dispatch();
}

//-----------------------------------------------------------------------------

function RegisterHackScore( Controller PrimaryHacker, Controller SecondaryHacker )
{
	XMPGame(Level.Game).NotifyHackedEnergySource( Self, PrimaryHacker, SecondaryHacker );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	bBlockActors=true
	bBlockPlayers=true
	bBlockNonZeroExtentTraces=true
	Description="Generator"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.EnergySource.Generator_MT_001'
	AmbientSound=sound'U2XMPA.EnergyRelay.EnergyRelayAmbient'
	HackCompletedSound=sound'U2XMPA.EnergyRelay.EnergyRelayActivate'
	RepSkinIndex=4
	AlternateSkins(0)=Shader'XMPWorldItemsT.EnergySource.metl_generator_MT_001_red'
	AlternateSkins(1)=Shader'XMPWorldItemsT.EnergySource.metl_generator_MT_001_blue'
	EnergyRate=0.0020
	HackGainedMessage=class'EnergySourceGainedMessage'
	HackLostMessage=class'EnergySourceLostMessage'
}
