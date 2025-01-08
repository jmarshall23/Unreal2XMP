#include "ParticleSystems.h"

//------------------------------------------------------------------------------
UBOOL AParticleRef::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AParticleRef::Tick);
	NOTE(debugf(TEXT("AParticleRef::Tick")));

	// Sync location and rotation -- in case our ParticleSystem is moved.
	if( GIsEditor && Ref )
	{
		SetLocation( Ref->Location );
		SetRotation( Ref->Rotation );
	}

	if( bReset )
	{
		bReset=false;
		GetLevel()->DestroyActor( Ref );
		Ref=NULL;
	}

	if( !Ref )
		FORCE_EVENT(eventSpawnRef());

	return Super::Tick( DeltaSeconds, TickType );

	unguard;
}

//------------------------------------------------------------------------------
void AParticleRef::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(AParticleRef::Render);
	NOTE(debugf(TEXT("AParticleRef::Render")));

	// Draw icon in editor.
	if( !Ref || (GIsEditor && !(Frame->Viewport->Actor->ShowFlags & SHOW_PlayerCtrl)) )
	{
		INT PrevDT = DrawType;
		DrawType = DT_Sprite;
		Owner->Render(Frame,Lights,NULL,RI);
		DrawType = PrevDT;
	}

	unguard;
}

//------------------------------------------------------------------------------
void AParticleRef::Destroy()
{
	guard(AParticleRef::Destroy);
	NOTE(debugf(TEXT("AParticleRef::Destroy")));

	if( GIsEditor && Ref )
	{
		GetLevel()->DestroyActor( Ref );
	}

	Super::Destroy();

	unguard;
}


IMPLEMENT_CLASS(AParticleRef);