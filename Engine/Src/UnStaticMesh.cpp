/*=============================================================================
	UnStaticMesh.cpp: Static mesh class implementation.
	Copyright 1997-2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRenderPrivate.h"

IMPLEMENT_CLASS(UStaticMesh);
IMPLEMENT_CLASS(UStaticMeshInstance);
IMPLEMENT_CLASS(AStaticMeshActor);

//
//	UStaticMesh::UStaticMesh
//

UStaticMesh::UStaticMesh()
{
}

//
//	UStaticMesh::StaticConstructor
//

void UStaticMesh::StaticConstructor()
{
	guard(UStaticMesh::StaticConstructor);

	new(GetClass()->HideCategories) FName(NAME_Object);

	new(GetClass(),TEXT("UseSimpleLineCollision"),RF_Public)	UBoolProperty(CPP_PROPERTY(UseSimpleLineCollision),TEXT(""),CPF_Edit);
	new(GetClass(),TEXT("UseSimpleBoxCollision"),RF_Public)		UBoolProperty(CPP_PROPERTY(UseSimpleBoxCollision),TEXT(""),CPF_Edit);
	new(GetClass(),TEXT("UseSimpleKarmaCollision"),RF_Public)	UBoolProperty(CPP_PROPERTY(UseSimpleKarmaCollision),TEXT(""),CPF_Edit);
	new(GetClass(),TEXT("UseVertexColor"),RF_Public)			UBoolProperty(CPP_PROPERTY(UseVertexColor),TEXT(""),CPF_Edit);

	FArchive ArDummy;
	UStruct* MaterialStruct = new(GetClass(),TEXT("StaticMeshMaterial")) UStruct(NULL);
	new(MaterialStruct,TEXT("Material"),RF_Public)			UObjectProperty(EC_CppProperty,STRUCT_OFFSET(FStaticMeshMaterial,Material),TEXT(""),CPF_Edit,UMaterial::StaticClass());
	new(MaterialStruct,TEXT("EnableCollision"),RF_Public)	UBoolProperty(EC_CppProperty,STRUCT_OFFSET(FStaticMeshMaterial,EnableCollision),TEXT(""),CPF_Edit);
	MaterialStruct->SetPropertiesSize(sizeof(FStaticMeshMaterial));
	MaterialStruct->Link(ArDummy,NULL);

	UArrayProperty*	A	= new(GetClass(),TEXT("Materials"),RF_Public)	UArrayProperty(CPP_PROPERTY(Materials),TEXT(""),CPF_Edit | CPF_EditConstArray);
	A->Inner			= new(A,TEXT("StructProperty0"),RF_Public)		UStructProperty(EC_CppProperty,0,TEXT(""),CPF_Edit,MaterialStruct);

	UseSimpleLineCollision = 0;
	UseSimpleBoxCollision = 1;
	UseSimpleKarmaCollision = 1;
	UseVertexColor = 0;

	unguard;
}

//
//	UStaticMesh::PostEditChange
//

void UStaticMesh::PostEditChange()
{
	guard(UStaticMesh::PostEditChange);

	Super::PostEditChange();

	// Check if we need to rebuild the static mesh.

	UBOOL Rebuild = 0;

	for(INT MaterialIndex = 0;MaterialIndex < Materials.Num();MaterialIndex++)
	{
		FStaticMeshMaterial*	Material = &Materials(MaterialIndex);

		if(Material->EnableCollision != Material->OldEnableCollision)
		{
			Material->OldEnableCollision = Material->EnableCollision;
			Rebuild = 1;
			break;
		}
		if(Materials(MaterialIndex).Material && Materials(MaterialIndex).Material->IsA(UHardwareShader::StaticClass()))
		{
			Rebuild = 1;
		}
	}

	if(Rebuild)
		Build();

	unguard;
}

//
// UStaticMesh::Destroy
//

void UStaticMesh::Destroy()
{
	guard(UStaticMesh::Destroy);

#ifdef WITH_KARMA
	KTermStaticMeshCollision(this);
#endif

	// Destroy any inner's of this static mesh. This should only happen in the editor i think.
	if(KPhysicsProps && !PurgingGarbage())
	{
		delete KPhysicsProps;
		KPhysicsProps = NULL;
	}

	Super::Destroy();
	unguard;
}

//
//	UStaticMesh::Rename
//
void UStaticMesh::Rename( const TCHAR* InName, UObject* NewOuter )
{
	guard(UStaticMesh::Rename);

	// Also rename the collision mode (if present), if the outer has changed.
    if( NewOuter && CollisionModel && CollisionModel->GetOuter() == GetOuter() )
		CollisionModel->Rename( CollisionModel->GetName(), NewOuter );

	// Rename the static mesh
    Super::Rename( InName, NewOuter );

	unguard;
} 

//
//	FStaticMeshCollisionPrimitive
//

class FStaticMeshCollisionPrimitive
{
public:

	TArray<INT>		Children;
	FBox			BoundingBox;
	FPlane			Plane;
	FVector			Vertices[3];
	UMaterial*		Material;

	FStaticMeshCollisionPrimitive()
	{
		Material = NULL;
	}

	~FStaticMeshCollisionPrimitive()
	{
	}

	friend FArchive& operator<<(FArchive& Ar,FStaticMeshCollisionPrimitive& P)
	{
		Ar	<< P.Children
			<< P.BoundingBox;

		if(!P.Children.Num())
		{
			FPlane	SidePlanes[3];

			Ar	<< P.Plane;

			if(Ar.Ver() < 83)
			{
				Ar 	<< SidePlanes[0]
					<< SidePlanes[1]
					<< SidePlanes[2];
			}

			if(Ar.Ver() >= 80)
			{
				Ar	<< P.Vertices[0]
					<< P.Vertices[1]
					<< P.Vertices[2];
			}
			else
			{
				FIntersectPlanes3(P.Vertices[0],P.Plane,SidePlanes[2],SidePlanes[0]);
				FIntersectPlanes3(P.Vertices[1],P.Plane,SidePlanes[0],SidePlanes[1]);
				FIntersectPlanes3(P.Vertices[2],P.Plane,SidePlanes[1],SidePlanes[2]);
			}

			if(Ar.Ver() >= 95)
				Ar << P.Material;
		}

		return Ar;
	}
};

//
//	FStaticMeshOldVertex
//

struct FStaticMeshOldVertex
{
	FVector	Position,
			Normal;
	FLOAT	U,
			V;

	// Serializer.

	friend FArchive& operator<<(FArchive& Ar,FStaticMeshOldVertex& V)
	{
		Ar	<< V.Position
			<< V.Normal
			<< V.U
			<< V.V;

		return Ar;
	}
};

//
//	FStaticMeshOldCollisionTriangle
//

struct FStaticMeshOldCollisionTriangle
{
public:

	FPlane		Plane;
	FPlane		SidePlanes[3];
	INT			VertexIndices[3];
	INT			MaterialIndex;
	INT			CollisionTag;

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar,FStaticMeshOldCollisionTriangle& T)
	{
		return Ar	<< T.Plane
					<< T.SidePlanes[0]
					<< T.SidePlanes[1]
					<< T.SidePlanes[2]
					<< AR_INDEX(T.VertexIndices[0])
					<< AR_INDEX(T.VertexIndices[1])
					<< AR_INDEX(T.VertexIndices[2])
					<< AR_INDEX(T.MaterialIndex);
	}
};


//
//	FStaticMeshOldCollisionNode
//

struct FStaticMeshOldCollisionNode
{
public:

	INT		TriangleIndex,
			CoplanarIndex,
			ChildIndices[2];

	FBox	BoundingBox;

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar,FStaticMeshOldCollisionNode& N)
	{
		return Ar	<< AR_INDEX(N.TriangleIndex)
					<< AR_INDEX(N.CoplanarIndex)
					<< AR_INDEX(N.ChildIndices[0])
					<< AR_INDEX(N.ChildIndices[1])
					<< N.BoundingBox;
	}
};


//
//	FStaticMeshTriangle serializer.
//

FArchive& operator<<(FArchive& Ar,FStaticMeshTriangle& T)
{
	if(Ar.Ver() < 111)
	{
		FStaticMeshOldVertex	TempVertices[3];

		Ar	<< TempVertices[0]
			<< TempVertices[1]
			<< TempVertices[2];

		T.Vertices[0] = TempVertices[0].Position;
		T.UVs[0][0].U = TempVertices[0].U;
		T.UVs[0][0].V = TempVertices[0].V;
		T.Colors[0] = FColor(255,255,255,255);

		T.Vertices[1] = TempVertices[1].Position;
		T.UVs[1][0].U = TempVertices[1].U;
		T.UVs[1][0].V = TempVertices[1].V;
		T.Colors[1] = FColor(255,255,255,255);

		T.Vertices[2] = TempVertices[2].Position;
		T.UVs[2][0].U = TempVertices[2].U;
		T.UVs[2][0].V = TempVertices[2].V;
		T.Colors[2] = FColor(255,255,255,255);

		T.NumUVs = 1;
	}
	else
	{
		Ar	<< T.Vertices[0]
			<< T.Vertices[1]
			<< T.Vertices[2]
			<< T.NumUVs;

		for(INT UVIndex = 0;UVIndex < T.NumUVs;UVIndex++)
			Ar	<< T.UVs[0][UVIndex]
				<< T.UVs[1][UVIndex]
				<< T.UVs[2][UVIndex];

		Ar	<< T.Colors[0]
			<< T.Colors[1]
			<< T.Colors[2];
	}

	if(Ar.Ver() < 112)
	{
		FVector	Normal;
		Ar << Normal;

		Ar << T.LegacyMaterial << T.LegacyPolyFlags;
	}
	else
		Ar << T.MaterialIndex;

	Ar << T.SmoothingMask;
	
	return Ar;
}

//
//	UStaticMesh::Serialize
//

void UStaticMesh::Serialize(FArchive& Ar)
{
	guard(UStaticMesh::Serialize);

	if(Ar.Ver() < 85)
		UObject::Serialize(Ar);
	else
		Super::Serialize(Ar);

	if(Ar.Ver() < 92)
	{
		Ar	<< LegacyVertexBuffer
			<< LegacyIndexBuffer;
	}

	Ar	<< Sections
		<< BoundingBox;

	if(Ar.Ver() < 74)
	{
		UModel*	TempModel = NULL;
		Ar << TempModel;
	}

	if(Ar.Ver() < 92)
	{
		TArray<FStaticMeshLightInfo>	LightInfos;
		FCoords							RaytraceCoords;

		Ar	<< LightInfos
			<< RaytraceCoords;
	}

	if(Ar.Ver() >= 74)
	{
		if(Ar.Ver() < 112)
		{
			TArray<FStaticMeshCollisionPrimitive>	CollisionPrimitives;

			Ar << CollisionPrimitives;
		}

		if(Ar.Ver() < 92)
		{
			UBOOL	RefreshLighting;

			Ar << RefreshLighting;
		}
	}

	if(Ar.Ver() >= 112)
	{
		Ar	<< VertexStream
			<< ColorStream
			<< AlphaStream
			<< UVStreams
			<< IndexBuffer
			<< WireframeIndexBuffer
			<< CollisionModel;

		if( Ar.Ver() < 126 )
		{
			TArray<FStaticMeshOldCollisionTriangle> OldCollisionTriangles;
			TArray<FStaticMeshOldCollisionNode> OldCollisionNodes;
				
			Ar	<< OldCollisionTriangles
				<< OldCollisionNodes;
		}
		else
		{
			Ar	<< CollisionTriangles
				<< CollisionNodes
				<< CompressionScale;
		}

		if(Ar.Ver() < 114)
			Ar 	<< UseSimpleLineCollision
				<< UseSimpleBoxCollision
				<< UseVertexColor;
		
		if(Ar.LicenseeVer() >= 0x00)	//!!merge
			Ar << TBStream;
	}

	if(Ar.Ver() >= 77)
	{
		if(Ar.Ver() < 92)
			Ar << LegacyWireframeIndexBuffer;
	}

	if(Ar.Ver() >= 79)
	{
		if(Ar.Ver() < 97)
		{
			TArray<FStaticMeshTriangle>	Temp(RawTriangles);

			Ar << Temp;

			if(Ar.IsLoading())
			{
				RawTriangles.Empty(Temp.Num());
				RawTriangles.Add(Temp.Num());
				appMemcpy(&RawTriangles(0),&Temp(0),Temp.Num() * sizeof(FStaticMeshTriangle));
			}
		}
		else
		{
			UBOOL	SavedLazyLoad = GLazyLoad;

			GLazyLoad = 1;
			Ar << RawTriangles;
			GLazyLoad = SavedLazyLoad;
		}
	}

	if(Ar.Ver() < 81)
    {
		check( Ar.IsLoading() )
		InternalVersion = INDEX_NONE;
    }
	else if( Ar.IsLoading() )
    {
		Ar << InternalVersion;
	}
    else if( Ar.IsSaving() )
    {
		InternalVersion = STATICMESH_VERSION;
		Ar << InternalVersion;
    }

	// Serialize Karma collision/mass data.
	if(Ar.Ver() >= 100)
	{
		Ar << KPhysicsProps;
	}

	// Content authentication.
	if( Ar.Ver() >= 120)
	{
		Ar << AuthenticationKey;
	}

	unguard;
}

//
//	UStaticMesh::PostLoad
//
void UStaticMesh::PostLoad()
{
	guard(UStaticMesh::PostLoad);

	Super::PostLoad();

	if(InternalVersion == INDEX_NONE)
	{
		if(!RawTriangles.Num())
			RawTriangles.Load();

		// Derive the raw triangle data from the optimized static mesh data.
		for(INT SectionIndex = 0;SectionIndex < Sections.Num();SectionIndex++)
		{
			FStaticMeshSection&	Section = Sections(SectionIndex);

			if(Section.IsStrip)
			{
				for(INT TriangleIndex = 0;TriangleIndex < Section.NumPrimitives;TriangleIndex++)
				{
					INT	Indices[3];

					if(TriangleIndex & 1)
					{
						Indices[0] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex + 1);
						Indices[1] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex + 0);
						Indices[2] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex + 2);
					}
					else
					{
						Indices[0] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex + 0);
						Indices[1] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex + 1);
						Indices[2] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex + 2);
					}

					if(Indices[0] == Indices[1] || Indices[1] == Indices[2] || Indices[0] == Indices[2])
						continue;

					FStaticMeshTriangle*	Triangle = new(RawTriangles) FStaticMeshTriangle;

					Triangle->LegacyMaterial = Section.LegacyMaterial;
					Triangle->SmoothingMask = 0;
					Triangle->NumUVs = 1;

					for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
					{
						Triangle->Vertices[VertexIndex] = LegacyVertexBuffer->Vertices(Indices[VertexIndex]).Position;
						Triangle->UVs[VertexIndex][0].U = LegacyVertexBuffer->Vertices(Indices[VertexIndex]).U;
						Triangle->UVs[VertexIndex][0].V = LegacyVertexBuffer->Vertices(Indices[VertexIndex]).V;
						Triangle->Colors[VertexIndex] = FColor(255,255,255,255);
					}
				}
			}
			else
			{
				for(INT TriangleIndex = 0;TriangleIndex < Section.NumTriangles;TriangleIndex++)
				{
					INT	Indices[3];

					Indices[0] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex * 3 + 0);
					Indices[1] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex * 3 + 1);
					Indices[2] = LegacyIndexBuffer->Indices(Section.LegacyFirstIndex + TriangleIndex * 3 + 2);

					FStaticMeshTriangle*	Triangle = new(RawTriangles) FStaticMeshTriangle;

					Triangle->LegacyMaterial = Section.LegacyMaterial;
					Triangle->SmoothingMask = 0;
					Triangle->NumUVs = 1;

					for(INT VertexIndex = 0;VertexIndex < 3;VertexIndex++)
					{
						Triangle->Vertices[VertexIndex] = LegacyVertexBuffer->Vertices(Indices[VertexIndex]).Position;
						Triangle->UVs[VertexIndex][0].U = LegacyVertexBuffer->Vertices(Indices[VertexIndex]).U;
						Triangle->UVs[VertexIndex][0].V = LegacyVertexBuffer->Vertices(Indices[VertexIndex]).V;
						Triangle->Colors[VertexIndex] = FColor(255,255,255,255);
					}
				}
			}
		}
	}

	LegacyVertexBuffer = NULL;
	LegacyIndexBuffer = NULL;
	LegacyWireframeIndexBuffer = NULL;

	//!!OLDVER
	if( InternalVersion <= 6 && InternalVersion != INDEX_NONE )
	{
		// Convert polyflags to materials
		if(!RawTriangles.Num())
			RawTriangles.Load();

		DWORD LastPolyFlags = 0;
		UMaterial* LastMaterial = NULL;
		UMaterial* NewMaterial = NULL;

		for( INT i=0;i<RawTriangles.Num();i++ )
		{
			FStaticMeshTriangle& Tri = RawTriangles(i);
			if( Tri.LegacyMaterial )
			{
				if( Tri.LegacyMaterial != LastMaterial || Tri.LegacyPolyFlags != LastPolyFlags )
				{
					NewMaterial = Tri.LegacyMaterial->ConvertPolyFlagsToMaterial( Tri.LegacyMaterial, Tri.LegacyPolyFlags );
					LastMaterial = Tri.LegacyMaterial;
					LastPolyFlags = Tri.LegacyPolyFlags;
				}

				if( Tri.LegacyMaterial != NewMaterial )
				{
					Tri.LegacyMaterial = NewMaterial;
					Tri.LegacyPolyFlags = 0;
				}
			}
		}
	}

	if(InternalVersion < 8)
	{
		// Convert direct material references to material indices.

		if(!RawTriangles.Num())
			RawTriangles.Load();

		FStaticMeshMaterial*	CurrentMaterial = NULL;
		INT						CurrentMaterialIndex = INDEX_NONE;

		for(INT TriangleIndex = 0;TriangleIndex < RawTriangles.Num();TriangleIndex++)
		{
			FStaticMeshTriangle*	Triangle = &RawTriangles(TriangleIndex);

			if(!CurrentMaterial || Triangle->LegacyMaterial != CurrentMaterial->Material)
			{
				CurrentMaterialIndex = Materials.Num();
				CurrentMaterial = new(Materials) FStaticMeshMaterial(Triangle->LegacyMaterial);
			}

			Triangle->MaterialIndex = CurrentMaterialIndex;
			Triangle->LegacyMaterial = NULL;
		}
	}

	if(InternalVersion < STATICMESH_VERSION)
		Build();

	unguard;
}

//
//	UStaticMesh::GetRenderBoundingBox
//

FBox UStaticMesh::GetRenderBoundingBox(const AActor* Owner)
{
	return BoundingBox;
}

//
//	UStaticMesh::GetRenderBoundingSphere
//

FSphere UStaticMesh::GetRenderBoundingSphere(const AActor* Owner)
{
	return BoundingSphere;
}

FVector UStaticMesh::GetEncroachExtent(AActor* Owner) 
{ 
	return GetCollisionBoundingBox(Owner).GetExtent(); 
}

FVector UStaticMesh::GetEncroachCenter(AActor* Owner) 
{ 
	return GetCollisionBoundingBox(Owner).GetCenter(); 
}

//
//	UStaticMesh::Illuminate
//

void UStaticMesh::Illuminate(AActor* Owner,UBOOL ChangedOnly)
{
	guard(UStaticMesh::Illuminate);

    AMover*	MoverOwner = (Owner && Owner->IsAMover()) ? Cast<AMover>(Owner) : NULL;
	if(Owner->bStatic || (MoverOwner && !MoverOwner->bDynamicLightMover))
	{
		if(!(Owner->bHiddenEd || Owner->bHiddenEdGroup))
		{
			// Calculate the actor's bounding sphere.

			FDynamicActor*	DynamicActor = Owner->GetActorRenderData();
			FMatrix&		LocalToWorld = DynamicActor->LocalToWorld;
			FSphere&		BoundingSphere = DynamicActor->BoundingSphere;

			// Determine relevant lights.

			TArray<AActor*>	Lights;
			UBOOL			Changed = 0;

			for(INT LeafIndex = 0;LeafIndex < Owner->Leaves.Num();LeafIndex++)
			{
				FLeaf&	Leaf = Owner->XLevel->Model->Leaves(Owner->Leaves(LeafIndex));

				if(Leaf.iPermeating != INDEX_NONE)
				{
					for(INT iPermeating = Leaf.iPermeating;Owner->XLevel->Model->Lights(iPermeating);iPermeating++)
					{
						AActor*	LightActor = Owner->XLevel->Model->Lights(iPermeating);

                        if( LightActor->bHiddenEd || LightActor->bHiddenEdGroup )
                            continue;

						if(LightActor && LightActor->LightType != LT_None && LightActor->bSpecialLit == Owner->bSpecialLit && !LightActor->bDeleteMe && !LightActor->bDynamicLight)
						{
							AActor*	ExistingLight = NULL;

							for(INT LightIndex = 0;LightIndex < Lights.Num();LightIndex++)
							{
								if(Lights(LightIndex) == LightActor)
								{
									ExistingLight = Lights(LightIndex);
									break;
								}
							}

							if(!ExistingLight)
							{
								if(LightActor->LightEffect == LE_Sunlight)
								{
									if(Cast<ASkyZoneInfo>(LightActor->Region.Zone) || LightActor->Region.ZoneNumber == Leaf.iZone)
									{
										Lights.AddItem(LightActor);

										if(LightActor->bLightChanged)
											Changed = 1;
									}
								}
								else if((LightActor->Location - BoundingSphere).SizeSquared() < Square(LightActor->WorldLightRadius() + BoundingSphere.W))
								{
									Lights.AddItem(LightActor);

									if(LightActor->bLightChanged)
										Changed = 1;
								}
							}
						}
					}
				}
			}

			if(!ChangedOnly || Owner->bLightChanged || !Owner->StaticMeshInstance)
			{
				// Initialize the static mesh instance data.

				Owner->StaticMeshInstance = new(Owner->GetOuter(),NAME_None,0) UStaticMeshInstance();

				Owner->StaticMeshInstance->ColorStream.Colors.AddZeroed(VertexStream.Vertices.Num());
				Owner->StaticMeshInstance->ColorStream.Revision++;
			}
			else
			{
				// Remove old lights.

				for(INT LightIndex = 0;LightIndex < Owner->StaticMeshInstance->Lights.Num();LightIndex++)
					if(Owner->StaticMeshInstance->Lights(LightIndex).LightActor->bLightChanged)
						Owner->StaticMeshInstance->Lights.Remove(LightIndex--);

				Owner->StaticMeshInstance->ColorStream.Revision++;
			}

			// Add new lights.

			for(INT LightIndex = 0;LightIndex < Lights.Num();LightIndex++)
			{
				AActor*	LightActor = Lights(LightIndex);

				if(!ChangedOnly || Owner->bLightChanged || LightActor->bLightChanged)
				{
					FStaticMeshLightInfo*	LightInfo = new(Owner->StaticMeshInstance->Lights) FStaticMeshLightInfo(LightActor);
					BYTE*					BitPtr = &LightInfo->VisibilityBits(LightInfo->VisibilityBits.Add((VertexStream.Vertices.Num() + 7) / 8));
					BYTE					BitMask = 1;
					FVector					LightDirection = LightActor->Rotation.Vector();
					UBOOL					IsSunlight = LightInfo->LightActor->LightEffect == LE_Sunlight;

					for(INT VertexIndex = 0;VertexIndex < VertexStream.Vertices.Num();VertexIndex++)
					{
						FVector	SamplePoint = LocalToWorld.TransformFVector(VertexStream.Vertices(VertexIndex).Position),
								SampleNormal = LocalToWorld.TransformNormal(VertexStream.Vertices(VertexIndex).Normal).SafeNormal();

						if(!IsSunlight && ((LightActor->Location - SamplePoint) | SampleNormal) < 0.0f)
							*BitPtr &= ~BitMask;
						else if(IsSunlight && (LightDirection | SampleNormal) > 0.0f)
							*BitPtr &= ~BitMask;
						else if(!Owner->bStaticLighting)
						{
							FCheckResult	Hit(0);

							if(IsSunlight)// && Owner->XLevel->SingleLineCheck(Hit,Owner,SamplePoint - LightDirection * HALF_WORLD_MAX,SamplePoint - LightDirection * 4.0f,TRACE_ShadowCast|TRACE_Level|TRACE_Actors))
								*BitPtr |= BitMask;
							else if(!IsSunlight)
								*BitPtr |= BitMask;
							else
								*BitPtr &= ~BitMask;
						}
						else
						{
							FCheckResult	Hit(0);

							if(IsSunlight && Owner->XLevel->SingleLineCheck(Hit,NULL,SamplePoint - LightDirection * HALF_WORLD_MAX,SamplePoint - LightDirection * 4.0f,TRACE_ShadowCast|TRACE_Level|TRACE_Actors))
								*BitPtr |= BitMask;
							else if(!IsSunlight && Owner->XLevel->SingleLineCheck(Hit,NULL,SamplePoint + (LightActor->Location - SamplePoint).SafeNormal() * 4.0f,LightActor->Location,TRACE_ShadowCast|TRACE_Level|TRACE_Actors))
								*BitPtr |= BitMask;
							else
								*BitPtr &= ~BitMask;
						}

						BitMask <<= 1;

						if(!BitMask)
						{
							BitMask = 1;
							BitPtr++;
						}
					}
				}
			}

			CalculateStaticMeshLighting( this, Owner->StaticMeshInstance, DynamicActor );
			Owner->bLightChanged = 0;
		}
	}
	else
		Owner->StaticMeshInstance = NULL;

	unguard;
}

//
//	UStaticMeshInstance::Serialize
//

void UStaticMeshInstance::Serialize(FArchive& Ar)
{
	guard(UStaticMeshInstance::Serialize);

	Super::Serialize(Ar);

	if(Ar.Ver() < 112)
	{
		TArray<FRawColorStream>	ColorStreams;
		Ar << ColorStreams;
	}
	else
		Ar << ColorStream;

	if(Ar.Ver() >= 110)
		Ar << Lights;

	unguard;
}
