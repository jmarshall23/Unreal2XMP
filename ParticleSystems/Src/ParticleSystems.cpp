//=============================================================================
// ParticleSystems.cpp
// $Author: ALeiby $
// $Date: 2003/05/07 $
// $Revision: #2 $
//=============================================================================

#include "Core.h"
#include "Engine.h"

#include "ParticleSystems.h"
#include "UnParticleRender.h"

// Register names.
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) PARTICLESYSTEMS_API FName PARTICLESYSTEMS_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name) IMPLEMENT_FUNCTION(cls,idx,name)
#include "ParticleSystemsClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

TMap< UObject*, UObject* >* AParticleGenerator::GParticleDuplicates = NULL;
TMap< UParticle*, UParticle* >* AParticleGenerator::GDuplicateParticles = NULL;
TMap< UForceVars*, UForceVars* >* AParticleGenerator::GDuplicateForceVars = NULL;


IMPLEMENT_PACKAGE_EX(ParticleSystems)
{
	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name) PARTICLESYSTEMS_##name = FName(TEXT(#name),FNAME_Intrinsic);
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "ParticleSystemsClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY
}


/*-----------------------------------------------------------------------------
	Particle creation.
-----------------------------------------------------------------------------*/

particleClass* StringFindParticleClass( const TCHAR* Class )
{
	for(particleClass* C=particleClass::GetClassList();C;C=C->NextClass())
		if( appStricmp( Class, C->GetName() )==0 )
			return C;

	// try tacking a 'U' on the front.
	FString UName = FString(TEXT("U")) + FString(Class);
	for(particleClass* C=particleClass::GetClassList();C;C=C->NextClass())
		if( UName == C->GetName() )
			return C;

	debugf(NAME_Warning,TEXT("Particle class not found: '%s'!"),Class);
	return NULL;
}


/*-----------------------------------------------------------------------------
	ForceVars creation.
-----------------------------------------------------------------------------*/

forcevarsClass* StringFindForceVarsClass( const TCHAR* Class )
{
	for(forcevarsClass* C=forcevarsClass::GetClassList();C;C=C->NextClass())
		if( appStricmp( Class, C->GetName() )==0 )
			return C;

	// try tacking a 'U' on the front.
	FString UName = FString(TEXT("U")) + FString(Class);
	for(forcevarsClass* C=forcevarsClass::GetClassList();C;C=C->NextClass())
		if( UName == C->GetName() )
			return C;

	debugf(NAME_Warning,TEXT("ForceVars class not found: '%s'!"),Class);
	return NULL;
}


/*-----------------------------------------------------------------------------
	Particle classes..
-----------------------------------------------------------------------------*/

IMPLEMENT_PARTICLE_CLASS(UParticle);
IMPLEMENT_PARTICLE_CLASS(ULineParticle);
IMPLEMENT_PARTICLE_CLASS(UParticleDecal);
IMPLEMENT_PARTICLE_CLASS(UPointParticle);
IMPLEMENT_PARTICLE_CLASS(URecursiveParticle);
IMPLEMENT_PARTICLE_CLASS(URotatingParticle);
IMPLEMENT_PARTICLE_CLASS(USprite3DParticle);
IMPLEMENT_PARTICLE_CLASS(USpriteParticle);
IMPLEMENT_PARTICLE_CLASS(UStrandParticle);
IMPLEMENT_PARTICLE_CLASS(UStreakParticle);
IMPLEMENT_PARTICLE_CLASS(UTarpParticle);
IMPLEMENT_PARTICLE_CLASS(UTexturedStreakParticle);
IMPLEMENT_PARTICLE_CLASS(UTriangleParticle);
IMPLEMENT_PARTICLE_CLASS(UTriParticle);
IMPLEMENT_PARTICLE_CLASS(UViewVelocityScaledParticle);


/*-----------------------------------------------------------------------------
	ForceVars.
-----------------------------------------------------------------------------*/

IMPLEMENT_FORCEVARS_CLASS(UForceVars);
IMPLEMENT_FORCEVARS_CLASS(UFadeForceVars);
IMPLEMENT_FORCEVARS_CLASS(UFadeInForceVars);
IMPLEMENT_FORCEVARS_CLASS(UFadeOutForceVars);
IMPLEMENT_FORCEVARS_CLASS(UGeometryColliderExVars);
IMPLEMENT_FORCEVARS_CLASS(UInertiaForceVars);
IMPLEMENT_FORCEVARS_CLASS(ULimitedResizeForceVars);
IMPLEMENT_FORCEVARS_CLASS(UMoveZigVars);
IMPLEMENT_FORCEVARS_CLASS(UMultiTemplateForceVars);
IMPLEMENT_FORCEVARS_CLASS(UOscillatingPerParticleForceVars);
IMPLEMENT_FORCEVARS_CLASS(UParticleSalamanderForceVars);
IMPLEMENT_FORCEVARS_CLASS(UPerParticleFrictionVars);
IMPLEMENT_FORCEVARS_CLASS(UPerParticleGravityVars);
IMPLEMENT_FORCEVARS_CLASS(UResizeForceVars);
IMPLEMENT_FORCEVARS_CLASS(URotationForceVars);
IMPLEMENT_FORCEVARS_CLASS(URigidBodyVars);
IMPLEMENT_FORCEVARS_CLASS(USpringForceVars);
IMPLEMENT_FORCEVARS_CLASS(UTriangleForceVars);
IMPLEMENT_FORCEVARS_CLASS(UVariablePerParticleGravityVars);
IMPLEMENT_FORCEVARS_CLASS(UWindForceVars);


/*-----------------------------------------------------------------------------
	Extra junk.
-----------------------------------------------------------------------------*/

EXECFUNC(ExportParticle)
{
	guard(ExportParticle);
	NOTE(debugf(TEXT("ExportParticle: %s"), FString(inArgs[1])));
	if(inArgCount<=1 || !inArgs[1]){ GExecDisp->Printf( TEXT("Warning! ExportParticle: No object specified.") ); return; }

	AParticleGenerator* System = FindObject<AParticleGenerator>( ANY_PACKAGE, inArgs[1] );
	if(!System){ GExecDisp->Printf( TEXT("Warning! ExportParticle: System '%s' not found."), inArgs[1] ); return; }

	UPackage* Pkg = UObject::CreatePackage(NULL,TEXT("ParticleTemp"));
	debugf( TEXT("Exporting %s"), System->GetFullName() );

	AActor* NewSystem = (AActor*)UObject::StaticConstructObject( System->GetClass(), Pkg, NAME_None, 0, System );
	if( NewSystem )
	{
		// Make sure we have a level so we don't crash.
		NewSystem->XLevel = ((UEngine*)GExecDisp->UserData)->Client->Viewports(0)->Actor->GetLevel();

		UBOOL GIsSavedScriptableSaved = 1;
		Exchange(GIsScriptable,GIsSavedScriptableSaved);

		struct FuncParms{ class AActor* A; UBOOL B; };
		FuncParms Parms;
		Parms.A = System;
		Parms.B = 0;
		NewSystem->UObject::ProcessEvent(NewSystem->FindFunctionChecked(FName(TEXT("Conform"),FNAME_Intrinsic)),&Parms);
//		NewSystem->UObject::ProcessEvent(NewSystem->FindFunctionChecked(FName(TEXT("Clean"),FNAME_Intrinsic)),NULL);

		Exchange(GIsScriptable,GIsSavedScriptableSaved);

		NewSystem->Region		= NULL;
		NewSystem->Level		= NULL;
		NewSystem->XLevel		= NULL;
		NewSystem->Owner		= NULL;
		NewSystem->Instigator	= NULL;
		NewSystem->Base			= NULL;
		NewSystem->Location		= FVector(0,0,0);
		NewSystem->Rotation		= FRotator(0,0,0);
	}

	UObject::SavePackage( Pkg, NULL, 0xffffffff, TEXT("ParticleTemp.u"), GError, NULL );

	// Clean up
	{for( FObjectIterator It; It; ++It )
	{
		if( It->IsIn(Pkg) && It->IsA( TEXT("ParticleGenerator") ) )
		{
			UObject* Obj = *It;
			Obj->ConditionalDestroy();
			delete Obj;
		}
	}}

	Pkg->ConditionalDestroy();
	delete Pkg;
	unguard;
}
