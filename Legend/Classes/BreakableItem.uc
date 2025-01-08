//=============================================================================
// BreakableItem.
//=============================================================================
class BreakableItem extends Decoration
	native;

var() mesh BrokenMesh;
var() StaticMesh BrokenStaticMesh;
var() sound BreakSound;
var() ParticleGenerator BreakParticleEffectsRefs[6];
var() bool bDestroyCollision; 					// Turns off collision after light is broken

// HurtRadius
var() float DamageAmount;
var() float DamageRadius;
var() class<DamageType> DamageType;
var() float Momentum;

// ShakeView
var() bool bShakeView;							// set this to enable the following.
var(ShakeView) float	Duration;				// how long quake should last
var(ShakeView) float	ShakeMagnitude;			// How much to shake the camera.
var(ShakeView) float	BaseTossMagnitude;		// toss Magnitudenitude at epicenter, 0 at radius
var(ShakeView) float	Radius;					// radius of quake
var(ShakeView) bool		bTossNPCs;				// if true, NPCs in area are tossed around
var(ShakeView) bool		bTossPCs;				// if true, PCs in area are tossed around
var(ShakeView) float	MaxTossedMass;			// if bTossPawns true, only pawns <= this mass are tossed
var(ShakeView) float	TimeBetweenShakes;		// time between each shake
var(ShakeView) string	AmbientSoundStr;		// sound for duration of quake
var(ShakeView) string	ShakeSoundStr;			// sound for each shake

var int InitialHealth;

event PreBeginPlay()
{
	Super.PreBeginPlay();
	InitialHealth = Health;
}

interface function bool   HasUseBar( Controller User )		{ return true; }
interface function float  GetUsePercent( Controller User )	{ return float(Health) / float(InitialHealth); }
interface function string GetDescription( Controller User ) { return Description @ "(" @ Health @ ")"; }
interface function int    GetUseIconIndex( Controller User ){ return 0; }

function BreakApart()
{
// breakable handling -- override in subclasses.
	local int i;

	if( BrokenMesh!=None )
		Mesh = BrokenMesh;

	if( BrokenStaticMesh!=None )
		SetStaticMesh( BrokenStaticMesh );

	for( i=0; i<ArrayCount(BreakParticleEffectsRefs); i++ )
		if( BreakParticleEffectsRefs[i]!=None )
			ParticleExplosionRef( BreakParticleEffectsRefs[i] );

	if( BreakSound!=None )
		PlaySound( BreakSound, SLOT_None );

	if( DamageAmount>0 )
		HurtRadius( DamageAmount, DamageRadius, DamageType, Momentum, Location );

	if( bShakeView )
		ShakeView();

	Scaleglow = 1.0;
	Ambientglow = 0;
	SoundVolume = 0;

	if( bDestroyCollision )
		SetCollision( false, false, false );

	if( !BrokenMesh && !BrokenStaticMesh )
		Destroy();
}


function TakeDamage( int Damage, Pawn InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	if( bStatic )
		return;

	Instigator = InstigatedBy;
	if( Instigator != None )
	{
		MakeNoise( 1.0 );
	}
	if( Health > 0 )
	{
		Health -= Damage;
		if( Health <= 0 )
		{
			Level.Game.NotifyBreakableDestroyed( InstigatedBy.Controller, Self );
			BreakApart();
		}
	}
}


function ParticleExplosionRef( ParticleGenerator Effect )
{
	local ParticleSpawner Spawner;
	Spawner = Spawn( class'ParticleSpawner',,, Location, Rotation );
	Spawner.SetEffect(Effect);
}


function ShakeView()
{
	local EarthquakeTrigger T;

	T = Spawn( class'EarthquakeTrigger',,, Location );
	T.Duration				= Duration;
	T.ShakeMagnitude		= ShakeMagnitude;
	T.BaseTossMagnitude		= BaseTossMagnitude;
	T.Radius				= Radius;
	T.bTossNPCs				= bTossNPCs;
	T.bTossPCs				= bTossPCs;
	T.MaxTossedMass			= MaxTossedMass;
	T.TimeBetweenShakes		= TimeBetweenShakes;
	T.AmbientSoundStr		= AmbientSoundStr;
	T.ShakeSoundStr			= ShakeSoundStr;
	T.Trigger( self, Instigator );
}


defaultproperties
{
	Health=20

	bStatic=false
	bDamageable=true
	bProjTarget=true
	bDestroyCollision=true

	//ShakeView
	Duration=10.000000
	ShakeMagnitude=20.000000
	BaseTossMagnitude=2500000.000000
	Radius=2048.000000
	bTossNPCs=True
	bTossPCs=True
	MaxTossedMass=500.000000
	TimeBetweenShakes=0.500000
}

