//NEW(arl): SubActionParticleTrigger
//=============================================================================
// SubActionParticleTrigger:
//
// Triggers a particle attached to the SceneManager's AffectedActor.
//=============================================================================
class SubActionParticleTrigger extends MatSubAction
	native;

var(ParticleTrigger) string Name;	// Name of the attachment to trigger.

defaultproperties
{
	Icon=SubActionParticleTrigger
	Desc="ParticleTrigger"
}
