class VehicleExplosion extends Actor
	notplaceable;
var ParticleGenerator	VehicleExplosionType;
var float TimeToDie;

replication{

	reliable if (Role==ROLE_Authority)
		VehicleExplosionType;

}

simulated function PostNetBeginPlay()
{
	local ParticleGenerator Explosion;
	
	super.PostNetBeginPlay();
	TimeToDie = Level.TimeSeconds +1;
	if (level.netmode != NM_DedicatedServer)
	{
		if(VehicleExplosionType?)
		{
			Explosion = class'ParticleGenerator'.static.CreateNew( self, VehicleExplosionType, Location-Velocity );
			Explosion.SetRotation( Rotation );
			Explosion.Trigger(self, Instigator);
		}
		else
		{
			VehicleExplosionType = XMPVehicle(Owner).VehicleExplosionType;
			Explosion = class'ParticleGenerator'.static.CreateNew( self, VehicleExplosionType, Location-Velocity );
			Explosion.SetRotation( Rotation );
			Explosion.Trigger(self, Instigator);
		}
	}
} 

Simulated function Tick(float DeltaTime)
{
	super.tick(DeltaTime);
	if (Level.TimeSeconds > TimeToDie)
		destroy();
}

defaultproperties
{
	bCollideWorld=false
	bBlockActors=false
	bCollideActors=true	
	bBlockPlayers=false
	bBlockZeroExtentTraces=true
	DrawType=DT_None
	RemoteRole=ROLE_SimulatedProxy

}