//=============================================================================
// EFFECT_Quarry
//=============================================================================

class EFFECT_Quarry extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=17.5;
	EnvironmentDiffusion=1.0;
	Room=-1000;
	RoomHF=-1000;
	DecayTime=1.49;
	DecayHFRatio=0.83;
	Reflections=-1000;
	ReflectionsDelay=0.061;
	Reverb=500;
	ReverbDelay=0.025;
	RoomRolloffFactor=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=true;
	bReflectionsScale=true;
	bReflectionsDelayScale=true;
	bReverbScale=true;
	bReverbDelayScale=true;
	bDecayHFLimit=true;
}