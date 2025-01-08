#include "ParticleSystems.h"

////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void UForce::AddTemplate( UParticleTemplate* T )
{
	guard(UForce::AddTemplate);
	NOTE(debugf(TEXT("(%s)UForce::AddTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None") ));
	AffectedTemplates.AddItem( T );
	T->NotifyForceAttachment( this );
	bIterateAll = false;
	unguard;
}

//------------------------------------------------------------------------------
void UForce::RemoveTemplate( UParticleTemplate* T, UBOOL bNoRemove )
{
	guard(UForce::RemoveTemplate);
	NOTE(debugf(TEXT("(%s)UForce::RemoveTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None") ));
	if( !bNoRemove ) AffectedTemplates.RemoveItem( T );
	T->NotifyForceDetachment( this );
	//bIterateAll = (AffectedTemplates.Num() == 0);	// This causes badness when things are getting all cleaned up (Force becomes global before being deleted).
	unguard;
}

//------------------------------------------------------------------------------
void UForce::AddExternalTemplate( UParticleTemplate* T )
{
	guard(UForce::AddExternalTemplate);
	NOTE(debugf(TEXT("(%s)UForce::AddExternalTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None") ));
	AddTemplate(T);
	NotifyExternallyAttached(T);
	unguard;
}

//------------------------------------------------------------------------------
void UForce::RemoveExternalTemplate( UParticleTemplate* T, UBOOL bNoRemove )
{
	guard(UForce::RemoveExternalTemplate);
	NOTE(debugf(TEXT("(%s)UForce::RemoveExternalTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None") ));
	RemoveTemplate(T,bNoRemove);
	NotifyExternallyDetached(T);
	unguard;
}

//------------------------------------------------------------------------------
void UForce::NotifyDetachedFromSystem( AParticleGenerator* System )
{
	guard(UForce::NotifyDetachedFromSystem);
	NOTE(debugf(TEXT("(%s)UForce::NotifyDetachedFromSystem( %s )"), GetFullName(), System ? System->GetFullName() : TEXT("None") ));

	for( INT i=0; i<AffectedTemplates.Num(); i++ )
		if( AffectedTemplates(i) )
			RemoveTemplate( AffectedTemplates(i), true );
	AffectedTemplates.Empty();

	unguard;
}

/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
UForce* UForce::Duplicate( UObject* NewOuter )
{
	guard(UForce::Duplicate);
	NOTE(debugf(TEXT("(%s)UForce::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	// Make sure we haven't already been duplicated.
	if( AParticleGenerator::GParticleDuplicates )
	{
		UForce* Duplicate = Cast<UForce>(AParticleGenerator::GParticleDuplicates->FindRef( this ));
		if( Duplicate )
			return Duplicate;
	}

	UForce* NewForce = CastChecked<UForce>( StaticConstructObject
	(
		GetClass(),
		NewOuter,
		NAME_None,
		GetFlags(), /* 0? */
		this
	));

	if( !AParticleGenerator::GParticleDuplicates )
	{
		AParticleGenerator::GParticleDuplicates = new TMap< UObject*, UObject* >;
		check(AParticleGenerator::GParticleDuplicates!=NULL);
	}
	AParticleGenerator::GParticleDuplicates->Set( this, NewForce );
	AParticleGenerator::GParticleDuplicates->Set( NewForce, NewForce );

	if( NewForce )
	{
		// Delete existing inner objects first.	
		{TArray<UParticleTemplate*> Remove=NewForce->AffectedTemplates;
		for( INT i=0; i<Remove.Num(); i++ )
			if( Remove(i)->IsIn( NewForce ) )
				Remove(i)->DeleteUObject();}

		// Duplicate AffectedTemplates.
		NewForce->AffectedTemplates.Empty();
		for( INT i=0; i<AffectedTemplates.Num(); i++ )
			if( AffectedTemplates(i) )
				NewForce->AffectedTemplates.AddItem( AffectedTemplates(i)->Duplicate( NewOuter ) );
	}

	return NewForce;

	unguard;
}

//////////////////////
// AActor Overrides //
//////////////////////

//------------------------------------------------------------------------------
void UForce::Destroy()
{
	guard(UForce::Destroy);
	NOTE(debugf(TEXT("(%s)UForce::Destroy"), GetFullName() ));

	if( !PurgingGarbage() )
	{
		AParticleGenerator* System = Cast<AParticleGenerator>(GetOuter());
		if( System )
			System->RemoveForce( this );

		for( INT i=0; i<AffectedTemplates.Num(); i++ )
			if( AffectedTemplates(i) )
				RemoveTemplate( AffectedTemplates(i), true );
		AffectedTemplates.Empty();
	}

	Super::Destroy();

	unguard;
}

/////////////////////////////
// UnrealScript Interfaces //
/////////////////////////////

//------------------------------------------------------------------------------
void UForce::execAddTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UForce::execAddTemplate);
	NOTE(debugf(TEXT("UForce::execAddTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AddTemplate( T );
	
	unguard;
}

//------------------------------------------------------------------------------
void UForce::execRemoveTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UForce::execRemoveTemplate);
	NOTE(debugf(TEXT("UForce::execRemoveTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	RemoveTemplate( T );
	
	unguard;
}

//------------------------------------------------------------------------------
void UForce::execAddExternalTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UForce::execAddExternalTemplate);
	NOTE(debugf(TEXT("UForce::execAddExternalTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AddExternalTemplate( T );
	
	unguard;
}

//------------------------------------------------------------------------------
void UForce::execRemoveExternalTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(UForce::execRemoveExternalTemplate);
	NOTE(debugf(TEXT("UForce::execRemoveExternalTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	RemoveExternalTemplate( T );
	
	unguard;
}


IMPLEMENT_CLASS(UForce);

