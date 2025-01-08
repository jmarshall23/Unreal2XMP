//=============================================================================
// EFFECT_Parkinglot
//=============================================================================

class EFFECT_Parkinglot extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=8.3;
	EnvironmentDiffusion=1.0;
	Room=-1000;
	RoomHF=0;
	DecayTime=1.650;
	DecayHFRatio=1.5;
	Reflections=-1364;
	ReflectionsDelay=0.008;
	Reverb=-1153;
	ReverbDelay=0.012;
	RoomRolloffFactor=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=true;
	bReflectionsScale=true;
	bReflectionsDelayScale=true;
	bReverbScale=true;
	bReverbDelayScale=true;
	bDecayHFLimit=true;
}