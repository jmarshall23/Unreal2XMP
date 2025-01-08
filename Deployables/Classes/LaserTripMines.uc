
class LaserTripMines extends MineBase
	placeable;

//-----------------------------------------------------------------------------

var() array<TripLaser> TeamLaserEffects;
var() TripLaser LaserFX;
var vector StartPt, EndPt;				// endpoints of laser
var actor EndActor;						// actor that the end of the laser hit

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && Role==ROLE_Authority )
		StartPt, EndPt, EndActor;
}

//-----------------------------------------------------------------------------

simulated event PreBeginPlay()
{
	Super.PreBeginPlay();

	// On the client, use a timer to wait the arming delay before
	// spawning the laser.  On the server, this is accomplished through state code
	// of the parent class in the Armed state.  Since these are instantly placed,
	// and not tossed, they should be armed at the same time on client and server.
	if( Level.NetMode == NM_Client )
		AddTimer( 'CreateTheLaserBeam', ArmingDelay );
}

//-----------------------------------------------------------------------------

simulated event Destroyed()
{
	if( LaserFX? )
		LaserFX.Destroy();
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

simulated function CreateTheLaserBeam()
{
	local int EffectsIndex;

	EffectsIndex = GetTeam();
	if( EffectsIndex < TeamLaserEffects.Length )
	{
		if( !LaserFX )
		{
			LaserFX = TripLaser( class'ParticleGenerator'.static.CreateNew( self, TeamLaserEffects[EffectsIndex], Location ) );
			LaserFX.SetOwner(Self);
			LaserFX.AddConnectionEx( StartPt, EndPt, EndActor );
			LaserFX.DamageAmount = 0;
		}
	}
}

//-----------------------------------------------------------------------------

auto state Deployed
{
	function BeginState()
	{
		Super.BeginState();
		GotoState( 'Armed');
	}
}

//-----------------------------------------------------------------------------

state Armed
{
	simulated event Touch(Actor Other)
	{
		if( !LaserFX )
		{
			//Warn("Laser trip mine:" @ Self @ "touched without having a Laser");
			return;
		}

		if( Other == None || Pawn(Other)? )
			Super.Touch( Other );

	}

	//-------------------------------------------------------------------------

	simulated function PostArmed()
	{
		Super.PostArmed();
		CreateTheLaserBeam();
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	// assets
	bBlockKarma=true
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.Items.Laser_TripMine_001'
	TeamLaserEffects(0)=TripLaser'LaserTripMine_FX.TripLaser0'
	TeamLaserEffects(1)=TripLaser'LaserTripMine_FX.TripLaser1'
	DeploySound=Sound'U2XMPA.LaserTripMines.LM_Activate'
	ArmingSound=Sound'U2XMPA.LaserTripMines.LM_Armed'
	TrippedSound=Sound'U2XMPA.LaserTripMines.LM_Tripped'
	AmbientNoiseSound=Sound'U2XMPA.LaserTripMines.LM_Ambient'
	AlternateSkins(0)=Shader'XMPWorldItemsT.Items.LaserTripMine_SD_001_Red'
	AlternateSkins(1)=Shader'XMPWorldItemsT.Items.LaserTripMine_SD_001_Blue'
	RepSkinIndex=0

	// physical
	CollisionHeight=9.000000
	CollisionRadius=9.00000

	bDamageable=true
	Description="Laser Trip Mine"

	// HurtRadius
	DamageAmount=700.0
	DamageRadius=512.0
	DamageType=class'DamageTypeMine'
	Momentum=600000

	ExplodeDelay=0.2
	ArmingDelay=2.0
	EnergyCostPerSec=0.0015
}
