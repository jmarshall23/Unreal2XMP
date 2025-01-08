//
//	UseProjector
//

class UseProjector extends Projector;

//
//	PostBeginPlay
//

event PostBeginPlay()
{
	Super(Actor).PostBeginPlay();
	Disable('Tick');
}

//
//	Trigger
//

event Trigger( Actor Other, Pawn EventInstigator )
{
	Enable('Tick');
	SetCollision(True, False, False);
}

//
//	UnTrigger
//

event UnTrigger( Actor Other, Pawn EventInstigator )
{
	Disable('Tick');
	DetachProjector(true);
	SetCollision(False, False, False);
}

//
//	Tick
//

function Tick(float DeltaTime)
{
	local Pawn P;
	local PlayerController PC;
	local vector cameraLoc;
	local rotator cameraRot;
	local actor ViewActor;
	super.Tick(DeltaTime);
	PC = PlayerController(Owner);
	P = PC.Pawn;
	if( bCollideActors )
	{
		if( !P || !P.bUseEnabled )
		{
			SetCollision(False, False, False);
			return;
		}
	}
	else if( P? && P.bUseEnabled )
	{
		SetCollision(True, False, False);
	}
	else return;

	PC.PlayerCalcView(ViewActor, cameraLoc, cameraRot);
	FOV = PC.FOVAngle;
	UseTarget = PC.UseTarget;
	SetLocation(cameraLoc);
	SetRotation(cameraRot);
	CalcMatrix();
}

//
//	Touch
//

simulated event Touch( Actor Other )
{
	if( Other.bUsable && !(Other.IsA('Pawn') && Other.Mesh?) )
		AttachActor(Other);
}

//
//	Untouch
//

simulated event Untouch( Actor Other )
{
	DetachActor(Other);
}

//
//	Default properties
//

defaultproperties
{
	FrameBufferBlendingOp=PB_Add
	MaterialBlendingOp=PB_None
	bProjectBSP=False
	bProjectTerrain=False
	bProjectStaticMesh=False
	bProjectParticles=False
	bProjectActor=False
	bProjectOnAlpha=True
	bProjectOnUnlit=True
	bGradient=False
	bStatic=False
    CullDistance=1024.0
}

