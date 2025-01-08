#include "ParticleSystems.h"

//------------------------------------------------------------------------------
void ATarpBuilder::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(ATarpBuilder::Render);
	NOTE(debugf(TEXT("ATarpBuilder::Render")));

	if( ParticleTemplates.Num()==0 )
		AddTemplate( CastChecked<UTarpBuilderTemplate>(StaticConstructObject( UTarpBuilderTemplate::StaticClass(), this )) );

	Super::Render(Owner,Frame,Lights,RI);

	unguard;
}

//------------------------------------------------------------------------------
UBOOL ATarpBuilder::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(ATarpBuilder::Tick);
	NOTE(debugf(TEXT("ATarpBuilder::Tick")));

	UBOOL Result;

	if( bOn )
	{
		Result = Super::Tick( DeltaSeconds, TickType );

		if( ForcesToAdd.Num() )
		{
			for( INT i=0; i<ForcesToAdd.Num(); i++ )
				AddForce( ForcesToAdd(i) );
			ForcesToAdd.Empty();
		}
	}
	else
	{
		Result = AActor::Tick( DeltaSeconds, TickType );
	}

	return Result;

	unguard;
}

//------------------------------------------------------------------------------
void ATarpBuilder::execBuild( FFrame& Stack, RESULT_DECL )
{
	guard(ATarpBuilder::execBuild);
	NOTE(debugf(TEXT("ATarpBuilder::execBuild")));

	P_FINISH;

	UTarpBuilderTemplate* Template = ParticleTemplates.Num() ? Cast<UTarpBuilderTemplate>(ParticleTemplates(0)) : NULL;

	if( !Template )
	{
		Template = CastChecked<UTarpBuilderTemplate>(StaticConstructObject( UTarpBuilderTemplate::StaticClass(), this ));
		AddTemplate( Template );
	}

	if( Template )
	{
		if( Template->ParticleList.Num() )
			Template->Clean();
		Template->Build();
	}
	
	unguard;
}

//------------------------------------------------------------------------------
void ATarpBuilder::execAttachTo( FFrame& Stack, RESULT_DECL )
{
	guard(ATarpBuilder::execAttachTo);
	NOTE(debugf(TEXT("ATarpBuilder::execAttachTo")));

	P_GET_ACTOR(Other);
	P_GET_INT(ID);
	P_FINISH;

	UTarpBuilderTemplate* Template = ParticleTemplates.Num() ? Cast<UTarpBuilderTemplate>(ParticleTemplates(0)) : NULL;
	if( Template )
		Template->AttachTo( Other, ID );
	
	unguard;
}


IMPLEMENT_CLASS(ATarpBuilder);

