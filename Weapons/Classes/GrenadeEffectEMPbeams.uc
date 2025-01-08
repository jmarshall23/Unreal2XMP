
class GrenadeEffectEMPbeams extends ElectricGenerator;

defaultproperties
{
	bStatic=false
	bNoDelete=false
	bMovable=true
	RemoteRole=ROLE_SimulatedProxy
	LifeSpan=3.0
	bGameRelevant=true
	CollisionRadius=0.000000
	CollisionHeight=0.000000
	bStartAtOrigin=True
	bResetArcOnReuse=True
	LatchRate=(A=0.010000,B=0.050000)
	ArcLifeSpan=(A=0.010000,B=0.050000)
	MinArcLength=64.0
	AffectedTypes(0)=Light
	AffectedTypes(1)=Pawn
	SparkType="GL_EMP.ParticleSalamander1"
	bOn=False
	bInitiallyOn=False
	TimerDuration=1.200000
	BeamTexture=Texture'ParticleSystems.Pulse.GlowBeam'
	BeamSegLength=(A=8.000000,B=16.000000)
	BeamColor=(B=255,G=255,R=255)
	BeamWidth=(A=3.000000,B=6.000000)
	BeamTextureWidth=(A=12.000000,B=12.000000)
	NumBeams=(A=2.000000,B=1.000000)
	SpriteJointTexture=Texture'ParticleSystems.Pulse.GlowFuzz'
	SpriteJointSize=(A=6.000000,B=8.000000)
	SpriteJointColor=(B=128,G=128,R=128)
	MetaBeamSegLength=(A=40.000000,B=60.000000)
	MetaBeamWidth=(A=20.000000,B=35.000000)
	MetaBeamRefreshTime=(A=0.075000,B=0.025000)
	bHidden=True
	InitialState=TriggerTimed
}

