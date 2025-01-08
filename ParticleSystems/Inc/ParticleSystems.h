//=============================================================================
// ParticleSystems.h: ParticleSystems-specific package header
//=============================================================================

#ifndef _INC_ParticleSystems
#define _INC_ParticleSystems

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

#include "Core.h"
#include "Engine.h"

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef PARTICLESYSTEMS_API
	#define PARTICLESYSTEMS_API DLL_IMPORT
#endif

class AParticleGenerator;
class UParticleTemplate;
class UParticle;
class UForce;
class UForceVars;

typedef UParticle* FParticleHandle;


/*-----------------------------------------------------------------------------
	Macros.
-----------------------------------------------------------------------------*/

#define ZERO_OUT(a) appMemset(&(a),0,sizeof((a)));
#define FOR_ALL(a) for(INT i=0;i<(a).Num();i++)


/*-----------------------------------------------------------------------------
	Particle creation.
-----------------------------------------------------------------------------*/

#define NEW_PARTICLE(clas) CreateParticle<clas>()
template< class T > T* CreateParticle()
{
	T* P = new T();
	return P;
}

class particleClass* StringFindParticleClass( const TCHAR* Class );

template< class T > T* ParticleCastChecked( UParticle* A )
{
	T* P = ParticleCast<T>(A);
	if(!P) appErrorf(TEXT("ParticleCastChecked of %s to %s failed!"),A?*A->GetClassName():TEXT("NULL"),T::StaticClass()->GetName());
	return P;
}

template< class T > T* ParticleCast( UParticle* A )
{
	return (A && A->IsA(T::StaticClass())) ? (T*)A : NULL;
}

/*-----------------------------------------------------------------------------
	ForceVar creation.
-----------------------------------------------------------------------------*/

#define NEW_FORCEVARS(clas) CreateForceVars<clas>()
template< class T > T* CreateForceVars()
{
	T* V = new T();
	return V;
}

class forcevarsClass* StringFindForceVarsClass( const TCHAR* Class );

template< class T > T* ForceVarsCastChecked( UForceVars* A )
{
	T* V = ForceVarsCast<T>(A);
	if(!V) appErrorf(TEXT("ForceVarsCastChecked of %s to %s failed!"),A?*A->GetClassName():TEXT("NULL"),T::StaticClass()->GetName());
	return V;
}

template< class T > T* ForceVarsCast( UForceVars* A )
{
	return (A && A->IsA(T::StaticClass())) ? (T*)A : NULL;
}

/*-----------------------------------------------------------------------------
	FSTRUCT_Particle.
-----------------------------------------------------------------------------*/

class PARTICLESYSTEMS_API FSTRUCT_Particle
{
public:
	// Variables.
	FLOAT LifeSpan;

	FLOAT Weight;

	FLOAT MaxInitialVelocity;
	FLOAT MinInitialVelocity;

	FLOAT MaxDrawScale;
	FLOAT MinDrawScale;

	FLOAT MaxScaleGlow;
	FLOAT MinScaleGlow;

	BYTE GrowPhase;
	FLOAT MaxGrowRate;
	FLOAT MinGrowRate;

	BYTE FadePhase;
	FLOAT MaxFadeRate;
	FLOAT MinFadeRate;

	// Constructors.
	FSTRUCT_Particle() {}

	// Serializer.
	friend FArchive& operator<<( FArchive &Ar, FSTRUCT_Particle &P )
	{
		return Ar << P.LifeSpan << P.Weight << P.MaxInitialVelocity << P.MinInitialVelocity << P.MaxDrawScale << P.MinDrawScale << P.MaxScaleGlow << P.MinScaleGlow << P.GrowPhase << P.MaxGrowRate << P.MinGrowRate << P.FadePhase << P.MaxFadeRate << P.MinFadeRate;
	}
};

class PARTICLESYSTEMS_API FAdditionalData
{
public:
	// Variables.
	FRotator MaxInitialRotation;
	FRotator MinInitialRotation;
	FRotator MaxRotationRate;
	FRotator MinRotationRate;

	// Constructors.
	FAdditionalData() {}

	// Serializer.
	friend FArchive& operator<<( FArchive &Ar, FAdditionalData &D )
	{
		return Ar << D.MaxInitialRotation << D.MinInitialRotation << D.MaxRotationRate << D.MinRotationRate;
	}
};

class PARTICLESYSTEMS_API FElectricArc
{
public:
	// Variables.
	AActor* Actor;
	FVector Offset;
	FVector Point;
	FLOAT LifeSpan;
	INT iTri;
	class ABendiBeamGenerator* Arc;

	// Constructors.
	FElectricArc() {}
};

struct PARTICLESYSTEMS_API QuadPointInfo
{
	FVector Location;
	FColor Color;
	FLOAT Width;
};


/*-----------------------------------------------------------------------------
	Macros.
-----------------------------------------------------------------------------*/

#define NOTE(func)
//#define NOTE(func) func
#define NOTE1(func) func

#define RADIANS(d)	(FLOAT)((FLOAT)(d)*PI/180.f)
#define DEG(r)		(INT)(((r)*180.f/PI)+0.5f)

inline FLOAT AccuteDifference( FLOAT A, FLOAT B )
{
	FLOAT R = B - A;

	if( R < 0  ) R = -R;
	if( R > PI ) R = 2*PI - R;

	return R;
}

inline FVector CalcPerpAxis( FVector& A, FVector& B )
{
	FVector Seg = (A - B).SafeNormal();
	return FVector( Seg.Y, -Seg.X, 0 );	// Seg CROSS vect(0,0,1)
}

#define FORCE_EVENT(evnt) \
{ \
	UBOOL GIsScriptableSaved=1; \
	Exchange(GIsScriptable,GIsScriptableSaved); \
	evnt; \
	Exchange(GIsScriptable,GIsScriptableSaved); \
}


// Particle iteration macros.
//
// Generally you'll want to use the pair:
//
// PARTICLE_ITERATOR_BEGIN
//     [insert per-particle code here]
// PARTICLE_ITERATOR_END
//
// When you need to also do stuff on a per-template basis, you can split things up as follows:
//
// TEMPLATE_ITERATOR_BEGIN
//     [insert per-template code here]
//     PARTICLE_ITERATOR_BEGIN1               (note the 1)
//         [insert per-particle code here]
//     PARTICLE_ITERATOR_END1                 (note the 1)
// TEMPLATE_ITERATOR_END
//
// PARTICLE_ITERATOR_BEGIN1 can also be replaced by:
// PARTICLE_ITERATOR_NO_STASIS_BEGIN1 if you need to affect all particle regardless of stasis status, and:
// PARTICLE_ITERATOR_SAFE_BEGIN1 which is slower but necessary if you're planning to screw with the ParticleList (example: deleting particles).

#define TEMPLATE_ITERATOR_BEGIN \
	for( INT iTemplate = 0; iTemplate < (bIterateAll ? System->ParticleTemplates.Num() : AffectedTemplates.Num()); iTemplate++ ) { \
		UParticleTemplate* Template = (bIterateAll ? System->ParticleTemplates(iTemplate) : AffectedTemplates(iTemplate)); \
		check(Template!=NULL); /*!!Slow*/

#define PARTICLE_ITERATOR_STASIS_CHECK \
			if( Particle->StasisTimer < DeltaSeconds ) { \
				FLOAT DeltaTime = DeltaSeconds - Particle->StasisTimer; \
				(void)DeltaTime; /* Shut up compiler warning */

#define PARTICLE_ITERATOR_LIFESPAN_CHECK \
				if( Particle->LifeSpan > 0.f ){

#define PARTICLE_ITERATOR_BEGIN1 \
		for( INT i=0; i<Template->ParticleList.Num(); i++ ){ UParticle* Particle = Template->ParticleList(i); \
			PARTICLE_ITERATOR_STASIS_CHECK \
			PARTICLE_ITERATOR_LIFESPAN_CHECK

#define PARTICLE_ITERATOR_NO_STASIS_BEGIN1 \
		for( INT i=0; i<Template->ParticleList.Num(); i++ ){ UParticle* Particle = Template->ParticleList(i);

#define PARTICLE_ITERATOR_SAFE_BEGIN1 \
		for( INT i=0; i<Template->ParticleList.Num(); i++ ){ UParticle* Particle = Template->ParticleList(i); \
			PARTICLE_ITERATOR_STASIS_CHECK

#define PARTICLE_ITERATOR_BEGIN \
	TEMPLATE_ITERATOR_BEGIN \
	PARTICLE_ITERATOR_BEGIN1

#define PARTICLE_ITERATOR_NO_STASIS_BEGIN \
	TEMPLATE_ITERATOR_BEGIN \
	PARTICLE_ITERATOR_NO_STASIS_BEGIN1

#define TEMPLATE_ITERATOR_END	}
#define PARTICLE_ITERATOR_END1	} } }
#define PARTICLE_ITERATOR_NO_STASIS_END1 }
#define PARTICLE_ITERATOR_SAFE_END1 } }

#define PARTICLE_ITERATOR_END \
	PARTICLE_ITERATOR_END1 \
	TEMPLATE_ITERATOR_END

#define PARTICLE_ITERATOR_NO_STASIS_END \
	PARTICLE_ITERATOR_NO_STASIS_END1 \
	TEMPLATE_ITERATOR_END


// ForceVar macros.
//
// IMPORTANT: These are implemented as macros so we can avoid unnecessary casting with calls to GET_FORCEVARS, etc.
// We could get around this in C++ using template classes, unfortunately though, UnrealScript doesn't support templated classes.
//
// NOTE: These should only be used within subclasses of UForce.
//
// GET_FORCEVARS(t,p) should usually be followed by if(Vars){...}, otherwise use GET_FORCEVARS_CHECKED.
//
// Any force that needs to do things on a per-particle basis should use ForceVars.
// Just put a DECLARE_ADDITIONALVARS_CLASS in your .h file, and a
// IMPLEMENT_ADDITIONALVARS_CLASS(n) in your .cpp file.  See existing classes for example usage.
// You'll also need to implement InitParticle().  This is usually where you'll use ATTACH_FORCEVARS.

#define GET_FORCEVARS(type,particle) \
	type* Vars = (type*)particle->AdditionalVars.FindRef( this );

#define GET_FORCEVARS_CHECKED(type,particle) \
	type* Vars = (type*)particle->AdditionalVars.FindRef( this ); \
	if(!Vars){ debugf(NAME_Warning,TEXT("Failed to get ForceVars (%s) of %s!"),TEXT(#type),particle?particle->GetClassName():TEXT("NULL")); return; }
//	check(Vars!=NULL);	/*checkSlow(Vars!=NULL);*/

#define ATTACH_FORCEVARS(type,particle) \
	type* Vars = ForceVarsCastChecked<type>(type::StaticClass()->CreateInstance()); \
	particle->AdditionalVars.Set( this, Vars ); \
	NOTE(debugf(TEXT("(%s) Attached %s to %s"), GetFullName(), Vars->GetClassName(), particle->GetClassName() ));

#define DETACH_FORCEVARS(type,particle) \
	NOTE(debugf(TEXT("(%s)DETACH_FORCEVARS( %s )"), GetFullName(), particle->GetClassName() )); \
	GET_FORCEVARS(type,particle); \
	if( Vars ) \
	{ \
		particle->AdditionalVars.Remove( this ); \
		NOTE(debugf(TEXT("(%s) Detached %s from %s"), GetFullName(), Vars->GetClassName(), particle->GetClassName() )); \
		delete Vars; \
	} else { NOTE(debugf(TEXT("(%s) Attempt to detatch ForceVar from %s failed: ForceVar not found."), GetFullName(), particle->GetClassName() )); }

#define DECLARE_ADDITIONALVARS_CLASS \
	public: \
	void NotifyAttachedToSystem( AParticleGenerator* System ); \
	void NotifyExternallyAttached( UParticleTemplate* Template ); \
	void NotifyParticleCreated( AParticleGenerator* System, UParticle* &Particle ); \
	void NotifyDetachedFromSystem( AParticleGenerator* System ); \
	void NotifyExternallyDetached( UParticleTemplate* Template ); \
	void NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &Particle ); \
	private: \
	void InitParticle( UParticle* Particle ); \
	void UnInitParticle( UParticle* Particle ); \
	public:

// *cough*HACK*cough*
//
// Because we don't have the ability to override these functions within the same class (see IMPORTANT note above for why),
// I've added pre- and post- entry points within these functions to allow you to specify additional functions to call before or after.
//
// Be careful.

#define IMPLEMENT_ADDITIONALVARS_CLASS(name) \
	IMPLEMENT_ADDITIONALVARS_CLASS_OVERRIDE(name,{},{},{},{},{},{},{},{},{},{})

#define IMPLEMENT_ADDITIONALVARS_CLASS_PREOVERRIDE(name,preoverride1,preoverride2,preoverride3,preoverride4,preoverride5) \
	IMPLEMENT_ADDITIONALVARS_CLASS_OVERRIDE(name,preoverride1,preoverride2,preoverride3,preoverride4,preoverride5,{},{},{},{},{})

#define IMPLEMENT_ADDITIONALVARS_CLASS_POSTOVERRIDE(name,postoverride1,postoverride2,postoverride3,postoverride4,postoverride5) \
	IMPLEMENT_ADDITIONALVARS_CLASS_OVERRIDE(name,{},{},{},{},{},postoverride1,postoverride2,postoverride3,postoverride4,postoverride5)

#define IMPLEMENT_ADDITIONALVARS_CLASS_OVERRIDE(name,preoverride1,preoverride2,preoverride3,preoverride4,preoverride5,postoverride1,postoverride2,postoverride3,postoverride4,postoverride5) \
	void name::NotifyAttachedToSystem( AParticleGenerator* System ) \
	{ \
		guard(name::NotifyAttachedToSystem); \
		NOTE(debugf(TEXT("(%s)::NotifyAttachedToSystem( %s )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL") )); \
		preoverride1; \
		PARTICLE_ITERATOR_NO_STASIS_BEGIN \
			InitParticle( Particle ); \
		PARTICLE_ITERATOR_NO_STASIS_END \
		postoverride1; \
		unguardf(( TEXT("(%s)"), System ? System->GetFullName() : TEXT("None") )); \
	} \
	void name::NotifyExternallyAttached( UParticleTemplate* Template ) \
	{ \
		guard(name::NotifyExternallyAttached); \
		NOTE(debugf(TEXT("(%s)::NotifyExternallyAttached( %s )"), GetFullName(), Template ? Template->GetFullName() : TEXT("NULL") )); \
		for( INT i=0; i<Template->ParticleList.Num(); i++ ) \
		{ \
			UParticle* Particle = Template->ParticleList(i); \
			InitParticle(Particle); \
		} \
		unguardf(( TEXT("(%s)"), Template ? Template->GetFullName() : TEXT("None") )); \
	} \
	void name::NotifyParticleCreated( AParticleGenerator* System, UParticle* &Particle ) \
	{ \
		guard(name::NotifyParticleCreated); \
		NOTE(debugf(TEXT("(%s)::NotifyParticleCreated( %s, %s )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), Particle ? Particle->GetClassName() : TEXT("NULL") )); \
		preoverride2; \
		InitParticle( Particle ); \
		postoverride2; \
		unguardf(( TEXT("(%s)"), System ? System->GetFullName() : TEXT("None") )); \
	} \
	void name::NotifyDetachedFromSystem( AParticleGenerator* System ) \
	{ \
		guard(name::NotifyDetachedFromSystem); \
		NOTE(debugf(TEXT("(%s)::NotifyDetachedFromSystem( %s )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL") )); \
		preoverride3; \
		PARTICLE_ITERATOR_NO_STASIS_BEGIN \
			UnInitParticle( Particle ); \
		PARTICLE_ITERATOR_NO_STASIS_END \
		Super::NotifyDetachedFromSystem( System ); \
		postoverride3; \
		unguardf(( TEXT("(%s)"), System ? System->GetFullName() : TEXT("None") )); \
	} \
	void name::NotifyExternallyDetached( UParticleTemplate* Template ) \
	{ \
		guard(name::NotifyExternallyDetached); \
		NOTE(debugf(TEXT("(%s)::NotifyExternallyDetached( %s )"), GetFullName(), Template ? Template->GetFullName() : TEXT("NULL") )); \
		for( INT i=0; i<Template->ParticleList.Num(); i++ ) \
		{ \
			UParticle* Particle = Template->ParticleList(i); \
			UnInitParticle(Particle); \
		} \
		unguardf(( TEXT("(%s)"), Template ? Template->GetFullName() : TEXT("None") )); \
	} \
	void name::NotifyParticleDestroyed( AParticleGenerator* System, UParticle* &Particle ) \
	{ \
		guard(name::NotifyParticleDestroyed); \
		NOTE(debugf(TEXT("(%s)::NotifyParticleDestroyed( %s, %s )"), GetFullName(), System ? System->GetFullName() : TEXT("NULL"), Particle ? Particle->GetClassName() : TEXT("NULL") )); \
		preoverride4; \
		UnInitParticle( Particle ); \
		postoverride4; \
		unguardf(( TEXT("(%s)"), System ? System->GetFullName() : TEXT("None") )); \
	} \
	void name::UnInitParticle( UParticle* Particle ) \
	{ \
		guard(name::UnInitParticle); \
		preoverride5; \
		DETACH_FORCEVARS(name##Vars,Particle); \
		postoverride5; \
		unguardf(( TEXT("(%s)"), Particle ? Particle->GetClassName() : TEXT("None") )); \
	}


/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/

//contains basic particlesystem stream types for rendering
#include "UnParticleRenderTypes.h"

#include "ParticleSystemsClasses.h"

#include "SerializeParticles.h"

#include "UnParticles.h"
#include "UnForceVars.h"

#endif //_INC_ParticleSystems

//-----------------------------------------------------------------------------
//	end of ParticleSystems.h
//-----------------------------------------------------------------------------
