/*=============================================================================
	UnForceVars.h: ForceVar class definitions.
	Copyright (C) 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron R Leiby
=============================================================================*/

#include "ParticleSystems.h"
#include "SmallObj.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

inline DWORD GetTypeHash( const class UForceVars* A )
{
	return *(DWORD*)&A;
}

//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API forcevarsClass : public Loki::SmallObject
{
public:
	forcevarsClass(forcevarsClass* _Super,const TCHAR* _Name)
		:Super(_Super),Name(_Name)
	{
		// add to the global class list.
		NOTE(debugf(TEXT("Creating class: %s"),GetName()));
		ClassList = GetClassList();
		GetClassList() = this;
		NOTE(for(forcevarsClass* C=forcevarsClass::GetClassList();C;C=C->NextClass()) debugf(TEXT("%s"),C->GetName()));
	}
	forcevarsClass* GetSuper(){ return Super; }
	const TCHAR* GetName(){ return Name; }
	forcevarsClass* NextClass(){ return ClassList; }
	virtual UForceVars* CreateInstance()=0;
	static forcevarsClass* &GetClassList(){ static forcevarsClass* ClassList=NULL; return ClassList; }
private:
	forcevarsClass* Super;
	const TCHAR* Name;
	forcevarsClass* ClassList;
};

//------------------------------------------------------------------------------
#define DECLARE_FORCEVARS_BASECLASS_DEF( TClass, TSuperClass, TuiSuperClass, TCreateInstance ) \
public: \
	typedef TSuperClass Super;\
	TClass(){} /*uninitialized*/ \
	virtual ~TClass(){} \
	class TClass##Class : public forcevarsClass { \
		public: TClass##Class(forcevarsClass* C,const TCHAR* N):forcevarsClass(C,N){} \
		public: UForceVars* CreateInstance(){ return TCreateInstance; } }; \
	static forcevarsClass* StaticClass(){ static forcevarsClass* MyClass = new TClass##Class(TuiSuperClass,TEXT(#TClass)); return MyClass; } \
	virtual forcevarsClass* GetClass(){ return TClass::StaticClass(); } \
	virtual size_t GetSize(){ return sizeof(TClass); } \

#define DECLARE_FORCEVARS_BASECLASS( TClass, TSuperClass ) \
	DECLARE_FORCEVARS_BASECLASS_DEF( TClass, TSuperClass, NULL, NEW_FORCEVARS(TClass) ) \

#define DECLARE_FORCEVARS_CLASS( TClass, TSuperClass ) \
	DECLARE_FORCEVARS_BASECLASS_DEF( TClass, TSuperClass, TSuperClass::StaticClass(), NEW_FORCEVARS(TClass) ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return (FArchiveParticles&)Ar << *(UForceVars**)&Res; } \

#define DECLARE_FORCEVARS_ABSTRACT_CLASS( TClass, TSuperClass ) \
	DECLARE_FORCEVARS_BASECLASS_DEF( TClass, TSuperClass, TSuperClass::StaticClass(), NULL ) \

// force initialization of static classes at startup (ensures all classes are in the global class list).
#define IMPLEMENT_FORCEVARS_CLASS( TClass ) \
	static forcevarsClass* Static##TClass = TClass::StaticClass(); \


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UForceVars : public Loki::SmallObject
{
public:
    DECLARE_FORCEVARS_BASECLASS(UForceVars,Loki::SmallObject)
	FString GetClassName(){ return GetClass()->GetName(); }
	UBOOL IsA(forcevarsClass* Class)
	{
		for(forcevarsClass* C=GetClass();C;C=C->GetSuper())
			if(C==Class)
				return 1;
		return 0;
	}
	friend FArchive &operator<<( FArchive& Ar, UForceVars* &V )
		{ return (FArchiveParticles&)Ar << V; }
	virtual void Serialize( FArchive& Ar ){}
	virtual UForceVars* Duplicate( UObject* NewOuter );
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UWindForceVars : public UForceVars
{
public:
    FVector LastWindspeed;
    DECLARE_FORCEVARS_CLASS(UWindForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << LastWindspeed;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UVariablePerParticleGravityVars : public UForceVars
{
public:
    FLOAT Gravity;
    FLOAT EndGravity;
    FLOAT ChangeRate;
    DECLARE_FORCEVARS_CLASS(UVariablePerParticleGravityVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Gravity << EndGravity << ChangeRate;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UTriangleForceVars : public UForceVars
{
public:
    FVector Velocity;
    FCoords CenterMass;
    FVector P1Offset;
    FVector P2Offset;
    FVector P3Offset;
    FVector P1LastVelocity;
    FVector P2LastVelocity;
    FVector P3LastVelocity;
    DECLARE_FORCEVARS_CLASS(UTriangleForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Velocity << CenterMass << P1Offset << P2Offset << P3Offset << P1LastVelocity << P2LastVelocity << P3LastVelocity;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API USpringForceVars : public UForceVars
{
public:
    FVector SpringForce;
    FVector PivotOffset;
    DECLARE_FORCEVARS_CLASS(USpringForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << SpringForce << PivotOffset;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API URigidBodyVars : public UForceVars
{
public:
    FVector Offset;
    DECLARE_FORCEVARS_CLASS(URigidBodyVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Offset;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API URotationForceVars : public UForceVars
{
public:
    FLOAT RotationRate;
    FLOAT RotationTime;
    FLOAT RotationTimer;
    DECLARE_FORCEVARS_CLASS(URotationForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << RotationRate << RotationTime << RotationTimer;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UResizeForceVars : public UForceVars
{
public:
    FLOAT SizeRate;
    FLOAT SizeTime;
    FLOAT SizeTimer;
    DECLARE_FORCEVARS_CLASS(UResizeForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << SizeRate << SizeTime << SizeTimer;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UPerParticleGravityVars : public UForceVars
{
public:
    FLOAT Gravity;
    DECLARE_FORCEVARS_CLASS(UPerParticleGravityVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Gravity;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UPerParticleFrictionVars : public UForceVars
{
public:
    FLOAT Friction;
    DECLARE_FORCEVARS_CLASS(UPerParticleFrictionVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Friction;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UParticleSalamanderForceVars : public UForceVars
{
public:
	FLOAT Volume;			// Number of particles emitted per second.
	FLOAT ParticleTimer;	// Used internally to collect "emission residue" <-- fancy term for rounding errors between ticks.
    DECLARE_FORCEVARS_CLASS(UParticleSalamanderForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Volume << ParticleTimer;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UOscillatingPerParticleForceVars : public UForceVars
{
public:
    FVector Force;
    FVector Period;
    FVector CurrentTime;
    FVector LastAddition;
    DECLARE_FORCEVARS_CLASS(UOscillatingPerParticleForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Force << Period << CurrentTime << LastAddition;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UMultiTemplateForceVars : public UForceVars
{
public:
    TArrayNoInit<class UParticle*> Attached;
    TArrayNoInit<FVector> Offset;
    DECLARE_FORCEVARS_CLASS(UMultiTemplateForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Attached << Offset;
	}
	virtual UForceVars* Duplicate( UObject* NewOuter )
	{
		UMultiTemplateForceVars* NewForceVars = (UMultiTemplateForceVars*)Super::Duplicate( NewOuter );

		ZERO_OUT(NewForceVars->Attached);
		NewForceVars->Attached.Add(Attached.Num());
		FOR_ALL(Attached)
			NewForceVars->Attached(i) = Attached(i)->Duplicate( NewOuter );

		ZERO_OUT(NewForceVars->Offset);
		NewForceVars->Offset.Add(Offset.Num());
		FOR_ALL(Offset)
			NewForceVars->Offset(i) = Offset(i);

		return NewForceVars;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UMoveZigVars : public UForceVars
{
public:
    BITFIELD bAffected:1 GCC_PACK(4);
    DECLARE_FORCEVARS_CLASS(UMoveZigVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		SERIALIZE_BOOL(bAffected)
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API ULimitedResizeForceVars : public UForceVars
{
public:
    FLOAT SizeRate;
    DECLARE_FORCEVARS_CLASS(ULimitedResizeForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << SizeRate;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UInertiaForceVars : public UForceVars
{
public:
    FVector LastVelocity;
    DECLARE_FORCEVARS_CLASS(UInertiaForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << LastVelocity;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UGeometryColliderExVars : public UForceVars
{
public:
    INT Bounces;
    DECLARE_FORCEVARS_CLASS(UGeometryColliderExVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Bounces;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UFadeOutForceVars : public UForceVars
{
public:
    FLOAT InitialAlpha;
    FLOAT FadeTime;
    DECLARE_FORCEVARS_CLASS(UFadeOutForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << InitialAlpha << FadeTime;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UFadeInForceVars : public UForceVars
{
public:
    FLOAT FadeTime;
    FLOAT InitialFadeTime;
    FLOAT FinalAlpha;
    DECLARE_FORCEVARS_CLASS(UFadeInForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << FadeTime << InitialFadeTime << FinalAlpha;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UFadeForceVars : public UForceVars
{
public:
    FLOAT AlphaRate;
    FLOAT AlphaTime;
    FLOAT AlphaTimer;
    DECLARE_FORCEVARS_CLASS(UFadeForceVars,UForceVars)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << AlphaRate << AlphaTime << AlphaTimer;
	}
};

#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif
