
class RocketProjectileAlt extends Projectile;

var actor PaintedTargets[4];
var int i;

var() float SpreadOffset;

simulated event PreBeginPlay()
{
	Super.PreBeginPlay();
	if(Role==ROLE_Authority)
	{
		Velocity.Z += 0.75 * Speed;
		SetTimer( 0.5, false );
	}
}

function Timer()
{
	local RocketProjectileDrunken RD;
	local RocketProjectileSeeking RS;
	local vector OffsetDir;

	if( PaintedTargets[i]!=None )
	{
		RS = Spawn( class'RocketProjectileSeeking', Owner,, Location, Rotation );
		RS.Index=i;
		RS.SetTarget(PaintedTargets[i]);
	}
	else
	{
		switch(i)
		{
		case 0: OffsetDir.X=1.0; OffsetDir.Y= SpreadOffset; OffsetDir.Z= SpreadOffset; break;
		case 1: OffsetDir.X=1.0; OffsetDir.Y=-SpreadOffset; OffsetDir.Z= SpreadOffset; break;
		case 2: OffsetDir.X=1.0; OffsetDir.Y=-SpreadOffset; OffsetDir.Z=-SpreadOffset; break;
		case 3: OffsetDir.X=1.0; OffsetDir.Y= SpreadOffset; OffsetDir.Z=-SpreadOffset; break;
		}

		RD = Spawn( class'RocketProjectileDrunken', Owner,, Location, rotator(OffsetDir >> Rotation) );
		RD.Index=i;
		RD.SetupData();
		RD.SetCorrectLocation();
	}

	if (++i < ArrayCount(PaintedTargets))
		SetTimer( 0.12, false );
}

defaultproperties
{
	bBounce=true
	Physics=PHYS_Falling
	DrawScale=1.00000
	Speed=250
	MaxSpeed=250
	LifeSpan=15.0
	AmbientGlow=96
	bUnlit=true

	SpreadOffset=0.04

	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'343M.Rocket_Shell'
	bCollideActors=false
}
