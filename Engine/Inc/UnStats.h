/*=============================================================================
	UnStats.h: Performance statistics utilities.
	Copyright 1997-2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
		* Rewritten multiple times by Andrew Scheidecker and Daniel Vogel
		* Last rewrite by Daniel Vogel for .csv support
=============================================================================*/

#if 1 //NEW
#include "FStatAccumulator.h"
class UFont;
class UCanvas;
class FDefaultUIRenderer;
#endif

enum EStatsType
{
	STATSTYPE_Render		= 0,
	STATSTYPE_Game,
	STATSTYPE_Hardware,
	STATSTYPE_Anim,
	STATSTYPE_Matinee,
	STATSTYPE_Audio,
	STATSTYPE_Net,
	STATSTYPE_MAX
};

enum EStatsDataType
{
	STATSDATATYPE_DWORD		= 0,
	STATSDATATYPE_FLOAT,
	STATSDATATYPE_STRING,
	STATSDATATYPE_MAX
};

enum EStatsUnit
{
	STATSUNIT_Default		= 0,
	STATSUNIT_Combined_Default_MSec,
	STATSUNIT_Combined_Default_Default,
	STATSUNIT_Byte,
	STATSUNIT_KByte,
	STATSUNIT_MByte,
	STATSUNIT_MSec
};

class ENGINE_API FStats
{
public:
	FLOAT	FPSAvg;

	// Init Data.
	UBOOL			Registered[STATSTYPE_MAX];

	// Stats Data.
	TArray<DWORD>	DWORDStats;
	TArray<DWORD>	DWORDOldStats;
	TArray<FString>	DWORDStatsDescriptions;
	TArray<FString>	DWORDStatsSectionDescriptions;

	TArray<FLOAT>	FLOATStats;
	TArray<FLOAT>	FLOATOldStats;
	TArray<FString>	FLOATStatsDescriptions;
	TArray<FString>	FLOATStatsSectionDescriptions;

	TArray<FString>	FSTRINGStats;
	TArray<FString> FSTRINGOldStats;
	TArray<FString>	FSTRINGStatsDescriptions;
	TArray<FString>	FSTRINGStatsSectionDescriptions;

	struct FSingleStatsInfo
	{
		INT				Index;
		EStatsDataType	DataType;
		EStatsUnit		StatsUnit;
	};

	TArray<FSingleStatsInfo> Stats[STATSTYPE_MAX];

	struct FMovingAverage
	{		
		INT CycIndex;
		INT CycBufferSize;
		INT Count;
		EStatsDataType	DataType;
		TArray<DWORD>   Samples;
	};

	TArray<FMovingAverage> Averages;

	UBOOL bShowAssetMemStats; // Shows asset specific mem stats - // SL MODIFY
	UBOOL bShowMemStats;
	TArray<FSingleStatsInfo> AllStats;

	class FCountAssetMem* MemCount;
	UObject*  CheckObject;
	UObject*  OldCheckObject;

#if 1 //NEW (mdf) Legend stats
	INT	PosX, PosY;		// current text position.
	INT	RowStart;		// starting sectional X position.
	INT RowHeight;		// current section (Y) start position.
	INT	NumLines;		// sectional line counter.
	INT MaxLines;		// maximum number of lines in total section.

	DWORD Frame_StatsCycles; // stat overhead timing

	INT NumFrameSamples;
	FLOAT TotalFrameRate;
	
	// circular list for tracking avg frame time over last X seconds
	FLOAT FrameTimeSamples[ 120 ];
	INT ListStart;
	INT ListEnd;
	INT ListCount;
	FLOAT TotalFrameTime;
	FLOAT MinLastSec;
	FLOAT MaxLastSec;

	// stats which are accumulated (show min, max, avg since last cleared)
	FStatAccumulator
		Frame_CyclesA,
		Frame_FPSA,
		Frame_StatsCyclesA,
		Frame_Temp1CyclesA,
		Frame_Temp2CyclesA,
		Frame_Temp3CyclesA,
		Frame_UnknownCyclesA,
		
		Game_ActorTickCyclesA,
		Game_AnimationCyclesA,
		Game_CyclesA,
		Game_HearCyclesA,
		Game_KarmaCyclesA,
		Game_MoveCyclesA,
		Game_CScriptCyclesA,
		Game_PathCyclesA,
		Game_ScriptCyclesA,
		Game_ScriptTickCyclesA,
		Game_SeeCyclesA,
		Game_PhysicsCyclesA,
		Game_TickCyclesA,
		Game_TimerCyclesA,
		Game_TraceCyclesA,
		Game_TraceActorCyclesA,
		Game_TraceLevelCyclesA,
		Game_TraceTerrainCyclesA,

		Particle_CyclesA,
		Particle_DrawCyclesA,

		Render_BSPCyclesA,
		Render_CyclesA,
		Render_DecoLayerCyclesA,
		Render_MeshCyclesA,
		Render_LightingCyclesA,
		Render_OverlayCyclesA,
		Render_ProjectorCyclesA,
		Render_StaticMeshCyclesA,
		Render_TerrainCyclesA,
		Render_UnknownCyclesA,
		Render_VisibilityCyclesA,
		
		UI_TotalCyclesA;
#endif

	// Interface.
	FStats();
	virtual ~FStats();
	INT RegisterStats( EStatsType Type, EStatsDataType DataType, FString Description, FString SectionDescription, EStatsUnit StatsUnit );
	void Render(class UViewport* Viewport, class UEngine* Engine);
	void UpdateString( FString& Result, UBOOL Descriptions );
	void Clear();
	void CalcMovingAverage( INT StatIndex, DWORD CycleSize );

#if 1 //NEW (mdf) Legend stats
	inline void Header( FDefaultUIRenderer& Ren, UFont* StatFont, TCHAR* HeaderStr, UCanvas* Canvas );
	inline void AccumulateStat( FLOAT Val, FStatAccumulator& Acc, UBOOL bAccum );
	inline void DrawAccumulatedStat( FDefaultUIRenderer& Ren, UFont* StatFont, TCHAR* Str, FLOAT Val, FStatAccumulator& Acc, FLOAT FrameTime, FLOAT Budget=-1.0f, UBOOL bRaw=false, TCHAR* AppendStr=NULL );
	void RenderStatQuick( UViewport* Viewport, UEngine* Engine );
	void RenderFrameRate( UViewport* Viewport, UEngine* Engine );
	void ResetFrameTimeWindow();
	void ClearStats();
#endif
};

class ENGINE_API FEngineStats
{
public:
	INT		STATS_FirstEntry,
		
#if 1 //NEW (mdf) Legend stats
			STATS_Batch_Batches,
			STATS_Batch_Primitives,
			STATS_Batch_RenderCycles,

			STATS_BSP_CollisionCycles,
			STATS_BSP_DynamicLightingCycles,
			STATS_BSP_DynamicLights,
			STATS_BSP_Nodes,
			STATS_BSP_RenderCycles,
			STATS_BSP_Sections,
			STATS_BSP_Triangles,

			STATS_Corona_CollisionCycles,

			STATS_DecoLayer_Decorations,
			STATS_DecoLayer_RenderCycles,
			STATS_DecoLayer_Triangles,

			STATS_Fluid_RenderCycles,
			STATS_Fluid_SimulateCycles,
			STATS_Fluid_VertexGenCycles,

			STATS_Frame_RenderCycles,
			STATS_Frame_RenderOverlayCycles,	// first-person weapon overhead timing
			STATS_Frame_Temp1Cycles, 
			STATS_Frame_Temp2Cycles, 
			STATS_Frame_Temp3Cycles, 
			STATS_Frame_TotalCycles,

			STATS_Game_ActorTickCycles,
			STATS_Game_AnimationCycles,
			STATS_Game_AudioTickCycles,
			STATS_Game_CanvasCycles,
			STATS_Game_CleanupDestroyedCycles,
			STATS_Game_CScriptCycles, 
			STATS_Game_FindPathCycles,
			STATS_Game_HearCycles, 
			STATS_Game_HUDPostRender,
			STATS_Game_InteractionPostRender,
			STATS_Game_InteractionPreRender,
			STATS_Game_MLCheckCycles,
			STATS_Game_MoveCycles,
			STATS_Game_MPCheckCycles,
			STATS_Game_NetTickCycles,
			STATS_Game_NumMoves,
			STATS_Game_PhysicsCycles,
			STATS_Game_ScriptCycles,
			STATS_Game_ScriptDebugTime,
			STATS_Game_ScriptTickCycles,
			STATS_Game_SeePlayerCycles,
			STATS_Game_SpawningCycles,
			STATS_Game_TraceActorCycles,
			STATS_Game_TraceCycles, 
			STATS_Game_TraceLevelCycles, 
			STATS_Game_TraceTerrainCycles, 

			STATS_Game_UnusedCycles,
			STATS_Game_UpdateRenderData,

			STATS_Karma_Collision,
			STATS_Karma_CollisionContactGen,
			STATS_Karma_Dynamics,
			STATS_Karma_physKarma,
			STATS_Karma_physKarma_Con,
			STATS_Karma_physKarmaRagDoll,
			STATS_Karma_RagdollTrilist,
			STATS_Karma_Temp,
			STATS_Karma_TrilistGen,

			STATS_LightMap_Cycles,
			STATS_LightMap_Updates,

			STATS_Matinee_TickCycles,

			STATS_Mesh_DrawCycles,
			STATS_Mesh_LODCycles,
			STATS_Mesh_PoseCycles,
			STATS_Mesh_ResultCycles,
			STATS_Mesh_RigidCycles,
			STATS_Mesh_SkelCycles,
			STATS_Mesh_SkinCycles,

			STATS_Net_Channels,
			STATS_Net_InBunches,
			STATS_Net_InBytes,
			STATS_Net_InPacketLoss,
			STATS_Net_InPackets,
			STATS_Net_InUnordered,
			STATS_Net_NumPV,
			STATS_Net_NumReps,
			STATS_Net_NumRPC,
			STATS_Net_OutBunches,
			STATS_Net_OutBytes,
			STATS_Net_OutPacketLoss,
			STATS_Net_OutPackets,
			STATS_Net_OutUnordered,
			STATS_Net_Ping,
			STATS_Net_Speed,

			STATS_Particle_DrawCycles,
			STATS_Particle_TickCycles,
			STATS_Particle_RadiatorTick,
			STATS_Particle_ForceCycles,
			STATS_Particle_NumForces,
			STATS_Particle_NumParticles,
			STATS_Particle_Flushes,
			STATS_Particle_Projectors,

			STATS_Projector_AttachCycles,
			STATS_Projector_Projectors,
			STATS_Projector_RenderCycles,
			STATS_Projector_Triangles,

			STATS_StaticMesh_BatchedRenderCycles,
			STATS_StaticMesh_BatchedSortCycles,
			STATS_StaticMesh_BatchedSortedSections,
			STATS_StaticMesh_BatchedSortedTriangles,
			STATS_StaticMesh_BatchedUnsortedSections,
			STATS_StaticMesh_BatchedUnsortedTriangles,
			STATS_StaticMesh_Batches,
			STATS_StaticMesh_CollisionCacheFlushes,
			STATS_StaticMesh_CollisionCacheHits,
			STATS_StaticMesh_CollisionCacheMisses,
			STATS_StaticMesh_CollisionCycles,
			STATS_StaticMesh_RenderCycles,
			STATS_StaticMesh_Triangles,
			STATS_StaticMesh_UnbatchedRenderCycles,
			STATS_StaticMesh_UnbatchedSortCycles,
			STATS_StaticMesh_UnbatchedSortedSections,
			STATS_StaticMesh_UnbatchedSortedTriangles,
			STATS_StaticMesh_UnbatchedUnsortedSections,
			STATS_StaticMesh_UnbatchedUnsortedTriangles,

			STATS_Stats_RenderCycles,

			STATS_Stencil_Nodes,
			STATS_Stencil_RenderCycles,
			STATS_Stencil_Triangles,

			STATS_Terrain_CollisionCycles,
			STATS_Terrain_DrawPrimitives,
			STATS_Terrain_RenderCycles,
			STATS_Terrain_Sectors,
			STATS_Terrain_Triangles,

			STATS_Visibility_BoxRejects,
			STATS_Visibility_BoxTests,
			STATS_Visibility_MaskRejects,
			STATS_Visibility_MaskTests,
			STATS_Visibility_MeshLightCycles,
			STATS_Visibility_ScratchBytes,
			STATS_Visibility_SetupCycles,
			STATS_Visibility_TraverseCycles,
			
			STATS_UI_GameCycles,
			STATS_UI_RenderCycles,
#else
/*
			STATS_Frame_TotalCycles,
			STATS_Frame_RenderCycles,

			STATS_Karma_Collision,
			STATS_Karma_CollisionContactGen,
			STATS_Karma_TrilistGen,
			STATS_Karma_RagdollTrilist,
			STATS_Karma_Dynamics,
			STATS_Karma_physKarma,
			STATS_Karma_physKarma_Con,
			STATS_Karma_physKarmaRagDoll,
			STATS_Karma_Temp,

			STATS_BSP_RenderCycles,
			STATS_BSP_Sections,
			STATS_BSP_Nodes,
			STATS_BSP_Triangles,
			STATS_BSP_DynamicLightingCycles,
			STATS_BSP_DynamicLights,
			STATS_BSP_CollisionCycles,

			STATS_LightMap_Updates,
			STATS_LightMap_Cycles,

			STATS_Projector_RenderCycles,
			STATS_Projector_AttachCycles,
			STATS_Projector_Projectors,
			STATS_Projector_Triangles,
			
			STATS_Stencil_RenderCycles,
			STATS_Stencil_Nodes,
			STATS_Stencil_Triangles,

			STATS_Visibility_SetupCycles,
			STATS_Visibility_MaskTests,
			STATS_Visibility_MaskRejects,
			STATS_Visibility_BoxTests,
			STATS_Visibility_BoxRejects,
			STATS_Visibility_TraverseCycles,
			STATS_Visibility_ScratchBytes,
			STATS_Visibility_MeshLightCycles,

			STATS_Terrain_RenderCycles,
			STATS_Terrain_CollisionCycles,
			STATS_Terrain_Sectors,
			STATS_Terrain_Triangles,
			STATS_Terrain_DrawPrimitives,

			STATS_DecoLayer_RenderCycles,
			STATS_DecoLayer_Triangles,
			STATS_DecoLayer_Decorations,

			STATS_Matinee_TickCycles,

			STATS_Mesh_SkinCycles,
			STATS_Mesh_ResultCycles,
			STATS_Mesh_LODCycles,
			STATS_Mesh_SkelCycles,
			STATS_Mesh_PoseCycles,
			STATS_Mesh_RigidCycles,
			STATS_Mesh_DrawCycles,
	
			STATS_StaticMesh_Batches,
			STATS_StaticMesh_BatchedSortedSections,
			STATS_StaticMesh_BatchedSortedTriangles,
			STATS_StaticMesh_BatchedSortCycles,
			STATS_StaticMesh_BatchedUnsortedSections,
			STATS_StaticMesh_BatchedUnsortedTriangles,
			STATS_StaticMesh_BatchedRenderCycles,
			STATS_StaticMesh_UnbatchedSortedSections,
			STATS_StaticMesh_UnbatchedSortedTriangles,
			STATS_StaticMesh_UnbatchedSortCycles,
			STATS_StaticMesh_UnbatchedUnsortedSections,
			STATS_StaticMesh_UnbatchedUnsortedTriangles,
			STATS_StaticMesh_UnbatchedRenderCycles,
			STATS_StaticMesh_Triangles,
			STATS_StaticMesh_RenderCycles,
			STATS_StaticMesh_CollisionCycles,
			STATS_StaticMesh_CollisionCacheHits,
			STATS_StaticMesh_CollisionCacheMisses,
			STATS_StaticMesh_CollisionCacheFlushes,

			STATS_Batch_Batches,
			STATS_Batch_Primitives,
			STATS_Batch_RenderCycles,

			STATS_Stats_RenderCycles,

			STATS_Game_ScriptCycles,
			STATS_Game_ScriptTickCycles,
			STATS_Game_ActorTickCycles,
			STATS_Game_FindPathCycles,
			STATS_Game_SeePlayerCycles,
			STATS_Game_SpawningCycles,
			STATS_Game_AudioTickCycles,
			STATS_Game_CleanupDestroyedCycles,
			STATS_Game_UnusedCycles,
			STATS_Game_NetTickCycles,
			STATS_Game_CanvasCycles,
			STATS_Game_PhysicsCycles,
			STATS_Game_MoveCycles,
			STATS_Game_NumMoves,
			STATS_Game_MLCheckCycles,
			STATS_Game_MPCheckCycles,
			STATS_Game_UpdateRenderData,
			STATS_Game_HUDPostRender,
			STATS_Game_InteractionPreRender,
			STATS_Game_InteractionPostRender,
			STATS_Game_ScriptDebugTime,
			STATS_Fluid_SimulateCycles,
			STATS_Fluid_VertexGenCycles,
			STATS_Fluid_RenderCycles,

			STATS_Corona_CollisionCycles,

			STATS_Net_Ping,
			STATS_Net_Channels,
			STATS_Net_InUnordered,
			STATS_Net_OutUnordered,
			STATS_Net_InPacketLoss,
			STATS_Net_OutPacketLoss,
			STATS_Net_InPackets,
			STATS_Net_OutPackets,
			STATS_Net_InBunches,
			STATS_Net_OutBunches,
			STATS_Net_InBytes,
			STATS_Net_OutBytes,
			STATS_Net_Speed,
			STATS_Net_NumReps,
			STATS_Net_NumRPC,
			STATS_Net_NumPV,
*/
#endif
			STATS_LastEntry;

	FEngineStats();
	void Init();
};
