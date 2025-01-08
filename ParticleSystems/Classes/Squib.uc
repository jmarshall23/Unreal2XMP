
class Squib extends Effects;

var() int SquibIndex;
var() ParticleGenerator Effect;
var() array<Sound> Sounds;
var() range Pitch;

simulated function PreBeginPlay()
{
	local Actor Squib;
	Super.PreBeginPlay();
	if(bPendingDelete || Level.NetMode == NM_DedicatedServer)
		return;
	if (Level.SquibTemplates[SquibIndex] == None)
		Level.SquibTemplates[SquibIndex] = class'ParticleGenerator'.static.CreateNew( self, Effect, Location );
	Squib = Level.SquibTemplates[SquibIndex];
	Squib.SetLocation(Location);
	Squib.SetRotation(Rotation);
	Squib.Trigger(Self,Instigator);
	if (Sounds.length>0)
		PlaySound( Sounds[Rand(Sounds.length)], SLOT_None, 1.0, false, 100.0, GetRand(Pitch), true );
}

defaultproperties
{
	RemoteRole=ROLE_DumbProxy
	LifeSpan=1.0
	SquibIndex=-1
	Pitch=(A=0.95,B=1.0)
	//bHidden=true
	DrawType=DT_None
}

