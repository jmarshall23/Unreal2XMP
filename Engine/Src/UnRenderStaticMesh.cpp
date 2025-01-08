/*=============================================================================
	UnRenderStaticMesh.cpp: Static mesh rendering code.
	Copyright 2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRenderPrivate.h"

//
// FSortedIndexBuffer
//

#if ASMLINUX
inline void FloatToInt(int *int_pointer, float f)
{
    __asm__ __volatile
    (
        "  flds    (%%eax)  \n\t"
        "  frndint          \n\t"
        "  fistp   (%%edx)  \n\t"
            : /* output to *int_pointer is done by fistp */
            : "d" (int_pointer), "a" (&f)
            : "cc", "memory"
    );
}
#else
__forceinline void FloatToInt(int *int_pointer, float f)
{
    __asm  fld  f
    __asm  mov  edx,int_pointer
    __asm  FRNDINT
    __asm  fistp dword ptr [edx];
}
#endif


class FSortedIndexBuffer : public FIndexBuffer
{
public:

	struct FTriangleSortInfo
	{
		FLOAT	SortKey;
		INT		Indices[3];

		friend INT Compare(FTriangleSortInfo& A,FTriangleSortInfo& B)
		{
			return (INT)(B.SortKey - A.SortKey);
		}
	};

	UStaticMesh*	StaticMesh;
	INT				SectionIndex;
	FVector			ViewOrigin;
    FPlane          ViewPlane;
	_WORD			MinVertexIndex;
	UBOOL			ReverseVertexOrder;

	QWORD			CacheId;

	// Constructor.

	FSortedIndexBuffer(UStaticMesh* InStaticMesh,INT InSectionIndex,FVector InViewOrigin,_WORD InMinVertexIndex,UBOOL InReverseVertexOrder = 0)
	{
		StaticMesh = InStaticMesh;
		SectionIndex = InSectionIndex;
		ViewOrigin = InViewOrigin;
		MinVertexIndex = InMinVertexIndex;
		ReverseVertexOrder = InReverseVertexOrder;
	}

	// FRenderResource interface.

	virtual QWORD GetCacheId() { return CacheId; }
	virtual INT GetRevision() { return 1; }

	// FIndexBuffer interface.

	virtual INT GetSize()
	{
		return StaticMesh->Sections(SectionIndex).NumTriangles * 3 * sizeof(_WORD);
	}
	
	virtual INT GetIndexSize()
	{
		return sizeof(_WORD);
	}

	virtual void GetContents(void* VoidDest)
	{
		guard(FSortedIndexBuffer::GetContents);

		_WORD* Dest = (_WORD*)VoidDest;

		FMemMark					MemMark(GSceneMem);
		FStaticMeshSection*			Section = &StaticMesh->Sections(SectionIndex);
		_WORD*						Indices = &StaticMesh->IndexBuffer.Indices(Section->FirstIndex);
		FStaticMeshVertex*			Vertices = &StaticMesh->VertexStream.Vertices(0);
		FTriangleSortInfo*			SortedTriangles = New<FTriangleSortInfo>(GSceneMem,Section->NumTriangles);

		for(INT TriangleIndex = 0;TriangleIndex < Section->NumTriangles;TriangleIndex++)
		{
#if 1
            SortedTriangles[TriangleIndex].Indices[0] = Indices[TriangleIndex * 3 + 0];
			SortedTriangles[TriangleIndex].Indices[1] = Indices[TriangleIndex * 3 + 1];
			SortedTriangles[TriangleIndex].Indices[2] = Indices[TriangleIndex * 3 + 2];

            int d0,d1,d2;
            FloatToInt( &d0, (Vertices[Indices[TriangleIndex * 3 + 0]].Position - ViewOrigin).SizeSquared());
            FloatToInt( &d1, (Vertices[Indices[TriangleIndex * 3 + 1]].Position - ViewOrigin).SizeSquared());
            FloatToInt( &d2, (Vertices[Indices[TriangleIndex * 3 + 2]].Position - ViewOrigin).SizeSquared());
            SortedTriangles[TriangleIndex].SortKey = Max(d0,Max(d1,d2));
#else
			FVector	TriangleVertices[3] =
			{
				Vertices[Indices[TriangleIndex * 3 + 0]].Position,
				Vertices[Indices[TriangleIndex * 3 + 1]].Position,
				Vertices[Indices[TriangleIndex * 3 + 2]].Position
			};

			SortedTriangles[TriangleIndex].Indices[0] = Indices[TriangleIndex * 3 + 0];
			SortedTriangles[TriangleIndex].Indices[1] = Indices[TriangleIndex * 3 + 1];
			SortedTriangles[TriangleIndex].Indices[2] = Indices[TriangleIndex * 3 + 2];
			SortedTriangles[TriangleIndex].SortKey = Max((TriangleVertices[0] - ViewOrigin).SizeSquared(),Max((TriangleVertices[1] - ViewOrigin).SizeSquared(),(TriangleVertices[2] - ViewOrigin).SizeSquared()));
#endif
		}

		Sort(SortedTriangles,Section->NumTriangles);

		for(INT TriangleIndex = 0;TriangleIndex < Section->NumTriangles;TriangleIndex++)
		{
			if(ReverseVertexOrder)
			{
				*Dest++ = SortedTriangles[TriangleIndex].Indices[2] - Section->MinVertexIndex + MinVertexIndex;
				*Dest++ = SortedTriangles[TriangleIndex].Indices[1] - Section->MinVertexIndex + MinVertexIndex;
				*Dest++ = SortedTriangles[TriangleIndex].Indices[0] - Section->MinVertexIndex + MinVertexIndex;
			}
			else
			{
				*Dest++ = SortedTriangles[TriangleIndex].Indices[0] - Section->MinVertexIndex + MinVertexIndex;
				*Dest++ = SortedTriangles[TriangleIndex].Indices[1] - Section->MinVertexIndex + MinVertexIndex;
				*Dest++ = SortedTriangles[TriangleIndex].Indices[2] - Section->MinVertexIndex + MinVertexIndex;
			}
		}

		MemMark.Pop();

		unguard;
	}
};

//
//	CalculateStaticMeshLighting
//

void CalculateStaticMeshLighting(UStaticMesh* StaticMesh,UStaticMeshInstance* StaticMeshInstance,FDynamicActor* Owner)
{
	guard(CalculateStaticMeshLighting);

	// Update the static lighting stream.

	appMemzero(&StaticMeshInstance->ColorStream.Colors(0),StaticMeshInstance->ColorStream.Colors.Num() * sizeof(FColor));
	StaticMeshInstance->ColorStream.Revision++;

	for(INT LightIndex = 0;LightIndex < StaticMeshInstance->Lights.Num();LightIndex++)
	{
		FStaticMeshLightInfo*	LightInfo = &StaticMeshInstance->Lights(LightIndex);

		LightInfo->Applied = 0;

		if(LightInfo->LightActor)
		{
			FDynamicLight*	Light = LightInfo->LightActor->GetLightRenderData();
    
		    if(Light && !Light->Changed && !Light->Dynamic)
		    {
			    FStaticMeshVertex*	VertexPtr = &StaticMesh->VertexStream.Vertices(0);
			    FColor*				ColorPtr = &StaticMeshInstance->ColorStream.Colors(0);
			    BYTE*				BitPtr = LightInfo->VisibilityBits.Num() ? &LightInfo->VisibilityBits(0) : NULL;
			    BYTE				BitMask = 1;
    
			    for(INT VertexIndex = 0;VertexIndex < StaticMesh->VertexStream.Vertices.Num();VertexIndex++)
			    {
				    FVector	SamplePoint = Owner->LocalToWorld.TransformFVector(VertexPtr->Position),
						    SampleNormal = Owner->LocalToWorld.TransformNormal(VertexPtr->Normal).SafeNormal();
    
				    if(*BitPtr & BitMask)
					{
					    *ColorPtr += FColor(
									    Owner->Actor->ScaleGlow *
											0.5f *
											Light->Color *
											Light->SampleIntensity(
												SamplePoint,
												SampleNormal
												)
										);
					}
    
				    ColorPtr++;
				    VertexPtr++;
				    BitMask <<= 1;
    
				    if(!BitMask)
				    {
					    BitPtr++;
					    BitMask = 1;
				    }
			    }
    
			    LightInfo->Applied = 1;
		    }
		}
	}

	// Blend the vertex color/alpha into the color stream.

	FColor*	SrcPtr = &StaticMesh->ColorStream.Colors(0);
	FColor*	DestPtr = &StaticMeshInstance->ColorStream.Colors(0);

	for(INT VertexIndex = 0;VertexIndex < StaticMesh->VertexStream.Vertices.Num();VertexIndex++)
	{
		if(StaticMesh->UseVertexColor)
		{
			DestPtr->R = (BYTE) ((INT)SrcPtr->R * (INT)DestPtr->R / (INT)255);
			DestPtr->G = (BYTE) ((INT)SrcPtr->G * (INT)DestPtr->G / (INT)255);
			DestPtr->B = (BYTE) ((INT)SrcPtr->B * (INT)DestPtr->B / (INT)255);
		}

		DestPtr->A = SrcPtr->A;

		SrcPtr++;
		DestPtr++;
	}

	unguard;
}

//
//	DrawSection
//

void DrawSection(UStaticMesh* StaticMesh,INT SectionIndex,UMaterial* Material,FRenderInterface* RI)
{
	FStaticMeshSection*	Section = &StaticMesh->Sections(SectionIndex);

	if(Section->NumPrimitives > 0)
	{
		RI->SetMaterial(Material);

		// Render the section.
		RI->DrawPrimitive(
			Section->IsStrip ? PT_TriangleStrip : PT_TriangleList,
			Section->FirstIndex,
			Section->NumPrimitives,
			Section->MinVertexIndex,
			Section->MaxVertexIndex
			);
	}

	GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedUnsortedSections)++;
	GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedUnsortedTriangles) += Section->NumTriangles;
	GStats.DWORDStats(GEngineStats.STATS_StaticMesh_Triangles) += Section->NumTriangles;
}

//
//	DrawSortedSection
//

void DrawSortedSection(FLevelSceneNode* SceneNode,FDynamicActor* Owner,UStaticMesh* StaticMesh,INT SectionIndex,UMaterial* Material,FRenderInterface* RI)
{
	FStaticMeshSection*	Section = &StaticMesh->Sections(SectionIndex);

	if(Section->NumPrimitives > 0)
	{
		RI->SetMaterial(Material);

		// Sort the triangles back to front.

		INT					SortStartTime = appCycles();
        
		FSortedIndexBuffer	IndexBuffer(StaticMesh,SectionIndex,Owner->WorldToLocal.TransformFVector(SceneNode->ViewOrigin),Section->MinVertexIndex);

		INT	BaseIndex = RI->SetDynamicIndexBuffer(&IndexBuffer,0);

		GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedSortedSections)++;
		GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedSortedTriangles) += Section->NumTriangles;
		GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedSortCycles) += (appCycles() - SortStartTime);
		GStats.DWORDStats(GEngineStats.STATS_StaticMesh_Triangles) += Section->NumTriangles;

		// Render the section.
		RI->DrawPrimitive(
			Section->IsStrip ? PT_TriangleStrip : PT_TriangleList,
			BaseIndex,
			Section->NumPrimitives,
			Section->MinVertexIndex,
			Section->MaxVertexIndex
			);
	}
}

//
//	RenderStaticMesh
//

void RenderStaticMesh(FDynamicActor* Owner,FLevelSceneNode* SceneNode,TList<FDynamicLight*>* Lights,TList<FProjectorRenderInfo*>* Projectors,FRenderInterface* RI)
{
	guard(RenderStaticMesh);

	clock(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_RenderCycles));
	clock(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedRenderCycles));

	AActor*					Actor = Owner->Actor;
	UStaticMesh*			StaticMesh = Actor->StaticMesh;
	UStaticMeshInstance*	StaticMeshInstance = NULL;

	if(!StaticMesh->VertexStream.Vertices.Num())
		return;

	// Determine if we need a tangent-binormal stream. Also, check to see if we should be using a hardware shader.
	UBOOL  NeedTB = false;
	UBOOL  UseHardwareShader = false;

	for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
	{
		UHardwareShader*	Material = Cast<UHardwareShader>(StaticMesh->GetSkin(Owner->Actor,SectionIndex));
		if(Material)
		{
			INT trash;
			if(Material->StreamMapping.FindItem(FVF_Tangent,trash) || Material->StreamMapping.FindItem(FVF_Binormal,trash))
			{
				NeedTB = true;
			}
			UseHardwareShader = true;
			break;
		}
	}

	if(Actor->bSelected && (SceneNode->Viewport->Actor->ShowFlags & SHOW_SelectionHighlight))
	{
		// Highlight the static mesh.

		RI->EnableLighting(1,1,0,NULL,SceneNode->Viewport->Actor->RendMap == REN_LightingOnly,Owner->BoundingSphere);
		RI->SetAmbientLight(FColor(102,255,102));
	}
	else if(Actor->GetAmbientLightingActor()->bUnlit || !SceneNode->Viewport->IsLit())
	{
		// Disable lighting for this staticmesh.

		RI->EnableLighting(1,1,0,NULL,SceneNode->Viewport->Actor->RendMap == REN_LightingOnly,Owner->BoundingSphere);
		RI->SetAmbientLight(FColor(255,255,255));
	}
	else if(!SceneNode->Viewport->IsWire())
	{
		if(Actor->StaticMeshInstance)
		{
			// Verify the actor's instance lighting matches the static mesh.

			UBOOL	Matches = 1;

			if(Actor->StaticMeshInstance->ColorStream.Colors.Num() != StaticMesh->VertexStream.Vertices.Num())
				Matches = 0;

			if(Matches)
			{
				for(INT LightIndex = 0;LightIndex < Actor->StaticMeshInstance->Lights.Num();LightIndex++)
					if(Actor->StaticMeshInstance->Lights(LightIndex).VisibilityBits.Num() != (StaticMesh->VertexStream.Vertices.Num() + 7) / 8)
					{
						Matches = 0;
						break;
					}
			}

			if(Matches)
				StaticMeshInstance = Actor->StaticMeshInstance;
		}

		if(StaticMeshInstance)
		{
			// Determine whether the static lighting stream needs to be updated.

			UBOOL	UpdateStaticLighting = 0;

			for(INT LightIndex = 0;LightIndex < StaticMeshInstance->Lights.Num();LightIndex++)
			{
				FStaticMeshLightInfo*	LightInfo = &StaticMeshInstance->Lights(LightIndex);

                if( !LightInfo->LightActor )
                {
                    StaticMeshInstance->Lights.Remove(LightIndex);
                    LightIndex = Min(0,LightIndex-1);
                    continue;
                }

				FDynamicLight*			Light = LightInfo->LightActor->GetLightRenderData();

				if((!Light || Light->Dynamic || Light->Changed) == LightInfo->Applied)
				{
					UpdateStaticLighting = 1;
					break;
				}
			}

			// Update the static lighting.

			if(UpdateStaticLighting)
				CalculateStaticMeshLighting(StaticMesh,StaticMeshInstance,Owner);
		}

		// Set the hardware lights for the actor.
        FSphere lightSphere = Owner->BoundingSphere;
        if( Owner->Actor->bSpecialLit && !Owner->Actor->bStaticLighting )
            lightSphere.W = -1.0f;

		if(Owner->Actor->StaticSectionBatches.Num() != StaticMesh->Sections.Num())
		{
			RI->EnableLighting(1,1,1,NULL,SceneNode->Viewport->Actor->RendMap == REN_LightingOnly,lightSphere);
			 RI->SetAmbientLight(FColor(FPlane(Owner->AmbientColor) * 0.5f));
		}
		else
		{
			UBOOL	UseHardwareLighting = 0;

			for(TList<FDynamicLight*>* LightList = Lights;LightList;LightList = LightList->Next)
				if(!StaticMeshInstance || LightList->Element->Dynamic || LightList->Element->Changed)
				{
					UseHardwareLighting = 1;
					break;
				}

			RI->EnableLighting(UseHardwareLighting,1,1,NULL,SceneNode->Viewport->Actor->RendMap == REN_LightingOnly,lightSphere);
			RI->SetAmbientLight(FColor(0,0,0));
		}

		INT	NumHardwareLights = 0;

		for(TList<FDynamicLight*>* LightList = Lights;LightList;LightList = LightList->Next)
			if(!StaticMeshInstance || LightList->Element->Dynamic || LightList->Element->Changed || UseHardwareShader)
				RI->SetLight(NumHardwareLights++,LightList->Element);
	}

	if(SceneNode->Viewport->IsWire())
	{
	    // Set the local to world transform.
    
	    RI->SetTransform(TT_LocalToWorld,Owner->LocalToWorld);

		// Determine the wireframe color.

		UEngine*	Engine = SceneNode->Viewport->GetOuterUClient()->Engine;
		FColor		WireColor = Actor->IsA(AMover::StaticClass()) ? Engine->C_Mover : Engine->C_StaticMesh;

		RI->EnableLighting(1,0,0,NULL,0,Owner->BoundingSphere);

		if(Actor->bSelected && (SceneNode->Viewport->Actor->ShowFlags & SHOW_SelectionHighlight))
			RI->SetAmbientLight(WireColor);
		else
			RI->SetAmbientLight(FColor(WireColor.Plane() * 0.5f));

		static FSolidColorTexture	WhiteTexture(FColor(255,255,255));

		DECLARE_STATIC_UOBJECT( UProxyBitmapMaterial, HACKGAH, { HACKGAH->SetTextureInterface(&WhiteTexture); } );
		DECLARE_STATIC_UOBJECT( UFinalBlend, LineMaterial, { LineMaterial->Material = HACKGAH; } );

		RI->SetMaterial(LineMaterial);

		// Set the vertex stream and index buffer.

		FVertexStream*	VertexStreams[1] = { &StaticMesh->VertexStream };

		RI->SetVertexStreams(VS_FixedFunction,VertexStreams,1);
		RI->SetIndexBuffer(&StaticMesh->WireframeIndexBuffer,0);

		// Draw the wireframe static mesh.

		RI->DrawPrimitive(
			PT_LineList,
			0,
			StaticMesh->WireframeIndexBuffer.Indices.Num() / 2,
			0,
			StaticMesh->VertexStream.Vertices.Num() - 1
			);

		for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
			GStats.DWORDStats(GEngineStats.STATS_StaticMesh_Triangles) += StaticMesh->Sections(SectionIndex).NumTriangles;
	}
	else
	{
		// Set the vertex streams and index buffer.

		if(Owner->Actor->StaticSectionBatches.Num() == StaticMesh->Sections.Num())
		{
			RI->SetCullMode(CM_CW);

			for(INT Pass = 0;Pass < 2;Pass++)
			{
				for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
				{
					FBatchReference&	Ref = Owner->Actor->StaticSectionBatches(SectionIndex);

					if(Ref.ElementIndex != INDEX_NONE)
					{
						FStaticMeshBatch&	Batch = SceneNode->Level->StaticMeshBatches(Ref.BatchIndex);

						UMaterial*	Material = StaticMesh->GetSkin(Owner->Actor,SectionIndex);
						UBOOL		RequiresSorting = Material->RequiresSorting() && (Owner->BoundingSphere - SceneNode->ViewOrigin).SizeSquared() < Square(8192.0f);

						RI->SetMaterial(Material);

						FVertexStream*	VertexStreams[1] = { &Batch.Vertices };
						UBOOL			ReportDynamicUploads = !SceneNode->Viewport->Precaching && SceneNode->Viewport->GetOuterUClient()->ReportDynamicUploads;
						INT				UploadedSize = RI->SetVertexStreams(VS_FixedFunction,VertexStreams,1);

						if(UploadedSize && ReportDynamicUploads)
							debugf(TEXT("Uploading Batched StaticMesh: Actor=[%s] StaticMesh=[%s], Size=[%i]"), Owner->Actor->GetPathName(), Owner->Actor->StaticMesh->GetPathName(), UploadedSize );

						FStaticMeshBatch::FBatchElement&	Element = Batch.Elements(Ref.ElementIndex);

						if(Element.NumPrimitives > 0)
						{
							INT	BaseIndex;

							if(Pass == 0 && !RequiresSorting)
							{
								GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedUnsortedSections)++;
								GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedUnsortedTriangles) += Element.NumPrimitives;
								GStats.DWORDStats(GEngineStats.STATS_StaticMesh_Triangles) += Element.NumPrimitives;

								RI->SetIndexBuffer(&Batch.Indices,0);
								BaseIndex = Element.FirstIndex;
							}
							else if(Pass == 1 && RequiresSorting)
							{
								GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedSortedSections)++;
								GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedSortedTriangles) += Element.NumPrimitives;
								GStats.DWORDStats(GEngineStats.STATS_StaticMesh_Triangles) += Element.NumPrimitives;

								// Build a sorted index buffer.

								FSortedIndexBuffer	IndexBuffer(StaticMesh,SectionIndex,Owner->WorldToLocal.TransformFVector(SceneNode->ViewOrigin),Element.MinVertexIndex,Owner->Determinant < 0.0f);
								BaseIndex = RI->SetDynamicIndexBuffer(&IndexBuffer,0);
							}
							else
								continue;

							// Render the section.

							RI->DrawPrimitive(
								PT_TriangleList,
								BaseIndex,
								Element.NumPrimitives,
								Element.MinVertexIndex,
								Element.MaxVertexIndex
								);

							// Render the projectors hitting the section.

							AActor* ProjectorBase = Owner->Actor->GetProjectorBase();
							if(ProjectorBase->Projectors.Num() && !SceneNode->Viewport->IsWire() && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Projectors))
							{
								INT	ProjectorStartCycles = appCycles();

								RI->PushState();

								for(INT ProjectorIndex = 0;ProjectorIndex < ProjectorBase->Projectors.Num();ProjectorIndex++)
								{
									FProjectorRenderInfo*	ProjectorInfo = ProjectorBase->Projectors(ProjectorIndex);

									if(!ProjectorInfo->Render( Owner->Actor->Level->TimeSeconds ))
									{
										ProjectorBase->Projectors.Remove(ProjectorIndex--);
										continue;
									}

									// Setup blending.

									RI->EnableLighting(0,0);
									RI->SetMaterial(ProjectorInfo->GetMaterial(SceneNode,ProjectorBase,StaticMesh->GetSkin(Owner->Actor,SectionIndex)));

									// Render the projector.

									RI->DrawPrimitive(
										PT_TriangleList,
										BaseIndex,//Element.FirstIndex,
										Element.NumPrimitives,
										Element.MinVertexIndex,
										Element.MaxVertexIndex
										);

									GStats.DWORDStats(GEngineStats.STATS_Projector_Projectors)++;
									GStats.DWORDStats(GEngineStats.STATS_Projector_Triangles) += Element.NumPrimitives;
								}

								RI->PopState();

								GStats.DWORDStats(GEngineStats.STATS_Projector_RenderCycles) += appCycles() - ProjectorStartCycles;
							}

							// Render the dynamic projectors hitting the static mesh.

							if(!SceneNode->Viewport->IsWire() && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Projectors))
							{
								INT	ProjectorStartCycles = appCycles();

								RI->PushState();

								for(TList<FProjectorRenderInfo*>* ProjectorList = Projectors;ProjectorList;ProjectorList = ProjectorList->Next)
								{
									FProjectorRenderInfo*	ProjectorInfo = ProjectorList->Element;

									if(!ProjectorInfo->Render( Owner->Actor->Level->TimeSeconds ))
										continue;

									// Setup blending.

									RI->EnableLighting(0,0);
									RI->SetMaterial(ProjectorInfo->GetMaterial(SceneNode,ProjectorBase,StaticMesh->GetSkin(Owner->Actor,SectionIndex)));

									// Render the projector.

									RI->DrawPrimitive(
										PT_TriangleList,
										BaseIndex,//Element.FirstIndex,
										Element.NumPrimitives,
										Element.MinVertexIndex,
										Element.MaxVertexIndex
									);

									GStats.DWORDStats(GEngineStats.STATS_Projector_Projectors)++;
									GStats.DWORDStats(GEngineStats.STATS_Projector_Triangles) += Element.NumPrimitives;
								}

								RI->PopState();

								GStats.DWORDStats(GEngineStats.STATS_Projector_RenderCycles) += appCycles() - ProjectorStartCycles;
							}
						}
					}
				}
			}
		}
		else
		{
			// Set the local to world transform.

			RI->SetTransform(TT_LocalToWorld,Owner->LocalToWorld);

		    // Determine the required UV streams.
    
		    BYTE	RequiredUVStreams = 0;
    
		    for(INT MaterialIndex = 0;MaterialIndex < StaticMesh->Materials.Num();MaterialIndex++)
			    RequiredUVStreams |= StaticMesh->GetSkin(Owner->Actor,MaterialIndex)->RequiredUVStreams();
    
		    // Set the vertex streams.
    
		    FVertexStream*	VertexStreams[16];
		    INT				NumVertexStreams = 0;
    
		    VertexStreams[NumVertexStreams++] = &StaticMesh->VertexStream;
    
		    if(StaticMesh->UseVertexColor)
			    VertexStreams[NumVertexStreams++] = &StaticMesh->ColorStream;
		    else
			    VertexStreams[NumVertexStreams++] = &StaticMesh->AlphaStream;
    
		    if(StaticMeshInstance)
			    VertexStreams[NumVertexStreams++] = &StaticMeshInstance->ColorStream;
    
		    BYTE UVStreamMask = 0xff;
    
		    for(INT UVIndex = 0;UVIndex < StaticMesh->UVStreams.Num();UVIndex++,UVStreamMask <<= 1)
			    if(RequiredUVStreams & UVStreamMask)
				    VertexStreams[NumVertexStreams++] = &StaticMesh->UVStreams(UVIndex);
    
			// If we need the TB, and the stream is present in the static mesh 
 			if(NeedTB && StaticMesh->TBStream.TBs.Num() > 0)
                VertexStreams[NumVertexStreams++] = &StaticMesh->TBStream;

			UBOOL	ReportDynamicUploads	= !SceneNode->Viewport->Precaching && SceneNode->Viewport->GetOuterUClient()->ReportDynamicUploads;
			INT		UploadedSize			= RI->SetVertexStreams(UseHardwareShader ? VS_HardwareShaderDefined : VS_FixedFunction,VertexStreams,NumVertexStreams);
			if( UploadedSize && ReportDynamicUploads )
				debugf(TEXT("Uploading StaticMesh: Actor=[%s] StaticMesh=[%s], Size=[%i]"), Owner->Actor->GetPathName(), Owner->Actor->StaticMesh->GetPathName(), UploadedSize );

			RI->SetIndexBuffer(&StaticMesh->IndexBuffer,0);
    
		    // Draw the static mesh normally.
    
		    for(INT Pass = 0;Pass < 2;Pass++)
		    {
			    for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
			    {
				    UMaterial*	Material = StaticMesh->GetSkin(Owner->Actor,SectionIndex);
				    UBOOL		RequiresSorting = Material->RequiresSorting() && (Owner->BoundingSphere - SceneNode->ViewOrigin).SizeSquared() < Square(8192.0f);
    
				    if(Pass == 0 && !RequiresSorting)
					    DrawSection(StaticMesh,SectionIndex,Material,RI);
				    else if(Pass == 1 && RequiresSorting)
					    DrawSortedSection(SceneNode,Owner,StaticMesh,SectionIndex,Material,RI);
			    }
		    }
		
			// Render the projectors hitting the static mesh.
	        AActor* ProjectorBase = Owner->Actor->GetProjectorBase();
	        if(ProjectorBase->Projectors.Num() && !SceneNode->Viewport->IsWire() && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Projectors))
	        {
		        INT	ProjectorStartCycles = appCycles();

				RI->SetIndexBuffer(&StaticMesh->IndexBuffer,0);

				RI->PushState();
				RI->SetZBias(1);

				for(INT ProjectorIndex = 0;ProjectorIndex < ProjectorBase->Projectors.Num();ProjectorIndex++)
				{
					FProjectorRenderInfo*	ProjectorInfo = ProjectorBase->Projectors(ProjectorIndex);

					if(!ProjectorInfo->Render( Owner->Actor->Level->TimeSeconds ))
					{
						ProjectorBase->Projectors.Remove(ProjectorIndex--);
						continue;
					}

					for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
					{
						FStaticMeshSection*	Section = &StaticMesh->Sections(SectionIndex);

						if( Section->NumPrimitives <= 0 )
							continue;

						// Setup blending.

						RI->EnableLighting(0,0);

						// Render the projector.

						DrawSection(StaticMesh,SectionIndex,ProjectorInfo->GetMaterial(SceneNode,ProjectorBase,StaticMesh->GetSkin(Owner->Actor,SectionIndex)),RI);

						GStats.DWORDStats(GEngineStats.STATS_Projector_Projectors)++;
						GStats.DWORDStats(GEngineStats.STATS_Projector_Triangles) += Section->NumPrimitives;
					}
				}
        
				RI->PopState();

				GStats.DWORDStats(GEngineStats.STATS_Projector_RenderCycles) += appCycles() - ProjectorStartCycles;
	        }

			// Render the dynamic projectors hitting the static mesh.
			if(!SceneNode->Viewport->IsWire() && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Projectors))
			{
				INT	ProjectorStartCycles = appCycles();

				RI->SetIndexBuffer(&StaticMesh->IndexBuffer,0);

				RI->PushState();
				RI->SetZBias(1);

				for(TList<FProjectorRenderInfo*>* ProjectorList = Projectors;ProjectorList;ProjectorList = ProjectorList->Next)
				{
					FProjectorRenderInfo*	ProjectorInfo = ProjectorList->Element;

					if(!ProjectorInfo->Render( Owner->Actor->Level->TimeSeconds ))
						continue;

					for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
					{
						FStaticMeshSection*	Section = &StaticMesh->Sections(SectionIndex);

						if( Section->NumPrimitives <= 0 )
							continue;

						// Setup blending.

						RI->EnableLighting(0,0);

						// Render the projector.

						DrawSection(StaticMesh,SectionIndex,ProjectorInfo->GetMaterial(SceneNode,ProjectorBase,StaticMesh->GetSkin(Owner->Actor,SectionIndex)),RI);

						GStats.DWORDStats(GEngineStats.STATS_Projector_Projectors)++;
						GStats.DWORDStats(GEngineStats.STATS_Projector_Triangles) += Section->NumPrimitives;
					}
				}

				RI->PopState();

				GStats.DWORDStats(GEngineStats.STATS_Projector_RenderCycles) += appCycles() - ProjectorStartCycles;
			}
		}
	}

	// Batch projectors on this static mesh.

	if(!SceneNode->Viewport->IsWire() && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Projectors))
	{
		for(INT ProjectorIndex = 0;ProjectorIndex < Actor->StaticMeshProjectors.Num();ProjectorIndex++)
		{
			FStaticProjectorInfo*	ProjectorInfo = Actor->StaticMeshProjectors(ProjectorIndex);

			if(!ProjectorInfo->RenderInfo->Render( Owner->Actor->Level->TimeSeconds ))
			{
				delete Actor->StaticMeshProjectors(ProjectorIndex);
				Actor->StaticMeshProjectors.Remove(ProjectorIndex--);
				continue;
			}

			// Find an existing batch for the projector.

			FProjectorRenderBatch*	ProjectorBatch = NULL;

			for(TList<FProjectorRenderBatch*>* ProjectorBatchList = GProjectorBatchList;ProjectorBatchList;ProjectorBatchList = ProjectorBatchList->Next)
			{
				if(ProjectorBatchList->Element->ProjectedMaterial != ProjectorInfo->RenderInfo->Material)
					continue;

				if(ProjectorBatchList->Element->FramebufferBlending != ProjectorInfo->RenderInfo->FrameBufferBlendingOp)
					continue;

				if(ProjectorBatchList->Element->BaseMaterialBlending != ProjectorInfo->RenderInfo->MaterialBlendingOp)
					continue;

				if(ProjectorBatchList->Element->BaseMaterial != ProjectorInfo->BaseMaterial)
					continue;

				if(ProjectorBatchList->Element->TwoSided != ProjectorInfo->TwoSided)
					continue;

				if(ProjectorBatchList->Element->VertexBufferSize >= 100000)
					continue;

				ProjectorBatch = ProjectorBatchList->Element;
			}

			// Create a new batch if necessary.

			if(!ProjectorBatch)
			{
				ProjectorBatch = new(GSceneMem) FProjectorRenderBatch(
													ProjectorInfo->BaseMaterial,
													ProjectorInfo->RenderInfo->Material,
													(EProjectorBlending)ProjectorInfo->RenderInfo->MaterialBlendingOp,
													(EProjectorBlending)ProjectorInfo->RenderInfo->FrameBufferBlendingOp,
													ProjectorInfo->TwoSided
													);
				GProjectorBatchList = new(GSceneMem) TList<FProjectorRenderBatch*>(ProjectorBatch,GProjectorBatchList);
			}

			// Add the projector to the batch.

			ProjectorBatch->AddPrimitive(new(GSceneMem) FStaticProjectorPrimitive(ProjectorInfo));
		}
	}

	// Draw the vertex normals.

	if(SceneNode->Viewport->bShowNormals)
	{
		FLineBatcher	LineBatcher(RI);

		for(INT VertexIndex = 0;VertexIndex < StaticMesh->VertexStream.Vertices.Num();VertexIndex++)
		{
			FStaticMeshVertex&	Vertex = StaticMesh->VertexStream.Vertices(VertexIndex);

			LineBatcher.DrawLine(Vertex.Position,Vertex.Position + Vertex.Normal * 8.0f,FColor(255,255,0));
		}
	}

#ifdef WITH_KARMA
	// Draw Karma collision primitives
	if(StaticMesh->KPhysicsProps && Actor->bBlockKarma)
	{
		McdModelID model = Actor->getKModel();
		if(model)
			KModelDraw(model, KGData->DebugDrawOpt, KLineDraw);
		else
			StaticMesh->KPhysicsProps->Draw(RI, SceneNode->Viewport->Actor->ShowFlags);
	}
#endif

	// If we have a simple collision model, and its being used, draw it.
	if(SceneNode->Viewport->Actor->ShowFlags & SHOW_Collision &&
		StaticMesh->CollisionModel && 
		(StaticMesh->UseSimpleBoxCollision || StaticMesh->UseSimpleLineCollision || StaticMesh->UseSimpleKarmaCollision) )
	{
		UEngine* Engine = SceneNode->Viewport->GetOuterUClient()->Engine;

		for(INT NodeIndex = 0;NodeIndex < StaticMesh->CollisionModel->Nodes.Num();NodeIndex++)
		{
			FBspNode&	Node = StaticMesh->CollisionModel->Nodes(NodeIndex);

			for(INT VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
			{
				FLineBatcher(RI).DrawLine(
					StaticMesh->CollisionModel->Points(StaticMesh->CollisionModel->Verts(Node.iVertPool + VertexIndex).pVertex),
					StaticMesh->CollisionModel->Points(StaticMesh->CollisionModel->Verts(Node.iVertPool + ((VertexIndex + 1) % Node.NumVertices)).pVertex),
					Engine->C_ScaleBoxHi
					);
			}
		}
	}

	unclock(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_UnbatchedRenderCycles));
	unclock(GStats.DWORDStats(GEngineStats.STATS_StaticMesh_RenderCycles));

	unguard;
}

//
//	RenderBatchedStaticMesh
//

void RenderBatchedStaticMesh(FStaticMeshBatchList& BatchList,FDynamicActor* DynamicActor,FLevelSceneNode* SceneNode,FRenderInterface* RI)
{
	guard(RenderBatchedStaticMesh);

	AActor*			Actor = DynamicActor->Actor;
	UStaticMesh*	StaticMesh = Actor->StaticMesh;

	if( !StaticMesh ) 
		return;

	// Batch the static mesh's sections for rendering.

	if(Actor->StaticSectionBatches.Num() == StaticMesh->Sections.Num())
	{
		for(INT SectionIndex = 0;SectionIndex < StaticMesh->Sections.Num();SectionIndex++)
		{
			FBatchReference&	Ref = Actor->StaticSectionBatches(SectionIndex);

			if(Ref.ElementIndex != INDEX_NONE)
			{
				if(!BatchList.VisibleBatchElements[Ref.BatchIndex])
				{
					BatchList.VisibleBatchElements[Ref.BatchIndex] = New<INT>(GSceneMem,SceneNode->Level->StaticMeshBatches(Ref.BatchIndex).Elements.Num());
					BatchList.NumVisibleBatchElements[Ref.BatchIndex] = 0;
					BatchList.Batches[BatchList.NumBatches++] = Ref.BatchIndex;
				}

				BatchList.VisibleBatchElements[Ref.BatchIndex][BatchList.NumVisibleBatchElements[Ref.BatchIndex]++] = Ref.ElementIndex;
			}
		}
	}

	// Batch projectors on this static mesh.

	if(!SceneNode->Viewport->IsWire() && (SceneNode->Viewport->Actor->ShowFlags & SHOW_Projectors))
	{
		AActor* ProjectorBase = Actor->GetProjectorBase();

		for(INT ProjectorIndex = 0;ProjectorIndex < ProjectorBase->StaticMeshProjectors.Num();ProjectorIndex++)
		{
			FStaticProjectorInfo*	ProjectorInfo = ProjectorBase->StaticMeshProjectors(ProjectorIndex);

			if(!ProjectorInfo->RenderInfo->Render( DynamicActor->Actor->Level->TimeSeconds ))
			{
				delete ProjectorBase->StaticMeshProjectors(ProjectorIndex);
				ProjectorBase->StaticMeshProjectors.Remove(ProjectorIndex--);
				continue;
			}

			// Find an existing batch for the projector.

			FProjectorRenderBatch*	ProjectorBatch = NULL;

			for(TList<FProjectorRenderBatch*>* ProjectorBatchList = GProjectorBatchList;ProjectorBatchList;ProjectorBatchList = ProjectorBatchList->Next)
			{
				if(ProjectorBatchList->Element->ProjectedMaterial != ProjectorInfo->RenderInfo->Material)
					continue;

				if(ProjectorBatchList->Element->FramebufferBlending != ProjectorInfo->RenderInfo->FrameBufferBlendingOp)
					continue;

				if(ProjectorBatchList->Element->BaseMaterialBlending != ProjectorInfo->RenderInfo->MaterialBlendingOp)
					continue;

				if(ProjectorBatchList->Element->BaseMaterial != ProjectorInfo->BaseMaterial)
					continue;

				if(ProjectorBatchList->Element->TwoSided != ProjectorInfo->TwoSided)
					continue;

				if(ProjectorBatchList->Element->VertexBufferSize >= 100000)
					continue;

				ProjectorBatch = ProjectorBatchList->Element;
			}

			// Create a new batch if necessary.

			if(!ProjectorBatch)
			{
				ProjectorBatch = new(GSceneMem) FProjectorRenderBatch(
													ProjectorInfo->BaseMaterial,
													ProjectorInfo->RenderInfo->Material,
													(EProjectorBlending)ProjectorInfo->RenderInfo->MaterialBlendingOp,
													(EProjectorBlending)ProjectorInfo->RenderInfo->FrameBufferBlendingOp,
													ProjectorInfo->TwoSided
													);
				GProjectorBatchList = new(GSceneMem) TList<FProjectorRenderBatch*>(ProjectorBatch,GProjectorBatchList);
			}

			// Add the projector to the batch.

			ProjectorBatch->AddPrimitive(new(GSceneMem) FStaticProjectorPrimitive(ProjectorInfo));
		}
	}

	// Draw Karma collision primitives.

#ifdef WITH_KARMA
	if(StaticMesh->KPhysicsProps && Actor->bBlockKarma)
	{
		McdModelID model = Actor->getKModel();
		if(model)
			KModelDraw(model, KGData->DebugDrawOpt, KLineDraw);
		else
			StaticMesh->KPhysicsProps->Draw(RI, SceneNode->Viewport->Actor->ShowFlags);
	}
#endif

	// If we have a simple collision model, and its being used, draw it.

	if(SceneNode->Viewport->Actor->ShowFlags & SHOW_Collision &&
		StaticMesh->CollisionModel && 
		(StaticMesh->UseSimpleBoxCollision || StaticMesh->UseSimpleLineCollision || StaticMesh->UseSimpleKarmaCollision) )
	{
		UEngine* Engine = SceneNode->Viewport->GetOuterUClient()->Engine;

		RI->PushState();
		RI->SetTransform(TT_LocalToWorld,DynamicActor->LocalToWorld);

		for(INT NodeIndex = 0;NodeIndex < StaticMesh->CollisionModel->Nodes.Num();NodeIndex++)
		{
			FBspNode&	Node = StaticMesh->CollisionModel->Nodes(NodeIndex);

			for(INT VertexIndex = 0;VertexIndex < Node.NumVertices;VertexIndex++)
			{
				FLineBatcher(RI).DrawLine(
					StaticMesh->CollisionModel->Points(StaticMesh->CollisionModel->Verts(Node.iVertPool + VertexIndex).pVertex),
					StaticMesh->CollisionModel->Points(StaticMesh->CollisionModel->Verts(Node.iVertPool + ((VertexIndex + 1) % Node.NumVertices)).pVertex),
					Engine->C_ScaleBoxHi
					);
			}
		}

		RI->PopState();
	}

	unguard;
}