//=============================================================================
// TimedTrigger: causes an event after X seconds.
//=============================================================================
class TimedTrigger extends Trigger;

var() float DelaySeconds;
var() bool bRepeating;

function Timer()
{
	TriggerEvent(Event,self,None);

	if ( !bRepeating )
		Destroy();
}

function MatchStarting()
{
	SetTimer(DelaySeconds, bRepeating);
}

defaultproperties
{
	bObsolete=true
	bCollideActors=false
	DelaySeconds=1.0
}