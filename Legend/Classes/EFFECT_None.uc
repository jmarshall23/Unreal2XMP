//=============================================================================
// EFFECT_None: Plain room effect.
//=============================================================================

class EFFECT_None extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	Room=-10000;
	RoomHF=-10000;
	RoomRolloffFactor=0.0;
	DecayTime=0.1;
	DecayHFRatio=0.83;
	Reflections=-10000;
	ReflectionsDelay=0.0;
	Reverb=-10000;
	ReverbDelay=0.0;
	EnvironmentSize=1;
	EnvironmentDiffusion=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=false;
	bReflectionsScale=false;
	bReflectionsDelayScale=false;
	bReverbScale=false;
	bReverbDelayScale=false;
	bDecayHFLimit=false;	
}