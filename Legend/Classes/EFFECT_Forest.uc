//=============================================================================
// EFFECT_Forest
//=============================================================================

class EFFECT_Forest extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=38.0;
	EnvironmentDiffusion=0.30;
	Room=-1000;
	RoomHF=-3300;
	DecayTime=1.49;
	DecayHFRatio=0.54;
	Reflections=-2560;
	ReflectionsDelay=0.162;
	Reverb=-229;
	ReverbDelay=0.088;
	RoomRolloffFactor=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=true;
	bReflectionsScale=true;
	bReflectionsDelayScale=true;
	bReverbScale=true;
	bReverbDelayScale=true;
	bDecayHFLimit=true;
}