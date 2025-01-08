//=============================================================================
// Projectile.
//
// A delayed-hit projectile that moves around for some time after it is created.
//=============================================================================
class Projectile extends Actor
	abstract
	native;

//-----------------------------------------------------------------------------
// Projectile variables.

// Motion information.
var		float	Speed;				// Initial speed of projectile.
var		float	MaxSpeed;			// Limit on speed of projectile (0 means no limit)
var		float	TossZ;
var		float	BounceDamping;
var		float	StopNormalZ;
var		Actor	ZeroCollider;
var		bool	bSwitchToZeroCollision;	// if collisionextent nonzero, and hit actor with bBlockNonZeroExtents=0, switch to zero extent collision

// Damage attributes.
var		bool	bNoFalloff;			// Falloff parm for HurtRadius.
var		float	Damage; 
var		float	DamageRadius;        
var		float	MomentumTransfer;	// Momentum magnitude imparted by impacting projectile.
var		class<DamageType> MyDamageType;

// Projectile sound effects
var		sound	SpawnSound;			// Sound made when projectile is spawned.
var		sound	ImpactSound;		// Sound made when projectile hits something.
var		sound	BounceSound;		// Sound made when projectile bounces.
var		float	BounceSoundRadius;	// Radius used when playing BounceSound.

// explosion effects
var		class<Projector> ExplosionDecal;
var		float	ExploWallOut;		// distance to move explosions out from wall

simulated function PreBeginPlay()
{
	Super.PreBeginPlay();
	Velocity = vector(Rotation) * Speed;
}

//==============
// Encroachment
function bool EncroachingOn( actor Other )
{
	// Account for blocking volumes.
	if ( Volume(Other) != None )
		return false;

	if ( (Other.Brush != None) || (Brush(Other) != None) )
		return true;
		
	return false;
}

//==============
// Touching
simulated singular function Touch(Actor Other)
{
	local actor HitActor;
	local vector HitLocation, HitNormal, VelDir;
	local bool bBeyondOther;
	local float BackDist, DirZ;

	if ( Other == None ) // Other just got destroyed in its touch?
		return;
	if ( Other.bProjTarget || (Other.bBlockActors && Other.bBlockPlayers) )
	{
		if ( Velocity == vect(0,0,0) || Other.IsA('Mover') ) 
		{
			ProcessTouch(Other,Location);
			return;
		}
		
		//get exact hitlocation - trace back along velocity vector
		bBeyondOther = ( (Velocity Dot (Location - Other.Location)) > 0 );
		VelDir = Normal(Velocity);
		if( VelDir.Z >= 0 )
			DirZ = sqrt(VelDir.Z);
		else
			DirZ = -sqrt(-VelDir.Z);
		BackDist = Other.CollisionRadius * (1 - DirZ) + Other.CollisionHeight * DirZ;
		if ( bBeyondOther )
			BackDist += VSize(Location - Other.Location);
		else
			BackDist -= VSize(Location - Other.Location);

	 	HitActor = Trace(HitLocation, HitNormal, Location, Location - 1.1 * BackDist * VelDir, true);
		if (HitActor == Other)
			ProcessTouch(Other, HitLocation); 
		else if ( bBeyondOther )
			ProcessTouch(Other, Other.Location - Other.CollisionRadius * VelDir);
		else
			ProcessTouch(Other, Location);
	}
}

simulated function ProcessTouch(Actor Other, Vector HitLocation)
{
//	if ( Other != Instigator )
//		Explode(HitLocation,Normal(HitLocation-Other.Location),Other);

	// Generate a better hitnormal
	local vector HitNormal;
	if ( Other != Instigator )
	{
//		HitNormal = Normal( HitLocation - Other.Location );
		HitNormal = -Velocity;
		if( VSize2D(HitLocation - Other.Location)<(Other.CollisionRadius-1) )	// if we hit the top/bottom
		{
			HitNormal.X = 0;
			HitNormal.Y = 0;
		}
		else																	// otherwise we hit the side
		{
			HitNormal.Z = 0;
		}
		HitNormal = Normal( HitNormal );

		if( bBounce && Pawn(Other)==None )
		{
			HitWall( HitNormal, Other );
		}
		else
		{
			Explode( HitLocation, HitNormal, Other );
		}
	}
}

simulated function Landed (vector HitNormal)
{
	if( !bBounce )
		HitWall(HitNormal, None);
}

simulated function HitWall (vector HitNormal, actor Wall)
{
	local float BounceSpeed;

	if( bBounce )
	{
		BounceSpeed = VSize(Velocity);
		PlaySound( BounceSound, SLOT_Misc, FMin( 800.0, 800.0 * BounceSpeed/256.0), false, BounceSoundRadius );

		Velocity = BounceDamping * ( ( Velocity dot HitNormal ) * HitNormal * (-2.0) + Velocity );   // Reflect off Wall w/damping
		RandSpin( 100000 );
		
		if( BounceSpeed < 16 && HitNormal.Z > StopNormalZ )
		{
			SetPhysics( PHYS_None );
			Velocity = Vect(0,0,0);
			//why are we passing to Landed, if Landed is explicitly designed to do nothing?
			Landed( HitNormal );
		}
	}
	else
	{
		if ( Role == ROLE_Authority )
		{
			//NEW (mdf) allow for dynamic blocking volumes which want to handle damage
			if ( Mover(Wall) != None || BlockingVolume(Wall) != None )
			/*OLD
			if ( Mover(Wall) != None )
			*/
				Wall.TakeDamage( Damage, instigator, Location, MomentumTransfer * Normal(Velocity), MyDamageType);

			MakeNoise(1.0);
		}
		Explode(Location + ExploWallOut * HitNormal, HitNormal, Wall);
		if ( (ExplosionDecal != None) && (Level.NetMode != NM_DedicatedServer) )
			Spawn(ExplosionDecal,self,,Location, rotator(-HitNormal));
	}
}

simulated function BlowUp(vector HitLocation)
{
	HurtRadius(Damage,DamageRadius, MyDamageType, MomentumTransfer, HitLocation );
	if ( Role == ROLE_Authority )
		MakeNoise(1.0);
}

simulated function Explode(vector HitLocation, vector HitNormal, actor HitActor)
{
	if( Role == ROLE_Authority )
	{
		MakeNoise( 1.0 );

		if( Damage > 0 && DamageRadius > 0 )
		{
			HurtRadius( Damage, DamageRadius, MyDamageType, MomentumTransfer, HitLocation, bNoFalloff );
		}
		else if ( HitActor != None )
		{
			HitActor.TakeDamage( Damage, Instigator, HitLocation, MomentumTransfer * Normal(Velocity), MyDamageType );
		}
	}

	Destroy();
}

simulated final function RandSpin(float spinRate)
{
	DesiredRotation = RotRand();
	RotationRate.Yaw = spinRate * 2 *FRand() - spinRate;
	RotationRate.Pitch = spinRate * 2 *FRand() - spinRate;
	RotationRate.Roll = spinRate * 2 *FRand() - spinRate;	
}

static function vector GetTossVelocity(Pawn P, Rotator R)
{
	local vector V;

	V = Vector(R);
	V *= ((P.Velocity Dot V)*0.4 + Default.Speed);
	V.Z += Default.TossZ;
	return V;
}

defaultproperties
{
	 bAcceptsProjectors=false
	 bUseCylinderCollision=true
     MaxSpeed=+02000.000000
     DrawType=DT_Mesh
     Texture=S_Camera
     SoundVolume=0
     CollisionRadius=+00000.000000
     CollisionHeight=+00000.000000
     bCollideActors=True
     bCollideWorld=True
	 bNetTemporary=true
	 bGameRelevant=true
	 bReplicateInstigator=true
     Physics=PHYS_Projectile
     LifeSpan=+0014.000000
     NetPriority=+00002.500000
	 MyDamageType=class'DamageType'
	 RemoteRole=ROLE_SimulatedProxy
	 bUnlit=true
	 TossZ=+100.0
     bNetInitialRotation=true
	 bDisturbFluidSurface=true
	 BounceSoundRadius=100.0
	 BounceDamping=0.4
	 StopNormalZ=0.5
}
