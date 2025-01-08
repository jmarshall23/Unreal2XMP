//=============================================================================
// EFFECT_PIPE_RESONANT
//============================================================================

class EFFECT_PIPE_RESONANT extends I3DL2Listener
	editinlinenew;

defaultproperties
{
	EnvironmentSize=1.3f;
	EnvironmentDiffusion=0.910f;
	Room=-1200;
	RoomHF=-700;
	RoomLF=-1100;
	DecayTime=6.81f;
	DecayHFRatio=0.18f;
	DecayLFRatio=0.10f;
	Reflections=-300;
	ReflectionsDelay=0.010f;
	Reverb=-700;
	ReverbDelay=0.022f;
	EchoTime=0.250f;
	EchoDepth=0.000f;
	ModulationTime=0.250f;
	ModulationDepth=0.000f;
	AirAbsorptionHF=-5.0f;
	HFReference=2854.4f;
	LFReference=20.0f;
	RoomRolloffFactor=0.00f;
	//Flags=0x0;
}
