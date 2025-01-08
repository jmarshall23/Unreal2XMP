class PowerUpPickup extends Pickup
	abstract;

var() int       EnergyUnits;      // A pickup or station's number of EnergyUnits
var() int       HealthUnits;      // A pickup or station's number of HealthUnits
var() int		AmmoUnits;        // Percentage of total ammunition this powerup provides
var() int       TransferRate;     // The rate at which the units are passed to the PowerSuit or Player
							      // in terms of x units per second.
var() string    Description;	  // Popup description of this PowerUp

var() ParticleGenerator ParticleEffectA, ParticleEffectB;
var ParticleGenerator PickupEffectsA, PickupEffectsB;
var() vector PickupEffectOffset;

//-----------------------------------------------------------------------------

simulated event PreBeginPlay()
{
	Super.PreBeginPlay();
	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		if( ParticleEffectA? ) { PickupEffectsA = class'ParticleGenerator'.static.CreateNew( self, ParticleEffectA, Location + PickupEffectOffset ); PickupEffectsA.SetBase( Self ); }
		if( ParticleEffectB? ) { PickupEffectsB = class'ParticleGenerator'.static.CreateNew( self, ParticleEffectB, Location + PickupEffectOffset ); PickupEffectsB.SetBase( Self ); }
	}
}

//-----------------------------------------------------------------------------

simulated event Landed( vector HitNormal )
{
	Super.Landed( HitNormal );
	if( PickupEffectsA? ) PickupEffectsA.Trigger( Self, Instigator );
	if( PickupEffectsB? ) PickupEffectsB.Trigger( Self, Instigator );
}

//-----------------------------------------------------------------------------

simulated event Destroyed()
{
	if( PickupEffectsA? ) PickupEffectsA.Destroy();
	if( PickupEffectsB? ) PickupEffectsB.Destroy();
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

function inventory SpawnCopy( pawn Other )
{
	local inventory Copy;

	if ( Inventory != None )
	{
		Copy = Inventory;
		Inventory = None;
	}
	else
		Copy = spawn(InventoryType,Other,,,rot(0,0,0));

	if( PowerUp(Copy)? )
		PowerUp(Copy).TransferProperties( self );

	Copy.GiveTo( Other );

	return Copy;
}

//-----------------------------------------------------------------------------

function HandlePowerUpEmpty()
{
	Destroy();
}

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.Items.Pickup_TD_001'
	CollisionHeight=4.0
	CollisionRadius=22.0
	PickupEffectOffset=(X=0,Y=0,Z=63)
	bAmbientGlow=false
	AmbientGlow=64
	bHidden=false
	MaxDesireability=1.0 // bot should want this real bad if low on power
	FadeTimeout=90
}