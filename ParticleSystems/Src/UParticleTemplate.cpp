#include "ParticleSystems.h"

FVector									UParticleTemplate::Pts[MAX_PARTICLE_VERTICES*3];
QuadPointInfo							UParticleTemplate::QuadPts[MAX_PARTICLE_VERTICES];

inline UBOOL UParticleTemplate::NeedsHardwareLighting()
{
	return bDynamicLit || Lights.Num();
}

void UParticleTemplate::SetupLighting( FRenderInterface* RI, TList<FDynamicLight*>* DynLights )
{
	// Fix ARL: remove RaytracedLights because they are no longer used
	guard(UParticleTemplate::SetupLighting);

	UBOOL bHardwareLighting = NeedsHardwareLighting();
//	bHardwareLighting = 1;
	RI->EnableLighting( bHardwareLighting );

	HardwareLightsUsed = 0;
	if( !bHardwareLighting ) return;

	for( INT i=0; i<Lights.Num(); i++ )
		if( Lights(i) )
		{
			FDynamicLight DLight( Lights(i) );
//debugf(L"Set  Light %i to %s",HardwareLightsUsed, Lights(i)->GetFullName() );
			RI->SetLight( HardwareLightsUsed++, &DLight );
		}
	if( bDynamicLit )
		for(TList<FDynamicLight*>* LightList = DynLights;LightList;LightList = LightList->Next)
{
//debugf(L"Set DLight %i to %s",HardwareLightsUsed, LightList->Element->Actor->GetFullName() );
			RI->SetLight( HardwareLightsUsed++,LightList->Element );
}
	unguard;
}

void UParticleTemplate::TeardownLighting( FRenderInterface* RI )
{
	guard(UParticleTemplate::TeardownLighting);
	for( INT i=0; i<HardwareLightsUsed; i++ )
{
//debugf(L"Set NLight %i to NULL",i );
		RI->SetLight( i, NULL );
}
	unguard;
}

// NOTE[aleiby]: If we store multiple particle lists in a map indexed by
// their associated System, then we could share ParticleTemplates among
// multiple ParticleGenerators.

//------------------------------------------------------------------------------
UParticle* UParticleTemplate::GetParticle()
{
	guard(UParticleTemplate::GetParticle);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::GetParticle"), GetFullName() ));

	// Create particle.
	if(!ParticleClass)
		ParticleClass = StringFindParticleClass(*ParticleType);
	UParticle* Particle = ParticleClass->CreateInstance();

	// Add to list.
	if( bReverseDrawOrder || (bRandomDrawOrder && appFrand()<0.5f) )
	{
		ParticleList.AddItem(Particle);
	}
	else
	{
		ParticleList.Insert(0);
		ParticleList(0) = Particle;
	}

	// Init vars.
	InitParticle( Particle );
	AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());
	for( INT i = 0; i < System->Forces.Num(); i++ )
	{
		UForce* Force = System->Forces(i);
		if( Force && Force->bIterateAll )
		{
			NOTE(debugf(TEXT("%s (Initing particle with global force) %s"), GetFullName(), Force->GetFullName() ));
			Force->NotifyParticleCreated( System, Particle );
		}
	}
	for( INT i = 0; i < AffectingForces.Num(); i++ )
	{
		NOTE(debugf(TEXT("%s (Initing particle with local  force) %s"), GetFullName(), AffectingForces(i)->GetFullName() ));
		AffectingForces(i)->NotifyParticleCreated( System, Particle );
	}

	return Particle;

	unguardf(( TEXT("(%s)"), GetFullName() ));
}

//------------------------------------------------------------------------------
void UParticleTemplate::InitParticle( UParticle* &Particle )
{
	guard(UParticleTemplate::InitParticle);
	NOTE(debugf(TEXT("UParticleTemplate::InitParticle")));

	Particle->Location    = FVector(0,0,0);
	Particle->Velocity    = VRand() * InitialSpeed.GetRand();
	Particle->LifeSpan    = InitialLifeSpan.GetRand();
	Particle->StasisTimer = StasisTime.GetRand();

	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::SetParticleDirection( UParticle* P, FVector Dir )
{
	guard(UParticleTemplate::SetParticleDirection);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::SetParticleDirection( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Dir.X, Dir.Y, Dir.Z ));
	//P->Velocity = P->Velocity.Size() * Dir;
	P->Velocity = Dir * InitialSpeed.GetRand();
	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::SetParticleLocation( UParticle* P, FVector Loc )
{
	guard(UParticleTemplate::SetParticleLocation);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::SetParticleLocation( %s, (%f,%f,%f) )"), GetFullName(), P ? *P->GetClassName() : TEXT("None"), Loc.X, Loc.Y, Loc.Z ));
	P->Location = Loc;
	unguard;
}

//------------------------------------------------------------------------------
FVector UParticleTemplate::GetParticleNormal( UParticle* P )
{
	guard(UParticleTemplate::GetParticleNormal);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::GetParticleNormal"), GetFullName() ));
	return P->Velocity.SafeNormal();
	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::NotifyForceAttachment( UForce* Force )
{
	guard(UParticleTemplate::NotifyForceAttachment);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::NotifyForceAttachment( %s )"), GetFullName(), Force ? Force->GetFullName() : TEXT("None") ));

	AffectingForces.AddItem( Force );

	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::NotifyForceDetachment( UForce* Force )
{
	guard(UParticleTemplate::NotifyForceDetachment);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::NotifyForceDetachment( %s )"), GetFullName(), Force ? Force->GetFullName() : TEXT("None") ));

	AffectingForces.RemoveItem( Force );

	unguard;
}

//------------------------------------------------------------------------------
UParticleTemplate* UParticleTemplate::Duplicate( UObject* NewOuter )
{
	guard(UParticleTemplate::Duplicate);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::Duplicate( %s )"), GetFullName(), NewOuter ? NewOuter->GetFullName() : TEXT("None") ));

	// Make sure we haven't already been duplicated.
	if( AParticleGenerator::GParticleDuplicates )
	{
		UParticleTemplate* Duplicate = Cast<UParticleTemplate>(AParticleGenerator::GParticleDuplicates->FindRef( this ));
		if( Duplicate )
			return Duplicate;
	}

	UParticleTemplate* NewTemplate = CastChecked<UParticleTemplate>( StaticConstructObject
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
	AParticleGenerator::GParticleDuplicates->Set( this, NewTemplate );
	AParticleGenerator::GParticleDuplicates->Set( NewTemplate, NewTemplate );

	if( NewTemplate )
	{
		// Fix ARL: remove me after TArray duplication has been fixed.
		ZERO_OUT(NewTemplate->ParticleList);
		NewTemplate->ParticleList.Empty();
		NewTemplate->ParticleList.Add(ParticleList.Num());

		for( INT i=0; i<ParticleList.Num(); i++ )
			NewTemplate->ParticleList(i) = ParticleList(i)->Duplicate( NewOuter );

		NewTemplate->Extents = Extents;

		// Delete existing inner objects first.
		{TArray<UForce*> Remove=NewTemplate->AffectingForces;
		for( INT i=0; i<Remove.Num(); i++ )
			if( Remove(i)->IsIn( NewTemplate ) )
				Remove(i)->DeleteUObject();}

		// Fix ARL: remove me after TArray duplication has been fixed.
		ZERO_OUT(NewTemplate->AffectingForces);
		NewTemplate->AffectingForces.Empty();

		for( INT i=0; i<AffectingForces.Num(); i++ )
			if( AffectingForces(i)->IsIn(GetOuter()) )	// Only duplicate inner objects (as opposed to ExternallyAffected forces).
				NewTemplate->AffectingForces.AddItem( AffectingForces(i)->Duplicate( NewOuter ) );
	}

	return NewTemplate;

	unguard;
}

//------------------------------------------------------------------------------
void UParticle::Destroy()
{
	guard(UParticle::Destroy);
	for(TMap<UForce*,UForceVars*>::TIterator It(AdditionalVars);It;++It)
		delete It.Value();
	AdditionalVars.Empty();
	unguard;
}

//------------------------------------------------------------------------------
UParticle* UParticle::Duplicate( UObject* NewOuter )
{
	guard(UParticle::Duplicate);

	// Make sure we haven't already been duplicated.
	if( AParticleGenerator::GDuplicateParticles )
	{
		UParticle* Duplicate = AParticleGenerator::GDuplicateParticles->FindRef( this );
		if( Duplicate )
			return Duplicate;
	}

	UParticle* NewParticle = GetClass()->CreateInstance();
	appMemcpy( NewParticle, this, GetSize() );

	if( !AParticleGenerator::GDuplicateParticles )
	{
		AParticleGenerator::GDuplicateParticles = new TMap< UParticle*, UParticle* >;
		check(AParticleGenerator::GDuplicateParticles!=NULL);
	}
	AParticleGenerator::GDuplicateParticles->Set( this, NewParticle );
	AParticleGenerator::GDuplicateParticles->Set( NewParticle, NewParticle );

	ZERO_OUT(NewParticle->AdditionalVars);
	NewParticle->AdditionalVars.Empty();
	for( TMap< UForce*, UForceVars* >::TIterator It(AdditionalVars);It;++It )
		NewParticle->AdditionalVars.Set( It.Key()->Duplicate(NewOuter), It.Value()->Duplicate(NewOuter) );

	return NewParticle;
	unguard;
}

//------------------------------------------------------------------------------
UForceVars* UForceVars::Duplicate( UObject* NewOuter )
{
	guard(UForceVars::Duplicate);

	// Make sure we haven't already been duplicated.
	if( AParticleGenerator::GDuplicateForceVars )
	{
		UForceVars* Duplicate = AParticleGenerator::GDuplicateForceVars->FindRef( this );
		if( Duplicate )
			return Duplicate;
	}

	UForceVars* NewForceVars = GetClass()->CreateInstance();
	appMemcpy( NewForceVars, this, GetSize() );

	if( !AParticleGenerator::GDuplicateForceVars )
	{
		AParticleGenerator::GDuplicateForceVars = new TMap< UForceVars*, UForceVars* >;
		check(AParticleGenerator::GDuplicateForceVars!=NULL);
	}
	AParticleGenerator::GDuplicateForceVars->Set( this, NewForceVars );
	AParticleGenerator::GDuplicateForceVars->Set( NewForceVars, NewForceVars );

	return NewForceVars;
	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::Clean()
{
	guard(UParticleTemplate::Clean);
	NOTE(debugf(TEXT("UParticleTemplate::Clean")));

	for( INT i=0; i<ParticleList.Num(); i++ )
	{
		UParticle* Particle = ParticleList(i);
		RecycleParticle( Particle );
	}

	ParticleList.Empty();

	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::RecycleParticle( UParticle* Particle )
{
	guard(UParticleTemplate::RecycleParticle);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::RecycleParticle( %s )"), GetFullName(), Particle ? *Particle->GetClassName() : TEXT("None") ));

	check(Particle!=NULL);

	if( !PurgingGarbage() )
	{
		AParticleGenerator* System = CastChecked<AParticleGenerator>(GetOuter());

		// Notify forces.
		for( INT i = 0; i < System->Forces.Num(); )
		{
			UForce* Force = System->Forces(i);
			if( Force && Force->bIterateAll )
			{
				NOTE(debugf(TEXT("%s (UParticleTemplate::RecycleParticle | Notifying global force) %s"), GetFullName(), Force->GetFullName() ));
				Force->NotifyParticleDestroyed( System, Particle );
			}
			if( i < System->Forces.Num() && Force == System->Forces(i) ) i++;	// just in case the force was deleted on account of the above notification.
		}
		for( INT i = 0; i < AffectingForces.Num(); )
		{
			UForce* Force = AffectingForces(i);
			if( Force )
			{
				NOTE(debugf(TEXT("%s (UParticleTemplate::RecycleParticle | Notifying local  force) %s"), GetFullName(), Force->GetFullName() ));
				Force->NotifyParticleDestroyed( System, Particle );
			}
			if( i < AffectingForces.Num() && Force == AffectingForces(i) ) i++;	// just in case the force was deleted on account of the above notification.
		}
	}

	Particle->Destroy();
	delete Particle;

	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::Serialize( FArchive& Ar )
{
	guard(UParticleTemplate::Serialize);

	Super::Serialize( Ar );

//!!MERGE	if( Ar.LicenseeVer() >= LVERSION_PARTICLESERIALIZATION )
	{
		if( !Ar.IsTrans() && (Ar.IsLoading() || Ar.IsSaving()) )
		{
			NOTE(debugf(TEXT("Serializing %s"),GetFullName()));
			if( UClient::GetDefaultSettings()->ParticleSafeMode==1 )
			{
				if( Ar.IsSaving() )
				{
					Clean();
				}
				else
				{
					ParticleList.Empty();
					return;
				}
			}

			INT NumParticles;
			if( Ar.IsSaving() )
				NumParticles = ParticleList.Num();
			Ar << AR_INDEX(NumParticles);
			if( Ar.IsLoading() )
			{
				ParticleList.Empty(NumParticles);
				ParticleList.Add(NumParticles);
				LoadParticles ArLoad(Ar);
				for(INT i=0;i<NumParticles;i++)
					ArLoad << ParticleList(i);
			}
			else
			{
				SaveParticles ArSave(Ar);
				for(INT i=0;i<NumParticles;i++)
					ArSave << ParticleList(i);
			}
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
void UParticleTemplate::Destroy()
{
	guard(UParticleTemplate::Destroy);
	NOTE(debugf(TEXT("(%s)UParticleTemplate::Destroy"),GetFullName()));

	Clean();

	if( !PurgingGarbage() )
	{
		AParticleGenerator* System = Cast<AParticleGenerator>(GetOuter());
		if( System )
		{
			System->RemoveTemplate( this );
		}

		TArray<UForce*> Removed=AffectingForces;
		for( INT i=0; i<Removed.Num(); i++ )
			if( Removed(i) )
				Removed(i)->RemoveTemplate( this );

		check(AffectingForces.Num()==0);
	}

	Super::Destroy();
	unguard;
}

//------------------------------------------------------------------------------
FColor UParticleTemplate::Light( UParticle* Particle, TList<FDynamicLight*>* Lights )
{
	guard(UParticleTemplate::Light);

	if( bDynamicLit )
	{
		FColor Color = FColor(12,12,12,12);	// never go completely invisible because of lighting.

		INT NumLights=0;
		for(TList<FDynamicLight*>* LightList = Lights;LightList;LightList = LightList->Next)
		{
			FDynamicLight* Light = LightList->Element;
			Color += FColor(Light->Color * Light->SampleIntensity(Particle->Location,(Light->Position - Particle->Location).SafeNormal()));

			if(++NumLights==MaxDynamicLights)
				break;
		}

		return Color;
	}
	else
	{
		USpriteParticle* SpriteParticle = ParticleCast<USpriteParticle>(Particle);
		return SpriteParticle ? SpriteParticle->Color : FColor(255,255,255,255);
	}

	unguard;
}

IMPLEMENT_CLASS(UParticleTemplate);

