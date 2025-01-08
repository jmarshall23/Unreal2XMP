//=============================================================================
// EFFECT_Hallway
//=============================================================================

class EFFECT_Hallway extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=1.8;
	EnvironmentDiffusion=1.00;
	Room=-1000;
	RoomHF=-300;
	DecayTime=1.49;
	DecayHFRatio=0.59;
	Reflections=-1219;
	ReflectionsDelay=0.007;
	Reverb=441;
	ReverbDelay=0.011;
	RoomRolloffFactor=0.0;
	AirAbsorptionHF=-5;
	bDecayTimeScale=true;
	bReflectionsScale=true;
	bReflectionsDelayScale=true;
	bReverbScale=true;
	bReverbDelayScale=true;
	bDecayHFLimit=true;
}