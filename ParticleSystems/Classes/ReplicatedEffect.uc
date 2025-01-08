class ReplicatedEffect extends Effects
	abstract;

var() ParticleGenerator ParticleEffect;
var() float ParticleLifeSpan;
var() array<Sound> Sounds;
var() Range Pitch;

simulated function PreBeginPlay()
{
	local ParticleGenerator Effect;
	local Sound SelectedSound;

	Super.PreBeginPlay();
		
	if( !bPendingDelete && Level.NetMode != NM_DedicatedServer )
	{
		Effect = class'ParticleGenerator'.static.CreateNew( Self, ParticleEffect, Location );
		Effect.SetLocation( Location );
		Effect.SetRotation( Rotation );
		Effect.Trigger( Self,Instigator );
		Effect.ParticleLifeSpan = ParticleLifeSpan;
		
		if( Sounds.Length > 0 )
		{
			SelectedSound = Sounds[Rand(Sounds.Length)];
			PlaySound( SelectedSound, SLOT_None, 1.0, false, 100.0, GetRand(Pitch), true );
		}
	}
}

defaultproperties
{
	RemoteRole=ROLE_DumbProxy
	LifeSpan=1.0
	ParticleLifeSpan=5.0
	Pitch=(A=0.95,B=1.0)
	//bHidden=true
	DrawType=DT_None
}

