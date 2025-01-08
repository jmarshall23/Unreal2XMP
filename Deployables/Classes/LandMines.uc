
class LandMines extends MineBase
	placeable;

//-----------------------------------------------------------------------------

simulated event PostNetBeginPlay()
{
	Super.PostNetBeginPlay();

	// put armed mines that are becoming relevant into the correct state
	if( Physics != PHYS_Falling )
	{
		//DMTNS("creating armed mine");
		GotoState( 'Armed' );
	}
}

//-----------------------------------------------------------------------------

auto state Deployed
{
	function Landed(vector HitNormal)
	{
		Super.Landed( HitNormal );
		//DMTNS("Landed -- arming");
		GotoState( 'Armed' );
	}

	event BeginState()
	{
		//DMTNS("Deployed");
		Super.BeginState();
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	// assets
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.Items.Mine_SD_001'
	DeploySound=Sound'U2XMPA.LandMines.M_Activate'
	ArmingSound=Sound'U2XMPA.LandMine.M_Armed'
	TrippedSound=Sound'U2XMPA.LandMine.M_Tripped'
	AlternateSkins(0)=Shader'XMPWorldItemsT.Items.Mine_SD_001_Red'
	AlternateSkins(1)=Shader'XMPWorldItemsT.Items.Mine_SD_001_Blue'
	RepSkinIndex=0

	// physical
	CollisionHeight=10.000000
	CollisionRadius=35.00000
	Description="Land Mine"

	bDamageable=false			// turn on damage when mine has been armed
	Physics=PHYS_Falling // so always get Landed notification even for items placed on ground via editor

	// HurtRadius
	DamageAmount=800.0
	DamageRadius=500.0
	DamageType=class'DamageTypeMine'
	Momentum=400000

	ExplodeDelay=0.5
	ArmingDelay=2.0
	EnergyCostPerSec=0.0012
}