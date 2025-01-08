/*=============================================================================
	UnParticles.h: Particle class definitions.
	Copyright (C) 2002 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Aaron R Leiby
=============================================================================*/

#include "ParticleSystems.h"
#include "SmallObj.h"

#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,4)
#endif

inline DWORD GetTypeHash( const class UParticle* A )
{
	return *(DWORD*)&A;
}

//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API particleClass : public Loki::SmallObject
{
public:
	particleClass(particleClass* _Super,const TCHAR* _Name)
		:Super(_Super),Name(_Name)
	{
		// add to the global class list.
		NOTE(debugf(TEXT("Creating class: %s"),GetName()));
		ClassList = GetClassList();
		GetClassList() = this;
		NOTE(for(particleClass* C=particleClass::GetClassList();C;C=C->NextClass()) debugf(TEXT("%s"),C->GetName()));
	}
	particleClass* GetSuper(){ return Super; }
	const TCHAR* GetName(){ return Name; }
	particleClass* NextClass(){ return ClassList; }
	virtual UParticle* CreateInstance()=0;
	static particleClass* &GetClassList(){ static particleClass* ClassList=NULL; return ClassList; }
private:
	particleClass* Super;
	const TCHAR* Name;
	particleClass* ClassList;
};

//------------------------------------------------------------------------------
#define DECLARE_PARTICLE_BASECLASS_DEF( TClass, TSuperClass, TuiSuperClass, TCreateInstance ) \
public: \
	typedef TSuperClass Super;\
	TClass(){} /*uninitialized*/ \
	virtual ~TClass(){} \
	class TClass##Class : public particleClass { \
		public: TClass##Class(particleClass* C,const TCHAR* N):particleClass(C,N){} \
		public: UParticle* CreateInstance(){ return TCreateInstance; } }; \
	static particleClass* StaticClass(){ static particleClass* MyClass = new TClass##Class(TuiSuperClass,TEXT(#TClass)); return MyClass; } \
	virtual particleClass* GetClass(){ return TClass::StaticClass(); } \
	virtual size_t GetSize(){ return sizeof(TClass); } \

#define DECLARE_PARTICLE_BASECLASS( TClass, TSuperClass ) \
	DECLARE_PARTICLE_BASECLASS_DEF( TClass, TSuperClass, NULL, NEW_PARTICLE(TClass) )

#define DECLARE_PARTICLE_CLASS( TClass, TSuperClass ) \
	DECLARE_PARTICLE_BASECLASS_DEF( TClass, TSuperClass, TSuperClass::StaticClass(), NEW_PARTICLE(TClass) ) \
	friend FArchive &operator<<( FArchive& Ar, TClass*& Res ) \
		{ return (FArchiveParticles&)Ar << *(UParticle**)&Res; } \

#define DECLARE_PARTICLE_ABSTRACT_CLASS( TClass, TSuperClass ) \
	DECLARE_PARTICLE_BASECLASS_DEF( TClass, TSuperClass, TSuperClass::StaticClass(), NULL ) \

// force initialization of static classes at startup (ensures all classes are in the global class list).
#define IMPLEMENT_PARTICLE_CLASS( TClass ) \
	static particleClass* Static##TClass = TClass::StaticClass(); \


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UParticle : public Loki::SmallObject
{
public:
    FVector Location;
    FVector LastLocation;
    FVector Velocity;
    FLOAT LifeSpan;
    FLOAT StasisTimer;
	TMap< UForce*, UForceVars* > AdditionalVars;
	UBOOL bChild;	//HACK to support MultiTemplateForces with shorter lifespans.
    DECLARE_PARTICLE_BASECLASS(UParticle,Loki::SmallObject)
	FString GetClassName(){ return GetClass()->GetName(); }
	UBOOL IsA(particleClass* Class)
	{
		for(particleClass* C=GetClass();C;C=C->GetSuper())
			if(C==Class)
				return 1;
		return 0;
	}
	friend FArchive &operator<<( FArchive& Ar, UParticle* &P )
		{ return (FArchiveParticles&)Ar << P; }
	virtual void Serialize( FArchive& Ar )
	{
		Ar << Location << LastLocation << Velocity << LifeSpan << StasisTimer << AdditionalVars;
//!!MERGE		if( Ar.LicenseeVer() >= LVERSION_PARTICLE_CHILD )
			Ar << bChild;
	}
	virtual void Destroy();
	virtual UParticle* Duplicate( UObject* NewOuter );
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API USpriteParticle : public UParticle
{
public:
    INT SpriteIndex;
    INT CoronaIndex;
    FColor Color;
    FLOAT Alpha;
    FLOAT Size;
    FLOAT InitialLifeSpan;
	FLOAT LastTraceTime;
	UBOOL LastTraceResult;
    DECLARE_PARTICLE_CLASS(USpriteParticle,UParticle)

	INT GetSpriteIndex( USpriteParticleTemplate* Owner );
	INT GetCoronaIndex( USpriteParticleTemplate* Owner );

	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << SpriteIndex << CoronaIndex << Color << Alpha << Size << InitialLifeSpan;
//!!MERGE		if( Ar.LicenseeVer() >= LVERSION_PARTICLE_CORONA_OPTIMIZATION )
			Ar << LastTraceTime << LastTraceResult;

		// These should never have been serialized -- since it Level.TimeSeconds may get really large while editing a level.
		LastTraceTime=0.f;
		LastTraceResult=1;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UViewVelocityScaledParticle : public USpriteParticle
{
public:
    FLOAT ScaleFactor;
    FLOAT StretchOffset;
    DECLARE_PARTICLE_CLASS(UViewVelocityScaledParticle,USpriteParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << ScaleFactor << StretchOffset;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API USprite3DParticle : public USpriteParticle
{
public:
    DECLARE_PARTICLE_CLASS(USprite3DParticle,USpriteParticle)
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API URotatingParticle : public USpriteParticle
{
public:
    FLOAT Angle;
    DECLARE_PARTICLE_CLASS(URotatingParticle,USpriteParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Angle;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UParticleDecal : public USpriteParticle
{
public:
    FVector Offsets[4];
#if 0 //ProjDecals
	FProjectorRenderInfo* RenderInfo;
#endif
    DECLARE_PARTICLE_CLASS(UParticleDecal,USpriteParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Offsets[0] << Offsets[1] << Offsets[2] << Offsets[3];
#if 0 //ProjDecals
		if( Ar.LicenseeVer() >= LVERSION_PARTICLE_PROJECTORS )
			Ar << RenderInfo;
#endif
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UTriParticle : public UParticle
{
public:
    class UMaterial* Texture;
    FLOAT Alpha;
    FLOAT P0Dist;
    FLOAT P1Dist;
    FLOAT P2Dist;
    FLOAT P01Ang;
    FLOAT P12Ang;
    FLOAT P20Ang;
    class UParticle* P1;
    class UParticle* P2;
    DECLARE_PARTICLE_CLASS(UTriParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Texture << Alpha << P0Dist << P1Dist << P2Dist << P01Ang << P12Ang << P20Ang << P1 << P2;
	}
	virtual UParticle* Duplicate( UObject* NewOuter )
	{
		UTriParticle* NewParticle = (UTriParticle*)Super::Duplicate( NewOuter );
		if(P1) NewParticle->P1 = P1->Duplicate( NewOuter );
		if(P2) NewParticle->P2 = P2->Duplicate( NewOuter );
		return NewParticle;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UTriangleParticle : public UParticle
{
public:
    class UMaterial* Texture;
    FLOAT Alpha;
    class UParticle* P1;
    class UParticle* P2;
    DECLARE_PARTICLE_CLASS(UTriangleParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Texture << Alpha << P1 << P2;
	}
	virtual UParticle* Duplicate( UObject* NewOuter )
	{
		UTriangleParticle* NewParticle = (UTriangleParticle*)Super::Duplicate( NewOuter );
		if(P1) NewParticle->P1 = P1->Duplicate( NewOuter );
		if(P2) NewParticle->P2 = P2->Duplicate( NewOuter );
		return NewParticle;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UTexturedStreakParticle : public UParticle
{
public:
    FLOAT Alpha;
    FLOAT WidthStart;
    FLOAT WidthEnd;
    TArrayNoInit<FVector> PrevLocations;
    DECLARE_PARTICLE_CLASS(UTexturedStreakParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Alpha << WidthStart << WidthEnd << PrevLocations;
	}
	virtual UParticle* Duplicate( UObject* NewOuter )
	{
		UTexturedStreakParticle* NewParticle = (UTexturedStreakParticle*)Super::Duplicate( NewOuter );

		ZERO_OUT(NewParticle->PrevLocations);
		NewParticle->PrevLocations.Add(PrevLocations.Num());
		FOR_ALL(PrevLocations)
			NewParticle->PrevLocations(i) = PrevLocations(i);

		return NewParticle;
	}
	virtual void Destroy()
	{
		PrevLocations.Empty();
		Super::Destroy();
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UTarpParticle : public UParticle
{
public:
    FLOAT UTex;
    FLOAT VTex;
    FVector Normal;
    class UTarpParticle* NextUParticle;
    class UTarpParticle* NextVParticle;
    class UAnchorForce* Anchor;
    TArrayNoInit<class USlipForce*> SlipForces;
    DECLARE_PARTICLE_CLASS(UTarpParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << UTex << VTex << Normal << NextUParticle << NextVParticle << Anchor << SlipForces;
	}
	virtual void Destroy()
	{
		SlipForces.Empty();
		Super::Destroy();
	}
	virtual UParticle* Duplicate( UObject* NewOuter )
	{
		UTarpParticle* NewParticle = (UTarpParticle*)Super::Duplicate( NewOuter );

		if(NextUParticle)	NewParticle->NextUParticle	= (UTarpParticle*)NextUParticle->Duplicate( NewOuter );
		if(NextVParticle)	NewParticle->NextVParticle	= (UTarpParticle*)NextVParticle->Duplicate( NewOuter );
		if(Anchor)			NewParticle->Anchor			= (UAnchorForce*)Anchor->Duplicate( NewOuter );

		ZERO_OUT(NewParticle->SlipForces);
		NewParticle->SlipForces.Add(SlipForces.Num());
		FOR_ALL(SlipForces)
			NewParticle->SlipForces(i) = (USlipForce*)SlipForces(i)->Duplicate( NewOuter );

		return NewParticle;
	}
};

//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UStreakParticle : public UParticle
{
public:
    FColor Color;
    FLOAT Alpha;
    TArrayNoInit<FVector> PrevLocations;
    DECLARE_PARTICLE_CLASS(UStreakParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Color << Alpha << PrevLocations;
	}
	virtual void Destroy()
	{
		PrevLocations.Empty();
		Super::Destroy();
	}
	virtual UParticle* Duplicate( UObject* NewOuter )
	{
		UStreakParticle* NewParticle = (UStreakParticle*)Super::Duplicate( NewOuter );

		ZERO_OUT(NewParticle->PrevLocations);
		NewParticle->PrevLocations.Add(PrevLocations.Num());
		FOR_ALL(PrevLocations)
			NewParticle->PrevLocations(i) = PrevLocations(i);

		return NewParticle;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UStrandParticle : public UParticle
{
public:
    FColor Color;
    BITFIELD bRoot:1 GCC_PACK(4);
    class UStrandParticle* NextStrandParticle GCC_PACK(4);
    DECLARE_PARTICLE_CLASS(UStrandParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Color;
		SERIALIZE_BOOL(bRoot)
		Ar << NextStrandParticle;
	}
	virtual UParticle* Duplicate( UObject* NewOuter )
	{
		UStrandParticle* NewParticle = (UStrandParticle*)Super::Duplicate( NewOuter );
		if(NextStrandParticle) NewParticle->NextStrandParticle = (UStrandParticle*)NextStrandParticle->Duplicate( NewOuter );
		return NewParticle;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API URecursiveParticle : public UParticle
{
public:
    class AParticleGenerator* Generator;
    DECLARE_PARTICLE_CLASS(URecursiveParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Generator;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API UPointParticle : public UParticle
{
public:
    FColor Color;
    FLOAT Alpha;
    DECLARE_PARTICLE_CLASS(UPointParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << Color << Alpha;
	}
};


//------------------------------------------------------------------------------
class PARTICLESYSTEMS_API ULineParticle : public UParticle
{
public:
    FVector EndPoint;
    FColor Color;
    BITFIELD bDirected:1 GCC_PACK(4);
    FLOAT ArrowOffset GCC_PACK(4);
    FLOAT ArrowWidth;
    FLOAT ArrowHeight;
    FColor ArrowColor;
    DECLARE_PARTICLE_CLASS(ULineParticle,UParticle)
	virtual void Serialize( FArchive& Ar )
	{
		Super::Serialize(Ar);
		Ar << EndPoint << Color;
		SERIALIZE_BOOL(bDirected)
		Ar << ArrowOffset << ArrowWidth << ArrowHeight << ArrowColor;
	}
};


#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

