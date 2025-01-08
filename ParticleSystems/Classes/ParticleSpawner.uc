//=============================================================================
// $Author: Mfox $
// $Date: 4/30/02 12:27p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleSpawner.uc
// Author:	Aaron R Leiby
// Date:	7 May 2003
//------------------------------------------------------------------------------
// Description:	Used to spawn particle effects on clients (from server).
//------------------------------------------------------------------------------
// How to use this class:
// + Spawn on server.
// + Call SetEffect.
//------------------------------------------------------------------------------

class ParticleSpawner extends Effects;

var ParticleGenerator Image;

replication
{
	reliable if( bNetInitial && bNetDirty && (Role==ROLE_Authority) )
		Image;
}

//------------------------------------------------------------------------------
function SetEffect( ParticleGenerator P )
{
	Image=P;
	if( Level.NetMode != NM_DedicatedServer )
		SpawnEffect();
}

//------------------------------------------------------------------------------
simulated function PostNetBeginPlay()
{
	if( Role < ROLE_Authority )
		SpawnEffect();
}

//------------------------------------------------------------------------------
simulated function SpawnEffect()
{
	local ParticleGenerator Effect;

	DMN("SpawnEffect: "@Image);

	if( Image == None )
		return;

	Effect = class'ParticleGenerator'.static.CreateNew( Self, Image, Location );
	Effect.SetRotation( Rotation );
	Effect.Trigger( Self, Instigator );
	Effect.ParticleLifeSpan = Effect.GetMaxLifeSpan() + Effect.TimerDuration + Effect.PrimeTime;
	Effect.SetBase( Base );
}

defaultproperties
{
	bHidden=true
	Lifespan=1.0
	RemoteRole=ROLE_SimulatedProxy
}

