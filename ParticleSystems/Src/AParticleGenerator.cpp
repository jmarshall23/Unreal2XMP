#include "ParticleSystems.h"

//------------------------------------------------------------------------------
UBOOL AParticleGenerator::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AParticleGenerator::Tick);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Tick( %f )"), GetFullName(), DeltaSeconds ));

	LastDeltaSeconds = DeltaSeconds;

//!!MERGE	if( !GIsEditor && (bManualTick || TickType==LEVELTICK_ViewportsOnly) )
	if( !GIsEditor && TickType==LEVELTICK_ViewportsOnly )
		return Super::Tick( DeltaSeconds, TickType );

#if 0 //NEW MJL: gameplay stuff runs on server in particlesystems
	// Don't tick on the server.
	if( !GetLevel()->Engine->Client )
		return 1;
#endif

	if( (AttachmentBone == NAME_None || !bAttachDelayTick) && !bForceDelayTick )	// particle attachments will tick themselves in render using LastDeltaSeconds.
		ManualTick( DeltaSeconds );

	return Super::Tick( DeltaSeconds, TickType );

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::ManualTick( FLOAT DeltaSeconds )
{
	guard(AParticleGenerator::ManualTick);
	NOTE(debugf(TEXT("AParticleGenerator::ManualTick")));

	if (!bLockParticles) {	/*goto END*/

//!!MERGE	DeltaSeconds *= Clamp( ActorTimeDilation, 0.1f, 255.f );
	DeltaSeconds = Clamp( DeltaSeconds, 0.0f, 0.5f );

	UBOOL bParticleTick = true;
	if( !GIsEditor )
	{
		// PrimeTime.
		if( PrimeTime < 0.f )
		{
			PrimeTime += DeltaSeconds;
			if( PrimeTime < 0.f )
				bParticleTick = false;
			else
			{
				DeltaSeconds = PrimeTime;
				PrimeTime = 0.f;
			}
		}
		else if( PrimeTime > 0.f )
		{
			DeltaSeconds += PrimeTime;
			PrimeTime = 0.f;
		}

		// ParticleLifeSpan.
		if( ParticleLifeSpan < 0.f )
		{
//!!MERGE			GetLevel()->DestroyActor( this, 0, bAttachment );
			GetLevel()->DestroyActor( this, 0 );
			return;
		}
	}

	// RampTime.
	if( RampTimer > 0.f )
	{
		RampTimer = Max( 0.f, RampTimer - DeltaSeconds );

		if( RampDir==RAMP_Up )
		{
			VolumeScale = 1.0f - (RampTimer / RampUpTime);
		}
		else if( RampDir==RAMP_Down )
		{
			VolumeScale = RampTimer / RampDownTime;
			if( RampTimer==0.f )
				bOn=false;
		}
	}
	if( RampDir==RAMP_None )
		VolumeScale = 1.f;

	FLOAT TimeIdle = GetLevel()->GetLevelInfo()->TimeSeconds - LastTimeDrawn;

	// Shut down if we haven't been drawn in a while.
	if
	(	IdleTime == 0.f
	||	TimeIdle < IdleTime
	||	(bOn && NumParticles() == 0)
	)
	{
		// Tick subclasses (generate particles).
		clock(GStats.DWORDStats(GEngineStats.STATS_Particle_TickCycles));
		if( bParticleTick )
		{
			FLOAT RemainingTick = DeltaSeconds;
			if( InternalTimer>0.0f && RemainingTick>InternalTimer )
				RemainingTick = InternalTimer;
			FLOAT TotalTick = RemainingTick;
			if( bInterpolate && LastLocation != FVector(0,0,0) && MaxTickSize > 0.f )
			{
				FLOAT Pct = (FLOAT)((INT)(RemainingTick / MaxTickSize));
				FVector DeltaLoc = (Location - LastLocation) / Pct;
#if 1 //NEW: account for wrapping.
				INT DeltaRotPitch = Rotation.Pitch - LastPitch;
				INT DeltaRotYaw   = Rotation.Yaw   - LastYaw;
				INT DeltaRotRoll  = Rotation.Roll  - LastRoll;
				DeltaRotPitch = DeltaRotPitch & 0xFFFF; if( DeltaRotPitch > 32767 ) DeltaRotPitch -= 0x10000;
				DeltaRotYaw   = DeltaRotYaw   & 0xFFFF; if( DeltaRotYaw   > 32767 )	DeltaRotYaw   -= 0x10000;
				DeltaRotRoll  = DeltaRotRoll  & 0xFFFF; if( DeltaRotRoll  > 32767 )	DeltaRotRoll  -= 0x10000;
				FLOAT DeltaPitch = DeltaRotPitch / Pct;
				FLOAT DeltaYaw   = DeltaRotYaw   / Pct;
				FLOAT DeltaRoll  = DeltaRotRoll  / Pct;
#else
				FLOAT DeltaPitch = (Rotation.Pitch - LastPitch) / Pct;
				FLOAT DeltaYaw   = (Rotation.Yaw   - LastYaw  ) / Pct;
				FLOAT DeltaRoll  = (Rotation.Roll  - LastRoll ) / Pct;
#endif
				while( RemainingTick > MaxTickSize )
				{
					LastLocation += DeltaLoc;
					LastPitch    += DeltaPitch;
					LastYaw      += DeltaYaw;
					LastRoll     += DeltaRoll;

					FRotator LastRotation = FRotator(LastPitch,LastYaw,LastRoll);
					ParticleTick( MaxTickSize, LastLocation, LastRotation, (TotalTick - RemainingTick) );

					RemainingTick -= MaxTickSize;
				}
			}
			ParticleTick( RemainingTick, Location, Rotation, (TotalTick - RemainingTick) );
			LastLocation = Location;
			LastPitch    = Rotation.Pitch;
			LastYaw      = Rotation.Yaw;
			LastRoll     = Rotation.Roll;
		}
		unclock(GStats.DWORDStats(GEngineStats.STATS_Particle_TickCycles));

		// Update forces.
		// NOTE[aleiby]: Forces are examined on particles created on this tick so they don't get all
		// bunched up at the emmision location.  StasisTime will take care of any ambiguities involved.
		clock(GStats.DWORDStats(GEngineStats.STATS_Particle_ForceCycles));
		GStats.DWORDStats(GEngineStats.STATS_Particle_NumForces) += Forces.Num();
		FLOAT OldBoundSizeSquared = GetPrimitive()->GetRenderBoundingBox(this).GetExtent().SizeSquared();
			ForceTick(DeltaSeconds);	// factored out to allow subclasses to override.
		FLOAT NewBoundSizeSquared = GetPrimitive()->GetRenderBoundingBox(this).GetExtent().SizeSquared();
		if
		(	((NewBoundSizeSquared==0 || OldBoundSizeSquared==0) && NewBoundSizeSquared!=OldBoundSizeSquared)
		||	(Abs(NewBoundSizeSquared - LastBoundSizeSquared) > 256.f) )
		{
			ClearRenderData();
			LastBoundSizeSquared = NewBoundSizeSquared;
		}
		unclock(GStats.DWORDStats(GEngineStats.STATS_Particle_ForceCycles));

		// Update StasisTimers
		for( INT iTemplate = 0; iTemplate < ParticleTemplates.Num(); iTemplate++ )
		{
			UParticleTemplate* Template = ParticleTemplates(iTemplate);
			check(Template!=NULL); //!!Slow
			for( INT i=0; i<Template->ParticleList.Num(); i++ )
			{
				UParticle* Particle = Template->ParticleList(i);
				if( Particle->StasisTimer > 0.f )
					Particle->StasisTimer -= DeltaSeconds;
				if( Particle->StasisTimer < 0.f )
					Particle->StasisTimer = 0.f;
			}
		}
	}

	}	/*END*/

	// Update Timer as needed.
	if( InternalTimer > 0.f )
	{
		InternalTimer -= DeltaSeconds;

		if( InternalTimer <= 0.f )
		{
			InternalTimer = 0.f;
			if(bInitiallyOn) eventTurnOn();
			else             eventTurnOff();
		}
	}

	if( !GIsEditor )
	{
		if( ParticleLifeSpan != 0.f )
		{
			ParticleLifeSpan -= DeltaSeconds;
			if( ParticleLifeSpan == 0.f )
				ParticleLifeSpan = -0.0001f;
		}
	}

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::ForceTick( FLOAT DeltaSeconds )
{
	guard(AParticleGenerator::ForceTick);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Render"),GetFullName()));

	for( INT i=0; i<Forces.Num(); i++ )
		if( Forces(i) )
			Forces(i)->Apply( this, DeltaSeconds );
	for( INT i=0; i<ParticleTemplates.Num(); i++ )
		if( ParticleTemplates(i) )
			ParticleTemplates(i)->Tick( this, DeltaSeconds );

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::Render( FDynamicActor* Owner, FLevelSceneNode* Frame, TList<FDynamicLight*>* Lights, FRenderInterface* RI )
{
	guard(AParticleGenerator::Render);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Render"),GetFullName()));

	if( (AttachmentBone != NAME_None && bAttachDelayTick) || bForceDelayTick )		// delayed tick to wait for attachment location to be updated before spawning new particles.
		ManualTick( LastDeltaSeconds );

	INT NumParticlesDrawn=0;

	// Draw icon in editor.
	if( GIsEditor && !(Frame->Viewport->Actor->ShowFlags & SHOW_PlayerCtrl) )
	{
		RI->PushState();
		INT PrevDT = DrawType;
		DrawType = DT_Sprite;
		Owner->Render(Frame,Lights,NULL,RI);
		DrawType = PrevDT;
		RI->PopState();
	}

	// Draw bounding box.
	if( bShowBoundingBox )
	{
		RI->PushState();
		RI->SetTransform(TT_LocalToWorld,FMatrix::Identity);
		FLineBatcher LineBatcher(RI);
		FBox BoundingBox = GetPrimitive()->GetRenderBoundingBox(this).TransformBy(LocalToWorld());
		LineBatcher.DrawBox(BoundingBox,FColor(255,0,255));
		LineBatcher.Flush();
		RI->PopState();
	}
	
	clock(GStats.DWORDStats(GEngineStats.STATS_Particle_DrawCycles));
	for( INT iTemplate = 0; iTemplate < ParticleTemplates.Num(); iTemplate++ )
	{
		RI->PushState();

		NumParticlesDrawn += ParticleTemplates(iTemplate)->Render(Owner,Frame,Lights,RI,this);

		RI->PopState();
	}
	unclock(GStats.DWORDStats(GEngineStats.STATS_Particle_DrawCycles));

	if( NumParticlesDrawn > 0 )
	{
		LastTimeDrawn = GetLevel()->GetLevelInfo()->TimeSeconds;	
	}

	GStats.DWORDStats(GEngineStats.STATS_Particle_NumParticles) += NumParticlesDrawn;

	unguard;
}

//------------------------------------------------------------------------------
UParticleTemplate* AParticleGenerator::GetNextTemplate()
{
	guard(AParticleGenerator::GetNextTemplate);
	NOTE(debugf(TEXT("AParticleGenerator::GetNextTemplate")));

	// Fix ARL: Clean up empty slots in template array (ParticleTemplates) so we don't have to check for them all over the place.

	// Enforce MaxParticles.
	// NOTE[aleiby]: This is done here since this function always gets called before a particle is created, and if we return NULL
	// that will force the ParticleGenerator to give up trying to generate any more particles this tick.
	if( !GIsEditor && MaxParticles>0 )
	{
		if( ParticleCount <= 0 )
			return NULL;
		ParticleCount -= 1;
	}

	INT i;
	FLOAT TotalWeight, WeightedVal;
	UParticleTemplate* Template=NULL;

	for( i = 0; i < ParticleTemplates.Num(); i++ )
	{
		switch( SelectionMethod )
		{
		case SELECT_Linear:
			
			// Particle:Weight
			// A:4
			// B:4
			// C:2
			// Distribution: AB ABC AB ABC

			if( bLinearFrequenciesChanged )
			{
				// Integerize weights and find biggest value.
				LargestWeight = 0;
				for( i = 0; i < ParticleTemplates.Num(); i++ )
				{
					Template = ParticleTemplates(i);
					if( Template )
					{
						Template->SelectionWeight = (INT)Template->SelectionWeight;
						LargestWeight = Max( Template->SelectionWeight, LargestWeight );
					}
				}

				// Recalculate CumulativeFreqs.
				for( i = 0; i < ParticleTemplates.Num(); i++ )
				{
					Template = ParticleTemplates(i);
					if( Template )
					{
						Template->Frequency = Template->SelectionWeight ? LargestWeight / Template->SelectionWeight : 0.f;
						Template->CumulativeFreq = Template->Frequency;
					}
				}

				// Reset vars.
				TemplateIndex = 0;
				TemplateIteration = 1;
				bLinearFrequenciesChanged = false;
			}

			// If at least one template has a non-zero weight...
			if( LargestWeight > 0.f )
			{
				// Find the next particle who's frequency has been met (remember one frequency will always be 1.0).
				while
				(	ParticleTemplates(TemplateIndex)->CumulativeFreq == 0.0
				||	ParticleTemplates(TemplateIndex)->CumulativeFreq > (FLOAT)TemplateIteration
				)
				{
					//debugf( TEXT("(%d): ParticleTemplates(%d) = %f"), TemplateIteration, TemplateIndex, ParticleTemplates(TemplateIndex)->CumulativeFreq );
					TemplateIndex = (TemplateIndex+1) % ParticleTemplates.Num();
					if( TemplateIndex == 0 )
					{
						TemplateIteration++;
						if( TemplateIteration > LargestWeight )
						{
							TemplateIteration = 1;

							// Reset CumulativeFreqs.
							for( i = 0; i < ParticleTemplates.Num(); i++ )
							{
								ParticleTemplates(i)->CumulativeFreq = ParticleTemplates(i)->Frequency;
							}
						}
					}
				}
				ParticleTemplates(TemplateIndex)->CumulativeFreq += ParticleTemplates(TemplateIndex)->Frequency;
				//debugf( TEXT("%d"), TemplateIndex );
			}

			Template = ParticleTemplates( TemplateIndex );
			break;

		case SELECT_Random:
		default:
			// Add up all the weights.
			TotalWeight = 0;
			for( i = 0; i < ParticleTemplates.Num(); i++ )
			{
				Template = ParticleTemplates(i);
				if( Template )
				{
					TotalWeight += Template->SelectionWeight;
				}
			}

			// Pick a random number between 0 and TotalWeight.
			WeightedVal = TotalWeight * appFrand();

			// Map back to a slot.
#if 0 // NOTE[aleiby]: I blame the compiler.
			for
			(	TemplateIndex = 0
			,	Template = ParticleTemplates( TemplateIndex )
			,	TotalWeight = Template ? Template->SelectionWeight : 0.f

			;	TotalWeight < WeightedVal

			;	TemplateIndex++
			,	Template = ParticleTemplates( TemplateIndex )
			,	TotalWeight += Template ? Template->SelectionWeight : 0.f
			);
#else
			TemplateIndex = 0;
			Template = ParticleTemplates( TemplateIndex );
			TotalWeight = Template ? Template->SelectionWeight : 0.f;
			while( TotalWeight < WeightedVal )
			{
				TemplateIndex++;
				if( TemplateIndex == ParticleTemplates.Num() ) break;
				Template = ParticleTemplates( TemplateIndex );
				TotalWeight += Template ? Template->SelectionWeight : 0.f;
			}
#endif

			break;
		}

		// Keep trying until we find a valid template.
		// (Artifically limited to ParticleTemplates.Num() tries.)
		if( Template && Template->SelectionWeight > 0.f )
		{
			NOTE(debugf(TEXT("Using %s"), Template->GetFullName() ));
			return Template;
		}
	}

	debugf( NAME_Warning, TEXT("Failed to find a valid ParticleTemplate for %s"), GetFullName() );
	return NULL;
	
	unguard;
}

////////////////
// Interfaces //
////////////////

//------------------------------------------------------------------------------
void AParticleGenerator::AddForce( UForce* F )
{
	guard(AParticleGenerator::AddForce);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::AddForce( %s )"), GetFullName(), F ? F->GetFullName() : TEXT("None")));

	if( F && Forces.FindItemIndex(F)==INDEX_NONE && !ForceIsIgnored( F ) )
	{
		Forces.AddItem( F );
		F->NotifyAttachedToSystem( this );
	}

	unguard;
}

//------------------------------------------------------------------------------
bool AParticleGenerator::ForceIsIgnored( UForce* F )
{
	guard(AParticleGenerator::ForceIsIgnored);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::ForceIsIgnored( %s )"), GetFullName(), F ? F->GetFullName() : TEXT("None")));

	for( INT i = 0; i < IgnoredForces.Num(); i++ )
		if( F->GroupName == IgnoredForces(i) )
			return true;

	return false;

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::RemoveForce( UForce* F )
{
	guard(AParticleGenerator::RemoveForce);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::RemoveForce( %s )"), GetFullName(), F ? F->GetFullName() : TEXT("None")));

	if( F && Forces.FindItemIndex(F) != INDEX_NONE )
	{
		F->NotifyDetachedFromSystem( this );
		Forces.RemoveItem( F );
	}

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::RemoveForceType( FName ClassName, UBOOL bAndSubclasses )
{
	guard(AParticleGenerator::RemoveForceType);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::RemoveForceType( %s, %s )"), GetFullName(), *ClassName, bAndSubclasses ? TEXT("True") : TEXT("False")));

	for( INT i=0; i<Forces.Num(); /*below*/ )
		if( Forces(i) && bAndSubclasses ? Forces(i)->IsA(ClassName) : Forces(i)->GetClass()->GetFName()==ClassName )
			RemoveForce( Forces(i) );
		else i++;

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::AddTemplate( UParticleTemplate* T )
{
	guard(AParticleGenerator::AddTemplate);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::AddTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None")));
	if( T )
	{
		ParticleTemplates.AddItem( T );
		T->NotifyAddedToSystem( this );
	}
	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::RemoveTemplate( UParticleTemplate* T )
{
	guard(AParticleGenerator::RemoveTemplate);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::RemoveTemplate( %s )"), GetFullName(), T ? T->GetFullName() : TEXT("None")));
	if( T )
	{
		ParticleTemplates.RemoveItem( T );
		T->NotifyRemovedFromSystem( this );
	}
	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::RemoveTemplateType( FName ClassName, UBOOL bAndSubclasses )
{
	guard(AParticleGenerator::RemoveTemplateType);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::RemoveTemplateType( %s, %s )"), GetFullName(), *ClassName, bAndSubclasses ? TEXT("True") : TEXT("False")));

	for( INT i=0; i<ParticleTemplates.Num(); /*below*/ )
		if( ParticleTemplates(i) && bAndSubclasses ? ParticleTemplates(i)->IsA(ClassName) : ParticleTemplates(i)->GetClass()->GetFName()==ClassName )
			RemoveTemplate( ParticleTemplates(i) );
		else i++;

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::LockParticles()
{
	guard(AParticleGenerator::LockParticles);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::LockParticles"), GetFullName()));

	bLockParticles = true;
	LockedLocalToWorld = FTranslationMatrix(-Location) * FInverseRotationMatrix(Rotation);	// Fix ARL: Maybe override WorldToLocal and use that instead?

	ClearRenderData();

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::UnLockParticles()
{
	guard(AParticleGenerator::UnLockParticles);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::UnLockParticles"), GetFullName()));

	bLockParticles = false;
	LockedLocalToWorld = FMatrix::Identity;

	ClearRenderData();

	unguard;
}


/////////////
// Helpers //
/////////////

//------------------------------------------------------------------------------
// Fix ARL: !!DANGER WIL ROBINSON!!  Way too many assumptions going on here.
//------------------------------------------------------------------------------
void AParticleGenerator::Conform( AParticleGenerator* Image, UBOOL bDeleteExisting )
{
	guard(AParticleGenerator::Conform);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Conform( %s )"), GetFullName(), Image ? Image->GetFullName() : TEXT("None") ));

	if( !Image )
	{
		debugf( NAME_Warning, TEXT("(%s) Conform failed because Image was None."), GetFullName() );
		return;
	}

	if( bDeleteExisting )
		DestroyComponents();

	// Copy Tag since this gets reset in SpawnActor (needed for triggering attachments, etc.)
	Tag = Image->Tag;

	// Delete existing inner objects.	
	{TOrderedArray<UParticleTemplate*> Remove=ParticleTemplates;
	for( INT i=0; i<Remove.Num(); i++ )
		if( Remove(i)->IsIn( this ) )
			Remove(i)->DeleteUObject();}

	// Duplicate ParticleTemplates.
	ParticleTemplates.Empty();
	for( INT i=0; i<Image->ParticleTemplates.Num(); i++ )
		if( Image->ParticleTemplates(i) )
			ParticleTemplates.AddItem( Image->ParticleTemplates(i)->Duplicate( this ) );

	// Delete existing inner objects.	
	{TOrderedArray<UForce*> Remove=Forces;
	for( INT i=0; i<Remove.Num(); i++ )
		if( Remove(i)->IsIn( this ) )
			Remove(i)->DeleteUObject();}

	// Duplicate Forces.
	Forces.Empty();
	for( INT i=0; i<Image->Forces.Num(); i++ )
		if( Image->Forces(i) )
			Forces.AddItem( Image->Forces(i)->Duplicate( this ) );

	// Copy IgnoredForces?

	// Clean up.
	if( GParticleDuplicates )
	{
		GParticleDuplicates->Empty();
		delete GParticleDuplicates;
		GParticleDuplicates = NULL;
	}
	if( GDuplicateParticles )
	{
		GDuplicateParticles->Empty();
		delete GDuplicateParticles;
		GDuplicateParticles = NULL;
	}
	if( GDuplicateForceVars )
	{
		GDuplicateForceVars->Empty();
		delete GDuplicateForceVars;
		GDuplicateForceVars = NULL;
	}

	unguard;
}

//------------------------------------------------------------------------------
// Fix ARL: !!DANGER WIL ROBINSON!!  Way too many assumptions going on here.
//------------------------------------------------------------------------------
AParticleGenerator* AParticleGenerator::Duplicate( ULevel* Level )
{
	guard(AParticleGenerator::Duplicate);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Duplicate"), GetFullName() ));

	if( !Level )
	{
		debugf( NAME_Warning, TEXT("AParticleGenerator::Duplicate - Given Level was NULL.  Cannot duplicate ParticleGenerator.") );
		return NULL;
	}

	UBOOL GIsScriptableSaved = 0;	// don't call PreBeginPlay, InitExecution, etc. in script.
	if(GIsEditor)Exchange(GIsScriptable,GIsScriptableSaved);

	AParticleGenerator* NewGenerator = CastChecked<AParticleGenerator>(Level->SpawnActor
	( 
		GetClass(),
		NAME_None, 
		Location + FVector(32,32,0),
		Rotation,
		this,
		0,
		0
	));

	if( NewGenerator )
	{
		NewGenerator->Conform( this, 0 );
	}

	if(GIsEditor)Exchange(GIsScriptable,GIsScriptableSaved);

	return NewGenerator;

	unguard;
}

//------------------------------------------------------------------------------
struct IterateArrays
{
	UParticleTemplate *Old, *New;
	IterateArrays(UParticleTemplate* InOld, UParticleTemplate* InNew)
		: Old(InOld), New(InNew){}

	void Exec( UObject* Obj, void(IterateArrays::*Callback)(UObject*&) )
	{
		for( TFieldIterator<UProperty> iField(Obj->GetClass()); iField; ++iField )
		{
			UArrayProperty* Array = Cast<UArrayProperty>(*iField);
			if(Array)
			{
				UObjectProperty* Inner = Cast<UObjectProperty>(Array->Inner);
				if(Inner)
				{
					TArray<UObject*> &Objects = *(TArray<UObject*>*)((BYTE*)Obj + Array->Offset);
					for( INT i=0; i<Objects.Num(); i++ )
						(this->*Callback)(Objects(i));
				}
			}
		}
	}

	void Iterator_Debug(UObject* &Obj){debugf(TEXT("%s"),Obj->GetFullName());}
	void Iterator_Recurse(UObject* &Obj){IterateArrays::Exec(Obj,&IterateArrays::Iterator_Exchange);}
	void Iterator_Exchange(UObject* &Obj){if(Obj==Old)Obj=New;}
};

//------------------------------------------------------------------------------
void AParticleGenerator::ExchangeTemplate( UParticleTemplate* Old, UParticleTemplate* New )
{
	guard(AParticleGenerator::ExchangeTemplate);
	NOTE(debugf(TEXT("AParticleGenerator::ExchangeTemplate( %s, %s )"),Old?Old->GetFullName():TEXT("None"),New?New->GetFullName():TEXT("None")));

	IterateArrays Iterator(Old,New);
	Iterator.Exec(this,&IterateArrays::Iterator_Exchange);
	Iterator.Exec(this,&IterateArrays::Iterator_Recurse);

	unguard;
}

//------------------------------------------------------------------------------
FLOAT AParticleGenerator::GetMaxLifeSpan()
{
	guard(AParticleGenerator::GetMaxLifeSpan);
	NOTE(debugf(TEXT("AParticleGenerator::GetMaxLifeSpan")));

	FLOAT MaxLifeSpan=0;

	for( INT i=0; i<ParticleTemplates.Num(); i++ )
		if( ParticleTemplates(i) )
			MaxLifeSpan = Max( MaxLifeSpan, ParticleTemplates(i)->GetMaxLifeSpan(this) );

	for( INT i=0; i<Forces.Num(); i++ )
		if( Forces(i) )
			MaxLifeSpan = Max( MaxLifeSpan, Forces(i)->GetMaxLifeSpan(this) );

	NOTE(debugf(TEXT("%s: %f"),GetName(),MaxLifeSpan));

	return MaxLifeSpan;

	unguard;
}

//------------------------------------------------------------------------------
INT AParticleGenerator::NumParticles()
{
	guard(AParticleGenerator::NumParticles);
	NOTE(debugf(TEXT("AParticleGenerator::NumParticles")));

	INT NumParticles=0;

	for( INT i=0; i<ParticleTemplates.Num(); i++ )
		if( ParticleTemplates(i) )
			NumParticles += ParticleTemplates(i)->ParticleList.Num();

	return NumParticles;
	
	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::Clean()
{
	guard(AParticleGenerator::Clean);
	NOTE(debugf(TEXT("AParticleGenerator::Clean")));

	if( !bLockParticles )
		for( INT i=0; i<ParticleTemplates.Num(); i++ )
			if( ParticleTemplates(i) )
				ParticleTemplates(i)->Clean();

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::DestroyComponents()
{
	guard(AParticleGenerator::DestroyComponents);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::DestroyComponents"),GetFullName()));

#if 0 //SAFE_AND_SLOW

	TArray<UObject*> Inners;
	for( TObjectIterator<UObject> It; It; ++It )
		if( It->IsIn(this) )
			Inners.AddUniqueItem(*It);

	for( INT i=0; i<Inners.Num(); i++ )
		Inners(i)->DeleteUObject();

	ParticleTemplates.Empty();
	Forces.Empty();

#else

	while( Forces.Num() )
		Forces.Last()->DeleteUObject();					// This will cause the force to remove itself.

	while( ParticleTemplates.Num() )
		ParticleTemplates.Last()->DeleteUObject();		// This will cause the template to remove itself.

#endif

	unguard;
}

//------------------------------------------------------------------------------
// Note[aleiby]: Because InitExecution() is called before Spawned(), this 
// function we be executed on all newly created ParticleGenerators.  This
// probably isn't necessarily bad, but it does cause an extra bit of overhead.
//------------------------------------------------------------------------------
void AParticleGenerator::FixUp()
{
	guard(AParticleGenerator::FixUp);
	NOTE(debugf(TEXT("AParticleGenerator::FixUp")));

	///
	if( ParticleVersion < UCONST_PARTICLEVERSION_NumParticlesFix )
	{
		// Fix ARL: Convert some of these warnings to use NAME_ParticleSystems so we can turn them on and off via ini files.
		NOTE(debugf( NAME_Warning, TEXT("Fixing up %s. (NumParticlesFix)"), GetFullName() ));
		Clean();
	}

	///
	if( ParticleVersion < UCONST_PARTICLEVERSION_AffectingForces )
	{
		NOTE(debugf( NAME_Warning, TEXT("Fixing up %s. (AffectingForces)"), GetFullName() ));
		for( INT i=0; i<Forces.Num(); i++ )
		{
			UForce* Force = Forces(i);
			if( Force )
			{
				for( INT j=0; j<Force->AffectedTemplates.Num(); j++ )
				{
					UParticleTemplate* Template = Force->AffectedTemplates(j);
					if( Template )
					{
						Template->NotifyForceAttachment( Force );
					}
				}
			}
		}
	}

	///
//	if( ParticleVersion < UCONST_PARTICLEVERSION_AutoLit )
	{
		NOTE(debugf( NAME_Warning, TEXT("Fixing up %s. (AutoLit)"), GetFullName() ));
		UBOOL bDynamicLit=0;
		for( INT i=0; i<ParticleTemplates.Num(); i++ )
		{
			UParticleTemplate* Template = ParticleTemplates(i);
			if( Template && Template->bDynamicLit )
			{
				NOTE(debugf( NAME_Warning, TEXT("  -- enabling lighting for %s [template=%s]"), GetFullName(), Template->GetFullName() ));
				bDynamicLit = true;
			}
		}
		bUnlit = !bDynamicLit;
	}	

	///
	ParticleVersion = UCONST_PARTICLEVERSION_Current;

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::ValidateComponents()
{
	guard(AParticleGenerator::ValidateComponents);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::ValidateComponents"),GetFullName()));

	INT NumErrors=0;
	debugf(TEXT("Validating %s:"),GetFullName());
	for(INT i=0;i<ParticleTemplates.Num();i++)
	{
		UParticleTemplate* Template = ParticleTemplates(i);
		if(!Template){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i]==NULL"),GetFullName(),i); NumErrors++; continue; }
		if(!Template->IsIn(this)){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i]: %s is not an inner object!"),GetFullName(),i,Template->GetFullName()); NumErrors++; continue; }
		for(INT j=0;j<Template->AffectingForces.Num();j++)
		{
			UForce* Force = Template->AffectingForces(j);
			if(!Force){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i](%s)->AffectingForces[%i]==NULL"),GetFullName(),i,Template->GetFullName(),j); NumErrors++; continue; }
			if(!Force->IsIn(this)){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i](%s)->AffectingForces[%i]: %s is not an inner object!"),GetFullName(),i,Template->GetFullName(),j,Force->GetFullName()); NumErrors++; continue; }
			INT NumRefs=0;
			for(INT k=0;k<Forces.Num();k++)
				if(Forces(k)==Force)
					NumRefs++;
			if(NumRefs==0){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i](%s)->AffectingForces[%i](%s) is not in %s's Forces list!"),GetFullName(),i,Template->GetFullName(),j,Force->GetFullName(),GetFullName()); NumErrors++; continue; }
			if(NumRefs>1){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i](%s)->AffectingForces[%i](%s) is in %s's Forces list multiple times! [%i]"),GetFullName(),i,Template->GetFullName(),j,Force->GetFullName(),GetFullName(),NumRefs); NumErrors++; continue; }
			NumRefs=0;
			for(INT k=0;k<Template->AffectingForces.Num();k++)
				if(Template->AffectingForces(k)==Force)
					NumRefs++;
			if(NumRefs>1){ debugf(NAME_Warning,TEXT("(%s) ParticleTemplate[%i](%s)->AffectingForces[%i](%s) is in %s's AffectingForces list multiple times! [%i]"),GetFullName(),i,Template->GetFullName(),j,Force->GetFullName(),Template->GetFullName(),NumRefs); NumErrors++; continue; }
		}
	}
	for(INT i=0;i<Forces.Num();i++)
	{
		UForce* Force = Forces(i);
		if(!Force){ debugf(NAME_Warning,TEXT("(%s) Force[%i]==NULL"),GetFullName(),i); NumErrors++; continue; }
		if(!Force->IsIn(this)){ debugf(NAME_Warning,TEXT("(%s) Force[%i]: %s is not an inner object!"),GetFullName(),i,Force->GetFullName()); NumErrors++; continue; }
		for(INT j=0;j<Force->AffectedTemplates.Num();j++)
		{
			UParticleTemplate* Template = Force->AffectedTemplates(j);
			if(!Template){ debugf(NAME_Warning,TEXT("(%s) Forces[%i](%s)->AffectedTemplates[%i]==NULL"),GetFullName(),i,Force->GetFullName(),j); NumErrors++; continue; }
			if(!Template->IsIn(this)){ debugf(NAME_Warning,TEXT("(%s) Forces[%i](%s)->AffectedTemplates[%i]: %s is not an inner object!"),GetFullName(),i,Force->GetFullName(),j,Template->GetFullName()); NumErrors++; continue; }
			INT NumRefs=0;
			for(INT k=0;k<ParticleTemplates.Num();k++)
				if(ParticleTemplates(k)==Template)
					NumRefs++;
			if(NumRefs==0){ debugf(NAME_Warning,TEXT("(%s) Forces[%i](%s)->AffectedTemplates[%i](%s) is not in %s's ParticleTemplates list!"),GetFullName(),i,Force->GetFullName(),j,Template->GetFullName(),GetFullName()); NumErrors++; continue; }
			if(NumRefs>1){ debugf(NAME_Warning,TEXT("(%s) Forces[%i](%s)->AffectedTemplates[%i](%s) is in %s's ParticleTemplates list multiple times! [%i]"),GetFullName(),i,Force->GetFullName(),j,Template->GetFullName(),GetFullName(),NumRefs); NumErrors++; continue; }
			NumRefs=0;
			for(INT k=0;k<Force->AffectedTemplates.Num();k++)
				if(Force->AffectedTemplates(k)==Template)
					NumRefs++;
			if(NumRefs>1){ debugf(NAME_Warning,TEXT("(%s) Forces[%i](%s)->AffectedTemplates[%i](%s) is in %s's AffectedTemplates list multiple times! [%i]"),GetFullName(),i,Force->GetFullName(),j,Template->GetFullName(),Force->GetFullName(),NumRefs); NumErrors++; continue; }
		}
	}
	if(NumErrors)	debugf(TEXT("    Number of errors: %i!"),NumErrors);
	else			debugf(TEXT("    No errors!"));

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::RegisterExternallyAffectedTemplates()
{
	guard(AParticleGenerator::RegisterExternallyAffectedTemplates);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::RegisterExternallyAffectedTemplates"),GetFullName()));

	if( GetLevel() && GetLevel()->GetLevelInfo() )
	{
		for( INT i=0; i<ParticleTemplates.Num(); i++ )
			if( ParticleTemplates(i) && ParticleTemplates(i)->bExternallyAffected )
				GetLevel()->GetLevelInfo()->RegisterAffectedTemplate( ParticleTemplates(i) );
	}

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::UnRegisterExternallyAffectedTemplates()
{
	guard(AParticleGenerator::UnRegisterExternallyAffectedTemplates);
	NOTE(debugf(TEXT("AParticleGenerator::UnRegisterExternallyAffectedTemplates")));

	if( GetLevel() && GetLevel()->GetLevelInfo() )
	{
		for( INT i=0; i<ParticleTemplates.Num(); i++ )
			if( ParticleTemplates(i) && ParticleTemplates(i)->bExternallyAffected )
				GetLevel()->GetLevelInfo()->UnRegisterAffectedTemplate( ParticleTemplates(i) );
	}

	unguard;
}

//////////////////////
// AActor Overrides //
//////////////////////

//------------------------------------------------------------------------------
void AParticleGenerator::InitExecution()
{
	guard(AParticleGenerator::InitExecution);
	NOTE(debugf(TEXT("AParticleGenerator::InitExecution")));
	Super::InitExecution();
	FixUp();
	unguardobj;
}

//------------------------------------------------------------------------------
void AParticleGenerator::Spawned()
{
	guard(AParticleGenerator::Spawned);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Spawned"),GetFullName()));

	Super::Spawned();

	// NOTE[aleiby]: We're referencing the class's defaults since they don't
	// get properly copied over to instances.

	// NOTE[aleiby]: Default forces get copied correctly now, but this causes problems,
	// so just clear them out so it worked like before when it was broken still.
	DefaultForces.Empty();

	// Install default Forces.
	if( Forces.Num()==0 )	// if we were spawned using an Image, then we'll already have our default forces.
	{
		AParticleGenerator* Defaults = CastChecked<AParticleGenerator>(GetClass()->GetDefaultActor());
		for( INT i=0; i<Defaults->DefaultForces.Num(); i++ )
			if( Defaults->DefaultForces(i) )
				AddForce( CastChecked<UForce>(StaticConstructObject( Defaults->DefaultForces(i), this )) );
	}

	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::Destroy()
{
	guard(AParticleGenerator::Destroy);
	NOTE(debugf(TEXT("(%s)AParticleGenerator::Destroy"),GetFullName()));

	if( !PurgingGarbage() )
	{
		DestroyComponents();

		if( UClient::GetDefaultSettings()->ParticleSafeMode==2 )
		{
			for( TObjectIterator<UObject> It; It; ++It )
				if( It->IsIn( this ) )
					appErrorf(TEXT("Failed to delete inner object: %s"), It->GetFullName());
		}
	}

	Super::Destroy();
	unguard;
}

//------------------------------------------------------------------------------
void AParticleGenerator::PostEditChange()
{
	guard(AParticleGenerator::PostEditChange);
	NOTE(debugf(TEXT("AParticleGenerator::PostEditChange")));
	
	// particles loaded from ParticleRes don't have xlevel set.
	if(!XLevel)
		return;

	Super::PostEditChange();

	if( bLockParticles && LockedLocalToWorld==FMatrix::Identity )
		LockParticles();

	if( !bLockParticles && LockedLocalToWorld!=FMatrix::Identity )
		UnLockParticles();

	unguard;
}


/////////////////////////////
// UnrealScript Interfaces //
/////////////////////////////

//------------------------------------------------------------------------------
void AParticleGenerator::execAddForce( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execAddForce);
	NOTE(debugf(TEXT("AParticleGenerator::execAddForce")));

	P_GET_OBJECT(UForce,F);
	P_FINISH;

	AddForce( F );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execRemoveForce( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execRemoveForce);
	NOTE(debugf(TEXT("AParticleGenerator::execRemoveForce")));

	P_GET_OBJECT(UForce,F);
	P_FINISH;

	RemoveForce( F );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execRemoveForceType( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execRemoveForceType);
	NOTE(debugf(TEXT("AParticleGenerator::execRemoveForceType")));

	P_GET_NAME(ClassName);
	P_GET_UBOOL_OPTX(bAndSubclasses,0);
	P_FINISH;

	RemoveForceType( ClassName, bAndSubclasses );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execAddTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execAddTemplate);
	NOTE(debugf(TEXT("AParticleGenerator::execAddTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	AddTemplate( T );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execRemoveTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execRemoveTemplate);
	NOTE(debugf(TEXT("AParticleGenerator::execRemoveTemplate")));

	P_GET_OBJECT(UParticleTemplate,T);
	P_FINISH;

	RemoveTemplate( T );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execRemoveTemplateType( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execRemoveTemplateType);
	NOTE(debugf(TEXT("AParticleGenerator::execRemoveTemplateType")));

	P_GET_NAME(ClassName);
	P_GET_UBOOL_OPTX(bAndSubclasses,0);
	P_FINISH;

	RemoveTemplateType( ClassName, bAndSubclasses );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execGetParticleTemplates( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execGetParticleTemplates);
	NOTE(debugf(TEXT("AParticleGenerator::execGetParticleTemplates")));

	P_GET_OBJECT_REF(UObject,T);
	P_FINISH;

	*(UObject**)T = *(UObject**)&ParticleTemplates;

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execConform( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execConform);
	NOTE(debugf(TEXT("AParticleGenerator::execConform")));

	P_GET_OBJECT(AParticleGenerator,Image);
	P_GET_UBOOL_OPTX(bDeleteExisting,1);
	P_FINISH;

	Conform( Image, bDeleteExisting );
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execDuplicate( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execDuplicate);
	NOTE(debugf(TEXT("AParticleGenerator::execDuplicate")));

	P_GET_OBJECT_REF(AParticleGenerator,D);
	P_GET_OBJECT(ULevel,Level);
	P_FINISH;

	*(AParticleGenerator**)D = Duplicate(Level);
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execClean( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execClean);
	NOTE(debugf(TEXT("AParticleGenerator::execClean")));

	P_FINISH;

	Clean();

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execLockParticles( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execLockParticles);
	NOTE(debugf(TEXT("AParticleGenerator::execLockParticles")));

	P_FINISH;

	LockParticles();

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execUnLockParticles( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execUnLockParticles);
	NOTE(debugf(TEXT("AParticleGenerator::execUnLockParticles")));

	P_FINISH;

	UnLockParticles();

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execExchangeTemplate( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execExchangeTemplate);
	NOTE(debugf(TEXT("AParticleGenerator::execExchangeTemplate")));

	P_GET_OBJECT(UParticleTemplate,Old);
	P_GET_OBJECT(UParticleTemplate,New);
	P_FINISH;

	ExchangeTemplate(Old,New);

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execValidateComponents( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execValidateComponents);
	NOTE(debugf(TEXT("AParticleGenerator::execValidateComponents")));

	P_FINISH;

	ValidateComponents();

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execRegisterExternallyAffectedTemplates( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execRegisterExternallyAffectedTemplates);
	NOTE(debugf(TEXT("AParticleGenerator::execRegisterExternallyAffectedTemplates")));

	P_FINISH;

	RegisterExternallyAffectedTemplates();

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execUnRegisterExternallyAffectedTemplates( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execUnRegisterExternallyAffectedTemplates);
	NOTE(debugf(TEXT("AParticleGenerator::execUnRegisterExternallyAffectedTemplates")));

	P_FINISH;

	UnRegisterExternallyAffectedTemplates();

	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execGetMaxLifeSpan( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execGetMaxLifeSpan);
	NOTE(debugf(TEXT("AParticleGenerator::execGetMaxLifeSpan")));

	P_FINISH;

	*(FLOAT*)Result = GetMaxLifeSpan();
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execManualTick( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execManualTick);
	NOTE(debugf(TEXT("AParticleGenerator::execManualTick")));

	P_GET_FLOAT(DeltaSeconds);
	P_FINISH;

	ManualTick(DeltaSeconds);
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execParticleGetLocation( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execParticleGetLocation);
	NOTE(debugf(TEXT("AParticleGenerator::execParticleGetLocation")));

	P_GET_STRUCT(FParticleHandle,P);
	P_FINISH;

	*(FVector*)Result = P ? P->Location : FVector(0,0,0);
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execParticleSetLocation( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execParticleSetLocation);
	NOTE(debugf(TEXT("AParticleGenerator::execParticleSetLocation")));

	P_GET_STRUCT(FParticleHandle,P);
	P_GET_STRUCT(FVector,Loc)
	P_FINISH;

	if(P) P->Location = Loc;
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execParticleGetVelocity( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execParticleGetVelocity);
	NOTE(debugf(TEXT("AParticleGenerator::execParticleGetVelocity")));

	P_GET_STRUCT(FParticleHandle,P);
	P_FINISH;

	*(FVector*)Result = P ? P->Velocity : FVector(0,0,0);
	
	unguardexec;
}

//------------------------------------------------------------------------------
void AParticleGenerator::execParticleSetVelocity( FFrame& Stack, RESULT_DECL )
{
	guard(AParticleGenerator::execParticleSetVelocity);
	NOTE(debugf(TEXT("AParticleGenerator::execParticleSetVelocity")));

	P_GET_STRUCT(FParticleHandle,P);
	P_GET_STRUCT(FVector,Loc)
	P_FINISH;

	if(P) P->Velocity = Loc;
	
	unguardexec;
}


IMPLEMENT_CLASS(AParticleGenerator);