class ParticleLoader extends Actor placeable;

var() string Effects[32];
var() float TimeDelay;

simulated event PreBeginPlay()
{
	SetTimer(TimeDelay,false);
    Super.PreBeginPlay();
}

simulated event Timer()
{
    local int i;
    local ParticleGenerator P;

    for( i=0; i<ArrayCount(Effects); i++ )
    {
		if( Effects[i]!="" )
		{
			P = class'ParticleGenerator'.static.DynamicCreateNew( self, class'ParticleSalamander', Effects[i], Location );
			P.Trigger(Self,Instigator);
			P.ParticleLifeSpan = P.GetMaxLifeSpan() + P.TimerDuration;
		}
    }

    Destroy();
}

defaultproperties
{
	TimeDelay=0.5
//	Effects(0)="CAR_Muzzleflash.ParticleSalamander2"
//	Effects(1)="CAR_Muzzleflash.ParticleSalamander3"
//	Effects(2)="PistolEffects.ParticleSalamander1"
//	Effects(3)="PistolEffects.ParticleSalamander16"
//	Effects(4)="RocketEffects.ParticleSalamander0"
//	Effects(5)="RocketEffects.ParticleSalamander1"
//	Effects(6)="RocketEffects.ParticleSalamander2"
//	Effects(7)="GL_EMP.ParticleSalamander0"
//	Effects(8)="GL_EMP.ParticleSalamander1"
//	Effects(9)="GL_Toxic.ParticleSalamander0"
//	Effects(10)="Flamethrower_Effects.ParticleSalamander1"
//	Effects(11)="ImpactFX.ParticleSalamander0"
//	Effects(12)="ImpactFX.ParticleSalamander1"
//	Effects(13)="ImpactFX.ParticleSalamander3"
//	Effects(14)="Impact_Metal_AR.ParticleSalamander0"
//	Effects(15)="Impact_Metal_P.ParticleSalamander0"
//	Effects(16)="Impact_Metal_SR.ParticleSalamander0"
//	Effects(17)="Blood.ParticleSalamander2"
}