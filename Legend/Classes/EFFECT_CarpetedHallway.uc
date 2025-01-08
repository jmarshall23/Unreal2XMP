//=============================================================================
// EFFECT_CarpetedHallway
//=============================================================================

class EFFECT_CarpetedHallway extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=1.9;
	EnvironmentDiffusion=1.0;
	Room=-1000;
	RoomHF=-4000;
	DecayTime=0.3;
	DecayHFRatio=0.1;
	Reflections=-1831;
	ReflectionsDelay=0.002;
	Reverb=-1630;
	ReverbDelay=0.03;
	RoomRolloffFactor=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=true;
	bReflectionsScale=true;
	bReflectionsDelayScale=true;
	bReverbScale=true;
	bReverbDelayScale=true;
	bDecayHFLimit=true;
}