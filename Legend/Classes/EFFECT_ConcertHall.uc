//=============================================================================
// EFFECT_ConcertHall
//=============================================================================

class EFFECT_ConcertHall extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=19.6;
	EnvironmentDiffusion=1.0;
	Room=-1000;
	RoomHF=-500;
	DecayTime=3.92;
	DecayHFRatio=0.7;
	Reflections=-1231;
	ReflectionsDelay=0.02;
	Reverb=-2;
	ReverbDelay=0.029;
	RoomRolloffFactor=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=true;
	bReflectionsScale=true;
	bReflectionsDelayScale=true;
	bReverbScale=true;
	bReverbDelayScale=true;
	bDecayHFLimit=true;
}