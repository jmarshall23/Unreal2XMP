class WaterVolume extends PhysicsVolume;

var string EntrySoundName, ExitSoundName, EntryActorName;

function PostBeginPlay()
{
	Super.PostBeginPlay();

	if ( EntrySoundName != "" )
		EntrySound = Sound(DynamicLoadObject(EntrySoundName,class'Sound'));
	if ( ExitSoundName != "" )
		ExitSound = Sound(DynamicLoadObject(ExitSoundName,class'Sound'));
	if ( EntryActorName != "" )
		EntryActor = class<Actor>(DynamicLoadObject(EntryActorName,class'Class'));	
}

defaultproperties
{
	EntrySoundName="PlayerSounds.FootstepWater1"
	ExitSoundName="GeneralImpacts.ImpactSplash2"
	bWaterVolume=True
    FluidFriction=+00002.400000
	LocationName="under water"
	bDistanceFog=true
	DistanceFogColor=(R=32,G=64,B=128,A=64)
	DistanceFogStart=+8.0
	DistanceFogEnd=+2000.0
	KExtraLinearDamping=0.8
	KExtraAngularDamping=0.1
}