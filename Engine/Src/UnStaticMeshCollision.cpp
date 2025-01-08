/*=============================================================================
	UnStaticMeshCollision.cpp: Static mesh collision code.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

#include "EnginePrivate.h"

//!!vogel: won't compile but is useful for reference
#define OLD_COLLISION_CODE 0

#define INDEX_NONE_COLLISION 65535

//
//	FCachedCollisionData
//

class FCachedCollisionData
{
public:

	// FCachedTriangle

	struct FCachedTriangle
	{
		UBOOL	IsCached;
		FPlane	Plane;
		DWORD	Tag;
	};

	// FCachedVertex

	struct FCachedVertex
	{
		UBOOL	IsCached;
		FVector	Vertex;
	};

	// Cached data.

	AActor*				Owner;
	UStaticMesh*		StaticMesh;

	FMatrix				LocalToWorld,
						WorldToLocal;
	FLOAT				Determinant;

	FCachedTriangle*	CachedTriangles;
	FCachedVertex*		CachedVertices;

	DWORD				CurrentTag;

	// CalculateMatrices

	void CalculateMatrices()
	{
		WorldToLocal = Owner->WorldToLocal();
		LocalToWorld = Owner->LocalToWorld();
		Determinant = LocalToWorld.Determinant();
	}

	// Constructor.

	FCachedCollisionData(AActor* InOwner,UStaticMesh* InStaticMesh)
	{
		guard(FCachedCollisionData::FCachedCollisionData);

		Owner = InOwner;
		StaticMesh = InStaticMesh;

		CalculateMatrices();

		CachedTriangles = (FCachedTriangle*) &(this[1]);
		CachedVertices = (FCachedVertex*) &(CachedTriangles[StaticMesh->CollisionTriangles.Num()]);

		CurrentTag = 0;

		for(INT TriangleIndex = 0;TriangleIndex < StaticMesh->CollisionTriangles.Num();TriangleIndex++)
		{
			CachedTriangles[TriangleIndex].IsCached = 0;
			CachedTriangles[TriangleIndex].Tag = 0;
		}

		for(INT VertexIndex = 0;VertexIndex < StaticMesh->VertexStream.Vertices.Num();VertexIndex++)
			CachedVertices[VertexIndex].IsCached = 0;

		unguard;
	}

	// GetCachedPlane

	FORCEINLINE FPlane& GetCachedPlane(const INT TriangleIndex)
	{
		FCachedTriangle&	CachedTriangle = CachedTriangles[TriangleIndex];

		if(!CachedTriangle.IsCached)
		{
			FStaticMeshCollisionTriangle*	Triangle = &StaticMesh->CollisionTriangles(TriangleIndex);

			CachedTriangle.IsCached = Owner->bStatic;
			CachedTriangle.Plane = FPlane(
									GetCachedVertex(Triangle->VertexIndices[2]),
									GetCachedVertex(Triangle->VertexIndices[1]),
									GetCachedVertex(Triangle->VertexIndices[0])
									);

			if(Determinant < 0.0f)
				CachedTriangle.Plane = CachedTriangle.Plane.Flip();
		}

		return CachedTriangle.Plane;
	}

	// GetCachedTag

	FORCEINLINE DWORD& GetCachedTag(const INT TriangleIndex)
	{
		return CachedTriangles[TriangleIndex].Tag;
	}

	// GetCachedVertex

	FORCEINLINE FVector& GetCachedVertex(const INT VertexIndex)
	{
		FCachedVertex&	CachedVertex = CachedVertices[VertexIndex];

		if(!CachedVertex.IsCached)
		{
			CachedVertex.IsCached = Owner->bStatic;
			CachedVertex.Vertex = LocalToWorld.TransformFVector(StaticMesh->VertexStream.Vertices(VertexIndex).Position);
		}

		return CachedVertex.Vertex;
	}

	// CalculateSize

	static INT CalculateSize(UStaticMesh* StaticMesh)
	{
		return	sizeof(FCachedCollisionData) +
				sizeof(FCachedTriangle)	*		StaticMesh->CollisionTriangles.Num() +
				sizeof(FCachedVertex) *			StaticMesh->VertexStream.Vertices.Num();
	}

	// In-place allocator.

	void* operator new(size_t Size,EInternal* Mem)
	{
		return (void*) Mem;
	}
};

//
//	FCollisionCheck
//

class FCollisionCheck
{
public:

	FCheckResult*			Result;
	AActor*					Owner;
	UStaticMesh*			StaticMesh;

	// Constructor.

	FCollisionCheck(FCheckResult* InResult,AActor* InOwner,UStaticMesh* InStaticMesh)
	{
		Result = InResult;
		Owner = InOwner;
		StaticMesh = InStaticMesh;
	}
};

//
//	ClipWithBox
//

template<class T> UBOOL ClipWithBox(FBox BoundingBox,T& Clipee)
{
	if(!ClipWithPlane(FPlane(-1,0,0,-BoundingBox.Min.X),Clipee))
		return 0;

	if(!ClipWithPlane(FPlane(+1,0,0,+BoundingBox.Max.X),Clipee))
		return 0;

	if(!ClipWithPlane(FPlane(0,-1,0,-BoundingBox.Min.Y),Clipee))
		return 0;

	if(!ClipWithPlane(FPlane(0,+1,0,+BoundingBox.Max.Y),Clipee))
		return 0;

	if(!ClipWithPlane(FPlane(0,0,-1,-BoundingBox.Min.Z),Clipee))
		return 0;

	if(!ClipWithPlane(FPlane(0,0,+1,+BoundingBox.Max.Z),Clipee))
		return 0;

	return 1;
}

//
//	ClipWithEdge
//

template<class T> UBOOL ClipWithEdge(FVector I,FVector D,FVector N,FVector Axis,T& Clipee)
{
	FVector A = (FVector(Axis) ^ D).UnsafeNormal();

	if((N | A) < 0.0f)
		A *= -1.0f;

	if(!ClipWithPlane(FPlane(I,A),Clipee))
		return 0;

	return 1;
}

//
//	ClipWithTriangle
//

template<class T> UBOOL ClipWithTriangle(FCachedCollisionData* Data,FStaticMeshCollisionTriangle* Triangle,_WORD TriangleIndex,T& Clipee)
{
	FPlane	Plane = Data->GetCachedPlane(TriangleIndex);
	FVector	Vertices[3] =
	{
		Data->GetCachedVertex(Triangle->VertexIndices[0]),
		Data->GetCachedVertex(Triangle->VertexIndices[1]),
		Data->GetCachedVertex(Triangle->VertexIndices[2])
	};

	if(Data->Determinant < 0.0f)
		Exchange(Vertices[0],Vertices[2]);

	FBox	BoundingBox(0);

	BoundingBox += Vertices[0];
	BoundingBox += Vertices[1];
	BoundingBox += Vertices[2];

	if(!ClipWithBox(BoundingBox,Clipee))
		return 0;

	if(!ClipWithPlane(Plane,Clipee))
		return 0;

	if(!ClipWithPlane(Plane.Flip(),Clipee))
		return 0;

	for(INT SideIndex = 0;SideIndex < 3;SideIndex++)
	{
		FVector	I = Vertices[SideIndex],
				D = Vertices[(SideIndex + 1) % 3] - I,
				N = Plane ^ D;

		if((D.Y != 0.0f || D.Z != 0.0f) && !ClipWithEdge(I,D,N,FVector(1,0,0),Clipee))
			return 0;

		if((D.X != 0.0f || D.Z != 0.0f) && !ClipWithEdge(I,D,N,FVector(0,1,0),Clipee))
			return 0;

		if((D.X != 0.0f || D.Y != 0.0f) && !ClipWithEdge(I,D,N,FVector(0,0,1),Clipee))
			return 0;
	}

	return 1;
}

//
//	FClippedBox
//

class FClippedBox
{
public:

	FVector	Point,
			Extent;

	FVector	HitNormal;
	FLOAT	BestDist;

	// Constructor.

	FClippedBox(FVector InPoint,FVector InExtent,FLOAT InBestDist,FVector InNormal)
	{
		Point = InPoint;
		Extent = InExtent;

		BestDist = InBestDist;
		HitNormal = InNormal;
	}

	// ClipWithPlane

	friend UBOOL ClipWithPlane(const FPlane& Plane,FClippedBox& Clipee)
	{
		FLOAT	Dist = FBoxPushOut(Plane,Clipee.Extent) - Plane.PlaneDot(Clipee.Point);

		if(Dist < Clipee.BestDist)
		{
			Clipee.HitNormal = Plane;
			Clipee.BestDist = Dist;
		}

		return Dist > 0.0f;
	}
};

//
//	FBoxCollisionCheck
//

class FBoxCollisionCheck : public FCollisionCheck
{
public:

	FVector	Point,
			Extent;

	FVector	LocalPoint,
			LocalExtent;

	FLOAT	BestDist;

	FCacheItem*				CacheItem;
	FCachedCollisionData*	Data;

	// Constructor.

	FBoxCollisionCheck(FCheckResult* InResult,AActor* InOwner,UStaticMesh* InStaticMesh,FVector InPoint,FVector InExtent) :
		FCollisionCheck(InResult,InOwner,InStaticMesh)
	{
		guard(FBoxCollisionCheck::FBoxCollisionCheck);

		Point = InPoint;
		Extent = InExtent;

		BestDist = 100000.0f;

		QWORD	CacheId = MakeCacheID(CID_CollisionData,Owner,StaticMesh);

		Data = (FCachedCollisionData*) GCache.Get(CacheId,CacheItem);

		if(Data && (Data->Owner != Owner || Data->StaticMesh != StaticMesh))
		{
			// Unfortunately, we can't rely on items with this cache ID being for this actor.
			// Since Unreal reuses object names, occasionally an actor will be spawned with
			// the same name and static mesh as a previously destroyed actor which still has
			// collision data in the cache.  Because MakeCacheID uses the name of actor and
			// the static mesh to build a "unique" cache ID, it will generate a cache ID that
			// isn't unique.

			CacheItem->Unlock();
			GCache.Flush(CacheId);
			Data = NULL;

			STAT(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCacheFlushes)++);
		}

		if(!Data)
		{
			Data = (FCachedCollisionData*) GCache.Create(CacheId,CacheItem,FCachedCollisionData::CalculateSize(StaticMesh));
			new((EInternal*) Data) FCachedCollisionData(Owner,StaticMesh);
			STAT(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCacheMisses)++);
		}
		else if(!Owner->bStatic)
		{
			// Don't cache matrices for dynamic actors.
			Data->CalculateMatrices();
			STAT(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCacheHits)++);
		}
		else
			STAT(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCacheHits)++);

		Data->CurrentTag++;

		LocalPoint = Data->WorldToLocal.TransformFVector(Point);
		LocalExtent = FBox(-Extent,Extent).TransformBy(Data->WorldToLocal).GetExtent();

		unguard;
	}

	// Destructor.

	~FBoxCollisionCheck()
	{
		CacheItem->Unlock();
	}

	// Check

	UBOOL Check(_WORD NodeIndex)
	{
		UBOOL	Hit = 0;

		while(NodeIndex != INDEX_NONE_COLLISION)
		{
			FStaticMeshCollisionNode*		Node = &StaticMesh->CollisionNodes(NodeIndex);
			FStaticMeshCollisionTriangle*	Triangle = &StaticMesh->CollisionTriangles(Node->TriangleIndex);

			// Cull the node by bounding box.

			FBox BoundingBox;
			Node->BoundingBox.Expand( BoundingBox, StaticMesh->CompressionScale );

			BoundingBox.Min -= LocalExtent;
			BoundingBox.Max += LocalExtent;

			if(!FPointBoxIntersection(LocalPoint,BoundingBox))
				break;

#if OLD_COLLISION_CODE
			FLOAT	Dist = Triangle->Plane.PlaneDot(LocalPoint),
					PushOut = FBoxPushOut(Triangle->Plane,LocalExtent);
#else
			FStaticMeshVertex* Base = &StaticMesh->VertexStream.Vertices(0);

			FVector Verts[3] = 
			{ 
				(Base + Triangle->VertexIndices[0])->Position, 
				(Base + Triangle->VertexIndices[1])->Position,
				(Base + Triangle->VertexIndices[2])->Position
			};

			FVector Direction	= (Verts[1]-Verts[2]) ^ (Verts[0]-Verts[2]);
			FLOAT	FaceW		= Direction | Verts[0],
					Dist		= (Direction | LocalPoint) - FaceW,
					PushOut		= FBoxPushOut(Direction, LocalExtent);
#endif

			if(Dist <= -PushOut)
				NodeIndex = Node->ChildIndices[0];
			else if(Dist >= PushOut)
				NodeIndex = Node->ChildIndices[1];
			else
			{
				UBOOL	FrontFirst = Dist > 0.0f;

				// Recurse with the front children.

				if(Check(Node->ChildIndices[FrontFirst]) || Check(Node->CoplanarIndex))
					Hit = 1;

				// Check for intersection of the box with the triangle.

				FClippedBox	ClippedBox(Point,Extent,BestDist,Result->Normal);

				if(ClipWithTriangle(Data,Triangle,Node->TriangleIndex,ClippedBox))
				{
					Result->Location = Point + ClippedBox.HitNormal * ClippedBox.BestDist;
					Result->Normal = ClippedBox.HitNormal;
					Result->Item = Node->TriangleIndex;
					Result->Material = StaticMesh->GetSkin(Owner,Triangle->MaterialIndex);

					Hit = 1;
					BestDist = ClippedBox.BestDist;
				}

				// Recurse with the back children.

				NodeIndex = Node->ChildIndices[1 - (Dist > 0.0f)];
			}
		};

		return Hit;
	}
};

//
//	FClippedLine
//

class FClippedLine
{
public:

	FVector	Start,
			End,
			Extent,
			HitNormal;
	FLOAT	T0,
			T1;
	UBOOL	Hit;

	// Constructor.

	FClippedLine(FVector InStart,FVector InEnd,FVector InExtent,FLOAT InT1)
	{
		Start = InStart;
		End = InEnd;
		Extent = InExtent;
		HitNormal = FVector(0,0,0);
		Hit = 0;
		T0 = -1.0f;
		T1 = InT1;
	}

	// ClipWithPlane

	friend UBOOL ClipWithPlane(const FPlane& Plane,FClippedLine& Clipee)
	{
		FLOAT	PushOut = FBoxPushOut(Plane,Clipee.Extent),
				StartDist = Plane.PlaneDot(Clipee.Start),
				EndDist = Plane.PlaneDot(Clipee.End);

		if((StartDist - EndDist) > 0.00001f)
		{
			FLOAT	Time = (PushOut - StartDist) / (EndDist - StartDist);

			if(Time > Clipee.T0)
			{
				Clipee.T0 = Time;
				Clipee.HitNormal = Plane;
				Clipee.Hit = 1;
			}
		}
		else if((StartDist - EndDist) < -0.00001f)
		{
			FLOAT	Time = (PushOut - StartDist) / (EndDist - StartDist);

			if(Time < Clipee.T1)
				Clipee.T1 = Time;
		}
		else if(StartDist > PushOut && EndDist > PushOut)
			return 0;

		return Clipee.T0 < Clipee.T1 && Clipee.T1 > 0.0f;
	}
};

//
//	FSweptBoxCollisionCheck
//

class FSweptBoxCollisionCheck : public FBoxCollisionCheck
{
public:

	FVector	Start,
			End;

	FVector	LocalStart,
			LocalEnd,
			LocalDirection,
			LocalOneOverDirection;

	// Constructor.

	FSweptBoxCollisionCheck(FCheckResult* InResult,AActor* InOwner,UStaticMesh* InStaticMesh,FVector InStart,FVector InEnd,FVector InExtent) :
		FBoxCollisionCheck(InResult,InOwner,InStaticMesh,InStart,InExtent)
	{
		guard(FSweptBoxCollisionCheck::FSweptBoxCollisionCheck);

		Start = InStart;
		End = InEnd;

		LocalStart = LocalPoint;
		LocalEnd = Data->WorldToLocal.TransformFVector(End);
		LocalDirection = (LocalEnd - LocalStart);
		LocalOneOverDirection = FVector(1.0f / LocalDirection.X,1.0f / LocalDirection.Y,1.0f / LocalDirection.Z);

		unguard;
	}

	// Check

	UBOOL Check(_WORD NodeIndex)
	{
		UBOOL	Hit = 0;

		while(NodeIndex != INDEX_NONE_COLLISION)
		{
			FStaticMeshCollisionNode*		Node = &StaticMesh->CollisionNodes(NodeIndex);
			FStaticMeshCollisionTriangle*	Triangle = &StaticMesh->CollisionTriangles(Node->TriangleIndex);

			// Cull the node by bounding box.

			FBox BoundingBox;
			Node->BoundingBox.Expand( BoundingBox, StaticMesh->CompressionScale );

			BoundingBox.Min -= LocalExtent;
			BoundingBox.Max += LocalExtent;

			if(!FLineBoxIntersection(BoundingBox,LocalStart,LocalEnd,LocalDirection,LocalOneOverDirection))
				break;

#if OLD_COLLISION_CODE
			FPlane& TrianglePlane = Triangle->Plane;
#else
			FStaticMeshVertex* Base = &StaticMesh->VertexStream.Vertices(0);
			FVector Verts[3] = 
			{ 
				(Base + Triangle->VertexIndices[0])->Position, 
				(Base + Triangle->VertexIndices[1])->Position,
				(Base + Triangle->VertexIndices[2])->Position
			};
			FPlane TrianglePlane = FPlane( Verts[0], ((Verts[1]-Verts[2]) ^ (Verts[0]-Verts[2])).SafeNormal() );
#endif

			// Check the line against the node.

			FLOAT	StartDist = TrianglePlane.PlaneDot(LocalStart),
					EndDist = TrianglePlane.PlaneDot(LocalEnd),
					PushOut = FBoxPushOut(TrianglePlane,LocalExtent);

			if(StartDist <= -PushOut && EndDist <= -PushOut)
				NodeIndex = Node->ChildIndices[0];
			else if(StartDist >= PushOut && EndDist >= PushOut)
				NodeIndex = Node->ChildIndices[1];
			else
			{
				UBOOL	FrontFirst = StartDist >= EndDist;

				// Recurse with the front children and coplanars.

				if(Check(Node->ChildIndices[FrontFirst]))
				{
					Hit = 1;

					// Early out if the line hit one of the front child nodes.

					EndDist = TrianglePlane.PlaneDot(LocalStart + LocalDirection * Result->Time);

					if((FrontFirst && EndDist > PushOut) || (!FrontFirst && EndDist < -PushOut))
						break;
				}

				if(Check(Node->CoplanarIndex))
					Hit = 1;

				// Clip the line with the triangle.

				if(Data->GetCachedTag(Node->TriangleIndex) != Data->CurrentTag)
				{
					FClippedLine	ClippedLine(Start,End,Extent,Result->Time);

					if(ClipWithTriangle(Data,Triangle,Node->TriangleIndex,ClippedLine) && ClippedLine.Hit)
					{
						Result->Normal = ClippedLine.HitNormal;
						Result->Time = ClippedLine.T0;
						Result->Item = Node->TriangleIndex;
						Result->Material = StaticMesh->GetSkin(Owner,Triangle->MaterialIndex);

						Hit = 1;
					}

					// Tag triangles we check against, so we don't have to check against them again if they've been split.

					Data->GetCachedTag(Node->TriangleIndex) = Data->CurrentTag;
				}

				// See if we can discard the back subtree with the existing hit.

				UBOOL	Use[2] = { 1, 1 };

				if(Hit)
				{
					EndDist = TrianglePlane.PlaneDot(LocalStart + LocalDirection * Result->Time);

					Use[0] = StartDist <= PushOut || EndDist <= PushOut;
					Use[1] = StartDist >= -PushOut || EndDist >= -PushOut;
				}

				if(!Use[1 - FrontFirst])
					break;

				// Recurse with the back children.

				NodeIndex = Node->ChildIndices[1 - FrontFirst];
			}
		};

		return Hit;
	}
};

//
//	FLineCollisionCheck
//

class FLineCollisionCheck : public FCollisionCheck
{
public:

	FVector			Start,
					End,
					LocalStart,
					LocalEnd,
					LocalDirection,
					LocalOneOverDirection;
	FMatrix			WorldToLocal;

	// Constructor.

	FLineCollisionCheck(FCheckResult* InResult,AActor* InOwner,UStaticMesh* InStaticMesh,FVector InStart,FVector InEnd) :
		FCollisionCheck(InResult,InOwner,InStaticMesh)
	{
		Start = InStart;
		End = InEnd;

		WorldToLocal = Owner->WorldToLocal();

		LocalStart = WorldToLocal.TransformFVector(Start);
		LocalEnd = WorldToLocal.TransformFVector(End);
		LocalDirection = (LocalEnd - LocalStart);
		LocalOneOverDirection = FVector(1.0f / LocalDirection.X,1.0f / LocalDirection.Y,1.0f / LocalDirection.Z);
	}

	// Check
#if OLD_COLLISION_CODE
	UBOOL Check(INT NodeIndex)
	{
		while(NodeIndex != INDEX_NONE_COLLISION)
		{
			FStaticMeshCollisionNode*		Node = &StaticMesh->CollisionNodes(NodeIndex);
			FStaticMeshCollisionTriangle*	Triangle = &StaticMesh->CollisionTriangles(Node->TriangleIndex);

			// Cull the node by bounding box.

			if(!FLineBoxIntersection(Node->BoundingBox,LocalStart,LocalEnd,LocalDirection,LocalOneOverDirection))
				break;

			// Check the line against the node's plane.

			FLOAT	StartDist = Triangle->Plane.PlaneDot(LocalStart),
					EndDist = Triangle->Plane.PlaneDot(LocalEnd);

			if(StartDist > -0.001f && EndDist > -0.001f)
				NodeIndex = Node->ChildIndices[1];
			else if(StartDist < 0.001f && EndDist < 0.001f)
				NodeIndex = Node->ChildIndices[0];
			else
			{
				UBOOL	FrontFirst = StartDist > -0.001f;

				// Recurse with the front children and coplanars.

				if(Check(Node->ChildIndices[FrontFirst]) || Check(Node->CoplanarIndex))
					return 1;

				// Calculate the line's point of intersection with the node's plane.

				FLOAT	Time = -StartDist / (EndDist - StartDist);
				FVector	Intersection = LocalStart + LocalDirection * Time;

				// Check if the point of intersection is inside the triangle's edges.

				if(	Triangle->SidePlanes[0].PlaneDot(Intersection) < 0.0f &&
					Triangle->SidePlanes[1].PlaneDot(Intersection) < 0.0f &&
					Triangle->SidePlanes[2].PlaneDot(Intersection) < 0.0f)
				{
					Result->Normal = Owner->LocalToWorld().TransformNormal(Triangle->Plane);
					Result->Time = Time;
					Result->Item = Node->TriangleIndex;
					Result->Material = StaticMesh->GetSkin(Owner,Triangle->MaterialIndex);

					return 1;
				}

				// Recurse with the back children.

				NodeIndex = Node->ChildIndices[1 - FrontFirst];
			}
		};

		return 0;
	}
#else
	UBOOL Check(_WORD NodeIndex)
	{
		while(NodeIndex != INDEX_NONE_COLLISION)
		{
			FStaticMeshCollisionNode*		Node = &StaticMesh->CollisionNodes(NodeIndex);
			FStaticMeshCollisionTriangle*	Triangle = &StaticMesh->CollisionTriangles(Node->TriangleIndex);
			FBox							NodeBoundingBox;
			
			Node->BoundingBox.Expand( NodeBoundingBox, StaticMesh->CompressionScale );

			// Cull the node by bounding box.

			if(!FLineBoxIntersection(NodeBoundingBox,LocalStart,LocalEnd,LocalDirection,LocalOneOverDirection))
				break;

			// Calculate triangle planes on the fly.

			FStaticMeshVertex* Base = &StaticMesh->VertexStream.Vertices(0);

			FVector Verts[3] = 
			{ 
				(Base + Triangle->VertexIndices[0])->Position, 
				(Base + Triangle->VertexIndices[1])->Position,
				(Base + Triangle->VertexIndices[2])->Position
			};

#if 0
			FVector Direction	= (Verts[1]-Verts[2]) ^ (Verts[0]-Verts[2]);
			FLOAT	Threshold	= Direction.SizeSquared() * Square(0.001f),
					FaceW		= Direction | Verts[0],
					StartDist	= ((Direction | LocalStart) - FaceW),
					EndDist		= ((Direction | LocalEnd) - FaceW),
					SqStartDist = StartDist * fabs(StartDist),
					SqEndDist	= EndDist * fabs(EndDist);

			// Check the line against the node's plane.

			if(SqStartDist > -Threshold && SqEndDist > -Threshold)
				NodeIndex = Node->ChildIndices[1];
			else if(SqStartDist < Threshold && SqEndDist < Threshold)
				NodeIndex = Node->ChildIndices[0];
			else
			{
				UBOOL	FrontFirst = SqStartDist > -Threshold;
#else
			FVector	Direction		= (Verts[1]-Verts[2]) ^ (Verts[0]-Verts[2]);
			FPlane	TrianglePlane	= FPlane( Verts[0], Direction.SafeNormal() );

			FLOAT	StartDist		= TrianglePlane.PlaneDot(LocalStart),
					EndDist			= TrianglePlane.PlaneDot(LocalEnd);

			if(StartDist > -0.001f && EndDist > -0.001f)
				NodeIndex = Node->ChildIndices[1];
			else if(StartDist < 0.001f && EndDist < 0.001f)
				NodeIndex = Node->ChildIndices[0];
			else
			{
				UBOOL	FrontFirst = StartDist > -0.001f;

#endif

				// Recurse with the front children and coplanars.
				
				if(Check(Node->ChildIndices[FrontFirst]) || Check(Node->CoplanarIndex))
					return 1;

				// Calculate the line's point of intersection with the node's plane.
				
				FLOAT	Time = -StartDist / (EndDist - StartDist);
				FVector	Intersection = LocalStart + LocalDirection * Time;

				UBOOL Collision = 1;

				// Check if the point of intersection is inside the triangle's edges.
				
				for( INT SideIndex = 0; SideIndex < 3 && Collision; SideIndex++ )
				{
					FVector SideDirection	= Direction ^ (Verts[(SideIndex + 1) % 3] - Verts[SideIndex]);
					FLOAT	SideW			= SideDirection | Verts[SideIndex];

					if( ((SideDirection | Intersection) - SideW)  >= 0.f )
						Collision = 0;
				}
				
				if( Collision )
				{
					Result->Normal = Owner->LocalToWorld().TransformNormal(Direction.SafeNormal());
					Result->Time = Time;
					Result->Item = Node->TriangleIndex;
					Result->Material = StaticMesh->GetSkin(Owner,Triangle->MaterialIndex);

					return 1;
				}

				// Recurse with the back children.

				NodeIndex = Node->ChildIndices[1 - FrontFirst];
			}
		};

		return 0;
	}
#endif

};

UBOOL UStaticMesh::UseCylinderCollision( const AActor* Owner )
{
	guardSlow(UStaticMesh::UseCylinderCollision);

	return Owner->bUseCylinderCollision;
	unguardSlow;
}

//
//	UStaticMesh::GetCollisionBoundingBox
//

FBox UStaticMesh::GetCollisionBoundingBox(const AActor* Owner) const
{
	FBox Result;

	if(Owner->bUseCylinderCollision)
		Result = UPrimitive::GetCollisionBoundingBox(Owner);
	else
	{
		Result = BoundingBox.TransformBy(Owner->LocalToWorld());

		if(CollisionModel)
			Result += CollisionModel->GetCollisionBoundingBox(Owner);
	}

#ifdef WITH_KARMA
	// If this actor is bBlockKarma, when ensure its collision bounding box also includes 
	// the Karma primitive bounding box. This also keep the bounding box up to date.
	if(Owner->bBlockKarma)
	{
		McdModelID model = Owner->getKModel();
		if(model)
		{
			MeVector3 min, max;
			FVector umin, umax;
			McdModelGetAABB(model, min, max);
			KME2UPosition(&umin, min);
			KME2UPosition(&umax, max);
			Result += FBox(umin, umax);
		}
	}
#endif

	return Result;
}

//
//	UStaticMesh::LineCheck
//

UBOOL UStaticMesh::LineCheck(FCheckResult& Result,AActor* Owner,FVector End,FVector Start,FVector Extent,DWORD ExtraNodeFlags,DWORD TraceFlags)
{
	guard(UStaticMesh::LineCheck);

	clock(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCycles));
	UBOOL	Hit = 0,
			ZeroExtent = (Extent == FVector(0,0,0));

	if(Owner->bUseCylinderCollision)
		Hit = !UPrimitive::LineCheck(Result,Owner,End,Start,Extent,ExtraNodeFlags,TraceFlags);
	else if(CollisionModel && ((UseSimpleBoxCollision && !ZeroExtent) || (UseSimpleLineCollision && ZeroExtent)) && !(TraceFlags & TRACE_ShadowCast))
		Hit = !CollisionModel->LineCheck(Result,Owner,End,Start,Extent,ExtraNodeFlags,TraceFlags);
	else if(CollisionNodes.Num())
	{
		Result.Time = 1.0f;

		if(ZeroExtent)
			Hit = FLineCollisionCheck(&Result,Owner,this,Start,End).Check(0);
		else
			Hit = FSweptBoxCollisionCheck(&Result,Owner,this,Start,End,Extent).Check(0);

		if(Hit)
		{
			Result.Actor = Owner;
			Result.Primitive = this;
			Result.Time = Clamp(Result.Time - Clamp(0.1f,0.1f / (End - Start).Size(),1.0f / (End - Start).Size()),0.0f,1.0f);
			Result.Location = Start + (End - Start) * Result.Time;
			Result.Normal.Normalize();
		}
	}

	unclock(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCycles));
	return !Hit;

	unguard;
}

//
//	UStaticMesh::PointCheck
//

UBOOL UStaticMesh::PointCheck(FCheckResult& Result,AActor* Owner,FVector Location,FVector Extent,DWORD ExtraNodeFlags)
{
	guard(UStaticMesh::PointCheck);

	INT		StartCycles = appCycles();
	UBOOL	Hit = 0;

	if(Owner->bUseCylinderCollision)
		Hit = !UPrimitive::PointCheck(Result,Owner,Location,Extent,ExtraNodeFlags);
	else if(CollisionModel && UseSimpleBoxCollision)
		Hit = !CollisionModel->PointCheck(Result,Owner,Location,Extent,ExtraNodeFlags);
	else if(CollisionNodes.Num())
	{ 
		Hit = FBoxCollisionCheck(&Result,Owner,this,Location,Extent).Check(0);

		if(Hit)
		{
			Result.Actor = Owner;
			Result.Primitive = this;
			Result.Normal.Normalize();
		}
	}

	GStats.DWORDStats(GEngineStats.STATS_StaticMesh_CollisionCycles) += (appCycles() - StartCycles);

	return !Hit;

	unguard;
}


///////////////////////////// TRIANGLE QUERIES /////////////////////////////


//
// UStaticMesh::TriangleSphereQuery
//
// This will add triangles to the 'triangles' array that overlap the sphere 'sphere'
// Note - the pointers to the FStaticMeshCollisionTriangles are only valid as long as the static mesh is valid.

void UStaticMesh::TriangleSphereQuery(AActor* Owner, FSphere& sphere, TArray<FStaticMeshCollisionTriangle*> &triangles)
{
	guard(UStaticMesh::TriangleSphereQuery);

	// If there are no collision nodes in this static mesh, dont do anything.
	if(CollisionNodes.Num() == 0)
		return;

	FMatrix w2l = Owner->WorldToLocal();

	// Find bounding box we are querying triangles against in local space.
	FVector radiusVec = FVector(sphere.W, sphere.W, sphere.W);
	FBox WorldBox = FBox(sphere - radiusVec, sphere + radiusVec);
	FBox LocalBox = WorldBox.TransformBy(w2l);

	FVector LocalCenter, LocalExtent;
	LocalBox.GetCenterAndExtents(LocalCenter, LocalExtent);

	TArray<_WORD> NodeStack;
	CollisionTag++;
	NodeStack.AddItem(0);

	while(NodeStack.Num() != 0)
	{
		// Pop last node index off stack.
		_WORD nodeIx = NodeStack(NodeStack.Num() - 1);
		NodeStack.Remove(NodeStack.Num() - 1);

		check(nodeIx >= 0 && nodeIx < CollisionNodes.Num());
		FStaticMeshCollisionNode* node = &(CollisionNodes(nodeIx));

		check(node->TriangleIndex >= 0 && node->TriangleIndex < CollisionTriangles.Num());
		FStaticMeshCollisionTriangle* tri = &(CollisionTriangles(node->TriangleIndex));

		FBox NodeBoundingBox;
		node->BoundingBox.Expand( NodeBoundingBox, CompressionScale );

		// First see if query-box and node bounding box overlap.
		if( !LocalBox.Intersect(NodeBoundingBox) )
			continue;

#if OLD_COLLISION_CODE
		// Look at distance between query-box and node plane, and follow appropriate side.
		FLOAT Dist = tri->Plane.PlaneDot(LocalCenter);
		FLOAT PushOut = FBoxPushOut(tri->Plane, LocalExtent);
#else
		// Look at distance between query-box and node plane, and follow appropriate side.
		FStaticMeshVertex* Base = &VertexStream.Vertices(0);

		FVector Verts[3] = 
		{ 
			(Base + tri->VertexIndices[0])->Position, 
			(Base + tri->VertexIndices[1])->Position,
			(Base + tri->VertexIndices[2])->Position
		};

		FVector Direction	= (Verts[1]-Verts[2]) ^ (Verts[0]-Verts[2]);
		FLOAT	FaceW		= Direction | Verts[0],
				Dist		= (Direction | LocalCenter) - FaceW,
				PushOut		= FBoxPushOut(Direction, LocalExtent);
#endif

		if(Dist <= -PushOut)
		{
			if(node->ChildIndices[0] != INDEX_NONE_COLLISION) 
				NodeStack.AddItem(node->ChildIndices[0]); // BACK
		}
		else if(Dist >= PushOut)
		{
			if(node->ChildIndices[1] != INDEX_NONE_COLLISION) 
				NodeStack.AddItem(node->ChildIndices[1]); // FRONT
		}
		else
		{
			// Co-planar! Add to list (if not already added).
			_WORD cpNodeIx = nodeIx;
			while(cpNodeIx != INDEX_NONE_COLLISION)
			{
				FStaticMeshCollisionNode* cpNode = &(CollisionNodes(cpNodeIx));
				FStaticMeshCollisionTriangle* cpTri = &(CollisionTriangles(cpNode->TriangleIndex));

				if(	cpTri->CollisionTag != CollisionTag )
				{
					// Whatever happens, dont check this tri again.
					cpTri->CollisionTag = CollisionTag;

					UBOOL collideTri = 1;
#if 1
					// Check box against side planes of triangle.
					// Note - might still not overlap triangle (we'd need to bevel corners as well), 
					// but should remove a lot of unwanted triangles.
#if OLD_COLLISION_CODE
					for(INT i=0; i<3 && collideTri; i++)
					{
						FLOAT TriPlaneDist = cpTri->SidePlanes[i].PlaneDot(LocalCenter);
						FLOAT TriPlanePushOut = FBoxPushOut(cpTri->SidePlanes[i], LocalExtent);
						if(TriPlaneDist > TriPlanePushOut)
							collideTri = 0;
					}
#else	
					FVector cpVerts[3] = 
					{ 
						(Base + cpTri->VertexIndices[0])->Position, 
						(Base + cpTri->VertexIndices[1])->Position,
						(Base + cpTri->VertexIndices[2])->Position
					};

					for( INT i=0; i<3 && collideTri; i++ )
					{
						FVector SideDirection	= Direction ^ (cpVerts[(i + 1) % 3] - cpVerts[i]);
						FLOAT	SideW			= SideDirection | cpVerts[i];

						if( ((SideDirection | LocalCenter) - SideW)  > FBoxPushOut(SideDirection, LocalExtent) )
							collideTri = 0;
					}
#endif
#endif
					// Add triangle.
					if(collideTri)
						triangles.AddItem(cpTri);
				}

				cpNodeIx = cpNode->CoplanarIndex;
			}

			// Then continue to traverse both ways.
			if(node->ChildIndices[0] != INDEX_NONE_COLLISION) 
				NodeStack.AddItem(node->ChildIndices[0]); // BACK

			if(node->ChildIndices[1] != INDEX_NONE_COLLISION) 
				NodeStack.AddItem(node->ChildIndices[1]); // FRONT
		}
	};

	unguard;
}
