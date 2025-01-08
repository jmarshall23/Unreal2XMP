/*=============================================================================
	UnStats.cpp: In game performance statistics utilities.
	Copyright 1997-2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Andrew Scheidecker
		* Rewritten multiple times by Andrew Scheidecker and Daniel Vogel
		* Last rewrite by Daniel Vogel for .csv support
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

#if 1 //NEW (mdf) Legend stats
#include "..\..\UI\Inc\UnUIRender.h"
#include "FTimerManager.h"
EXECVAR( FLOAT, MinFPS, -1.0f );
EXECVAR( UBOOL, bStatAdjust, true );
#endif

class FCountAssetMem : public FArchive
{
public:
	FCountAssetMem(UObject* InObject)
		: Num(0), Max(0), StaticNum(0), StaticMax(0)
		, Context(InObject)
		, FirstObject(1)
	{
		guard(FArchiveTagUsed::FArchiveTagUsed);

		// Tag all objects as unreachable.
		for( FObjectIterator It; It; ++It )
			It->SetFlags( RF_Unreachable );

		*this << Context;

		for( FObjectIterator It; It; ++It )
			It->ClearFlags( RF_Unreachable );

		unguard;
	}
	SIZE_T GetNum() { return Num; }
	SIZE_T GetMax()	{ return Max; }

	FArchive& operator<<( UObject*& Object )
	{
		guardSlow(FArchiveTagUsed<<Obj);

		// Object could be a misaligned pointer.
		// Copy the contents of the pointer into a temporary and work on that.
		UObject* Obj;
		appMemcpy(&Obj, &Object, sizeof(INT));
		
		UBOOL BadType = true;

		if (!Obj)
			return *this;
		if (
			Obj->IsA(UStaticMeshInstance::StaticClass()) || 
			Obj->IsA(USkeletalMeshInstance::StaticClass()) || 
			0)
		{
			BadType = false;
			IsStatic = false;
		}
		if (
			Obj->IsA(USkeletalMesh::StaticClass()) || 
			Obj->IsA(UMeshAnimation::StaticClass()) || 
			Obj->GetClass()->IsChildOf(UMaterial::StaticClass()) || 
			Obj->IsA(UStaticMesh::StaticClass()) || 
			FirstObject)
		{
			BadType = false;
			IsStatic = true;
		}

		if (BadType)
			return *this;

		FirstObject = false;

		if( Obj && (Obj->GetFlags() & RF_Unreachable) )
		{
			// Only recurse the first time object is claimed.
			Obj->ClearFlags( RF_Unreachable );

			// Recurse.
			UObject* OldContext = Context;
			Context = Object;
			Obj->Serialize( *this );
			Context = OldContext;
		}

		return *this;
		unguardSlow;
	}
	FArchive& operator<<( FName& Name )
	{
		return *this;
	}
	struct FItem
	{
		UClass*	Class;
		SIZE_T	Num, Max;
		FItem( UClass* InClass=NULL )
		: Class(InClass), Num(0), Max(0)
		{}
		void Add( INT InNum, INT InMax )
		{Num+=InNum; Max+=InMax;}
	};
	void CountBytes( SIZE_T InNum, SIZE_T InMax )
	{
		TArray<FItem> &WhichClassCounts = (IsStatic ? StaticClassCounts : ClassCounts);
		INT ClassCountIndex=0;
		INT i;
		for (i=0; i<WhichClassCounts.Num(); i++)
			if (WhichClassCounts(i).Class == Context->GetClass())
			{
				break;
			}
		ClassCountIndex = i;
		if (i == WhichClassCounts.Num())
		{
			WhichClassCounts.AddZeroed(1);
			WhichClassCounts(ClassCountIndex).Class = Context->GetClass();
		}
		WhichClassCounts(ClassCountIndex).Add(InNum, InMax);

		if (IsStatic)
		{
			StaticNum += InNum;
			StaticMax += InMax;
		}
		else
		{
			Num += InNum;
			Max += InMax;
		}
	}
public:
	UObject* Context;
	TArray<FItem> StaticClassCounts;
	TArray<FItem> ClassCounts;
	SIZE_T StaticNum, StaticMax;
	SIZE_T Num, Max;
	UBOOL FirstObject, IsStatic;
};
// Global stats.
FStats GStats;
FEngineStats GEngineStats;

FStats::FStats()
{
	for( INT i=0; i<STATSTYPE_MAX; i++ )
		Registered[i] = 0;
	MemCount = NULL;
	CheckObject = NULL;
	OldCheckObject = NULL;
#if 1 //NEW (mdf) Legend stats
	ResetFrameTimeWindow();
#endif	
}

FStats::~FStats()
{
	if(MemCount) delete(MemCount);
}

INT FStats::RegisterStats( EStatsType Type, EStatsDataType DataType, FString Description, FString SectionDescription, EStatsUnit StatsUnit )
{
	guard(FStats::RegisterStats);
	INT Index		= -1;
	switch( DataType )
	{
	case STATSDATATYPE_DWORD:
		Index = DWORDStats.Add( 1 );
		DWORDOldStats.Add( 1 );
		new(DWORDStatsDescriptions)FString(Description);
		new(DWORDStatsSectionDescriptions)FString(SectionDescription + TEXT(" ") + Description);
		break;
	case STATSDATATYPE_FLOAT:
		Index = FLOATStats.Add( 1 );
		FLOATOldStats.Add( 1 );
		new(FLOATStatsDescriptions)FString(Description);
		new(FLOATStatsSectionDescriptions)FString(SectionDescription + TEXT(" ") + Description);
		break;
	case STATSDATATYPE_STRING:
		Index = FSTRINGStats.Add( 1 );
		FSTRINGOldStats.Add( 1 );
		new(FSTRINGStatsDescriptions)FString(Description);
		new(FSTRINGStatsSectionDescriptions)FString(SectionDescription + TEXT(" ") + Description);
		break;
	}

	FSingleStatsInfo SI;
	SI.Index		= Index;
	SI.DataType		= DataType; 
	SI.StatsUnit	= StatsUnit;

	AllStats.AddItem( SI );

	Stats[Type].AddItem( SI );

	return Index;
	unguard;
}

void FStats::UpdateString( FString& Result, UBOOL Descriptions )
{
	guard(FStats::UpdateString);
	DWORD UpdateCycles = 0;
	clock(UpdateCycles);
	if( Descriptions )
	{
		for( INT i=0; i<STATSTYPE_MAX; i++ )
		{
			for( INT n=0; n<Stats[i].Num(); n++ )
			{
				Result += FString::Printf(TEXT("%s,"), *DWORDStatsSectionDescriptions(Stats[i](n).Index) );
			}
		}
	}
	else
	{
		for( INT i=0; i<STATSTYPE_MAX; i++ )
		{
			for( INT n=0; n<Stats[i].Num(); n++ )
			{
				switch( Stats[i](n).DataType )
				{
				case STATSDATATYPE_DWORD:
					{
						DWORD Value = DWORDStats(Stats[i](n).Index);
						switch( Stats[i](n).StatsUnit )
						{
						case STATSUNIT_Default:
						case STATSUNIT_Combined_Default_MSec:
						case STATSUNIT_Combined_Default_Default:
						case STATSUNIT_Byte:
						case STATSUNIT_KByte:
						case STATSUNIT_MByte:
							Result += FString::Printf(TEXT("%u,"), Value );
							break;
						case STATSUNIT_MSec:
							Result += FString::Printf(TEXT("%f,"), (FLOAT) (Value * GSecondsPerCycle * 1000.0f) );
							break;
						}
						break;
					}
				case STATSDATATYPE_FLOAT:
				case STATSDATATYPE_STRING:
					appErrorf(TEXT("Implement me!"));
				}
			}
		}
	}
	unclock(UpdateCycles);
	
	// Stat update time.
	if( Descriptions )
		Result += TEXT("Stats Update");
	else
		Result += FString::Printf(TEXT("%f"), (FLOAT) (UpdateCycles * GSecondsPerCycle * 1000.0f) );
	Result += TEXT("\n");

	unguard;
}

//NOTE (mdf): called every frame from CMainLoop::RunLoop prior to ticking the engine
void FStats::Clear()
{
	guard(FStats::Clear);

	// Update old stats.
	if( DWORDStats.Num() )
		appMemcpy( &DWORDOldStats(0), &DWORDStats(0), DWORDStats.Num() * sizeof(DWORD) );
	if( FLOATStats.Num() )
		appMemcpy( &FLOATOldStats(0), &FLOATStats(0), FLOATStats.Num() * sizeof(FLOAT) );
	for( INT i=0; i<FSTRINGStats.Num(); i++ )
		FSTRINGOldStats(i) = FSTRINGStats(i);

	// Clear new ones.
	if( DWORDStats.Num() )
		appMemzero( &DWORDStats(0), DWORDStats.Num() * sizeof(DWORD) );
	if( FLOATStats.Num() )
		appMemzero( &FLOATStats(0), FLOATStats.Num() * sizeof(FLOAT) );
	for( INT i=0; i<FSTRINGStats.Num(); i++ )
		FSTRINGStats(i) = TEXT("");

#if 1 //NEW (mdf) Legend stats
	FTimerManager::FTimerCycles = 0;
#endif

	unguard;
}


void FStats::CalcMovingAverage( INT StatIndex, DWORD CycleSize )
{
#define DISCARDFIRST 3 
	// Automatic custom-size cyclic running average for MSEC timing parameters.

#if 1 //NEW (mdf)
	DWORD* LatestCycles = &(DWORDStats( StatIndex ));
#else
  	DWORD* LatestCycles = &(GStats.DWORDStats( StatIndex ));
#endif

	INT NewElements = (StatIndex+1) -  Averages.Num();
	if( NewElements > 0 )
		Averages.AddZeroed( NewElements );	

	if( Averages(StatIndex).CycBufferSize != (INT)CycleSize )
	{
		Averages(StatIndex).CycBufferSize = CycleSize;
		Averages(StatIndex).Samples.Empty();
		Averages(StatIndex).Samples.AddZeroed( CycleSize );
		Averages(StatIndex).Count = 0;
	}
	
	Averages(StatIndex).Count = Min( Averages(StatIndex).CycBufferSize + DISCARDFIRST, Averages(StatIndex).Count + 1 );

	if( Averages(StatIndex).Count > DISCARDFIRST )
	{
		// Store and compute running average.
		INT TotalElements = Averages(StatIndex).Count - DISCARDFIRST;
				
		Averages(StatIndex).Samples( Averages(StatIndex).CycIndex ) = *LatestCycles;
		SQWORD DAv = 0;
		for( INT i=0; i< TotalElements; i++ )
			DAv += Averages(StatIndex).Samples(i);

		Averages(StatIndex).CycIndex = ( Averages(StatIndex).CycIndex + 1) % Averages(StatIndex).CycBufferSize;
		*LatestCycles = (INT)(DAv/(SQWORD)TotalElements);
	}		
}


#define MAYBE_WRAP				if( (Canvas->CurY + 32) >= Viewport->SizeY ) {CurX += 200; Canvas->CurY = 8;}
#define DRAW_STRING(Arg)		MAYBE_WRAP Canvas->CurX = CurX; Canvas->WrappedPrintf(Canvas->TinyFont,0,*Arg);
#define TIME_PRINTF				FString::Printf(TEXT("%2.2f %s ms")	
#define TIME_ARGUMENTS(Arg)		(FLOAT) (DWORDOldStats(GEngineStats.##Arg) * GSecondsPerCycle * 1000.0f), *DWORDStatsDescriptions(GEngineStats.##Arg) )
#define DWORD_PRINTF			FString::Printf(TEXT("%u %s")	
#define DWORD_ARGUMENTS(Arg)	DWORDOldStats(GEngineStats.##Arg), *DWORDStatsDescriptions(GEngineStats.##Arg) )
#define DRAW_HEADLINE(Arg)		Canvas->Color = FColor(0,255,255); Canvas->CurY += 4; Dummy = ##Arg; DRAW_STRING( Dummy ); Canvas->Color = FColor(0,255,0);

#define DRAW_ALL(Arg)			\
for( INT i=0; i<Stats[##Arg].Num(); i++ )	\
{	\
	FSingleStatsInfo* StatsInfo = &Stats[##Arg](i);	\
	switch( StatsInfo->StatsUnit )	\
	{	\
		case STATSUNIT_Default:	\
		case STATSUNIT_Byte:	\
		{	\
			DRAW_STRING( FString::Printf(TEXT("%u %s"), DWORDOldStats(StatsInfo->Index), *DWORDStatsDescriptions(StatsInfo->Index)) );	\
			break;	\
		}	\
		case STATSUNIT_KByte:	\
		{	\
			DRAW_STRING( FString::Printf(TEXT("%u KByte %s"), DWORDOldStats(StatsInfo->Index) / 1024, *DWORDStatsDescriptions(StatsInfo->Index)) );	\
			break;	\
		}	\
		case STATSUNIT_MByte:	\
		{	\
			DRAW_STRING( FString::Printf(TEXT("%u MByte %s"), DWORDOldStats(StatsInfo->Index) / 1024 / 1024, *DWORDStatsDescriptions(StatsInfo->Index)) );	\
			break;	\
		}	\
		case STATSUNIT_MSec:	\
		{	\
			DRAW_STRING( FString::Printf(TEXT("%2.1f %s ms"), (FLOAT)(DWORDOldStats(StatsInfo->Index) * GSecondsPerCycle * 1000.0f), *DWORDStatsDescriptions(StatsInfo->Index) ));	\
			break;	\
		}	\
		case STATSUNIT_Combined_Default_MSec:	\
		{	\
			FSingleStatsInfo* NextInfo = StatsInfo + 1;	\
			DRAW_STRING( FString::Printf(TEXT("%2.1f %s ms [%u]"), (FLOAT)(DWORDOldStats(NextInfo->Index) * GSecondsPerCycle * 1000.0f), *DWORDStatsDescriptions(StatsInfo->Index), DWORDOldStats(StatsInfo->Index)) );	\
			i++;	\
			break;	\
		}	\
		case STATSUNIT_Combined_Default_Default:	\
		{	\
			FSingleStatsInfo* NextInfo = StatsInfo + 1;	\
			DRAW_STRING( FString::Printf(TEXT("In: %i, Out: %i %s"), DWORDOldStats(StatsInfo->Index), DWORDOldStats(NextInfo->Index), *DWORDStatsDescriptions(StatsInfo->Index) ));	\
			i++;	\
			break;	\
		}	\
	}	\
}

#if 1 //NEW (mdf) Legend stats
//---------------------------------------------------------------------------

// Budgets for 256MB P4/1.5GHz, 64MB GeForce3 (800x600 32bit, Medium Detail)
const FLOAT FrameBudget			= 33.3f;
const FLOAT GameBudget			= 12.0f;
const FLOAT UnknownBudget		=  0.2f;
const FLOAT UIBudget			=  1.5f; // was 0.5f
const FLOAT RenderUnknownBudget	=  0.5f;
const FLOAT RenderBudget		= FrameBudget - (GameBudget + UnknownBudget + UIBudget + RenderUnknownBudget); // what remains

const FLOAT GenericBudget		=  4.0f;

const INT LINEHEIGHT=8;			// vertical space between lines.
const INT SECTIONSPACE=170;		// horizontal space between sections.
const INT ROWSPACE=8;			// vertical space between rows.
				
#define BUDGET_COLOR(a,b)	((a)>=(b*1.5)) ? FColor(255,0,0) : (a>=b)     ? FColor(255,255,0) : FColor(0,255,0)
#define NEW_ROW				RowHeight+=(MaxLines*LINEHEIGHT)+ROWSPACE;NumLines=MaxLines=0;PosX=RowStart-SECTIONSPACE;
#define NEWLINE				MaxLines = Max( ++NumLines, MaxLines ); PosY += LINEHEIGHT;
#define STATFONT			Canvas->DebugFont
	
//---------------------------------------------------------------------------

inline void FStats::Header( FDefaultUIRenderer& Ren, UFont* StatFont, TCHAR* HeaderStr, UCanvas* Canvas )
{
	NumLines = 1;
	PosX += SECTIONSPACE;
	
	if( PosX > Canvas->Viewport->SizeX )
	{
		NEW_ROW
	}
	
	PosY = RowHeight;
	Ren.DrawString( HeaderStr, PosX, PosY, StatFont, 0, PF_AlphaTexture, FColor(255,255,255) );
}

//---------------------------------------------------------------------------

inline void FStats::AccumulateStat( FLOAT Val, FStatAccumulator& Acc, UBOOL bAccum )
{
	if( bAccum )
		Acc.AddStat( Val );
}

//---------------------------------------------------------------------------

inline void FStats::DrawAccumulatedStat( FDefaultUIRenderer& Ren, UFont* StatFont, TCHAR* Str, FLOAT Val, FStatAccumulator& Acc, FLOAT FrameTime, FLOAT Budget, UBOOL bRaw, TCHAR* AppendStr )
{
	NEWLINE
	
	TCHAR TempStr[256];
	FLOAT Avg = Acc.GetStatsAvg();
	FLOAT Pct = (Val/FrameTime)*100.0f;

	FLOAT Multiplier = 1.0f;
	if( !bRaw )
		Multiplier = GSecondsPerCycle*1000.0f;
	
	if( Budget > 100.0f )
		Pct = 0.0f;

	appSprintf( TempStr, TEXT("%6.1f%% %7.2f %7.2f %7.2f %7.2f %s%s"), 
		Pct, 
		Multiplier*Acc.GetStatsMin(), 
		Multiplier*Acc.GetStatsMax(), 
		Multiplier*Avg, 
		Multiplier*Val, 
		Str,
		AppendStr ? AppendStr : L"" );

	if( Budget < 0.0f )
		Budget = GenericBudget;
			
	Ren.DrawString( TempStr, PosX, PosY, StatFont, 0, PF_AlphaTexture, BUDGET_COLOR( Val*Multiplier, Budget ) );
}

//---------------------------------------------------------------------------

void FStats::RenderStatQuick( class UViewport* Viewport, class UEngine* Engine )
{
	//debugf( L"STATS_Frame_TotalCycles cur: %d", DWORDStats(GEngineStats.STATS_Frame_TotalCycles) );
	//debugf( L"STATS_Frame_TotalCycles old: %d", DWORDOldStats(GEngineStats.STATS_Frame_TotalCycles) );
	// set raw stats
	DWORD	Frame_Cycles				= DWORDOldStats(GEngineStats.STATS_Frame_TotalCycles);
	FLOAT	Frame_FPS					= 1.f / (Frame_Cycles * GSecondsPerCycle);
	DWORD	Frame_Temp1Cycles			= DWORDOldStats(GEngineStats.STATS_Frame_Temp1Cycles);
	DWORD	Frame_Temp2Cycles			= DWORDOldStats(GEngineStats.STATS_Frame_Temp2Cycles);
	DWORD	Frame_Temp3Cycles			= DWORDOldStats(GEngineStats.STATS_Frame_Temp3Cycles);
	DWORD	Game_ActorTickCycles    	= DWORDOldStats(GEngineStats.STATS_Game_ActorTickCycles);
	DWORD	Game_AnimationCycles    	= DWORDOldStats(GEngineStats.STATS_Game_AnimationCycles);
	DWORD	Game_Cycles					= Engine->GameCycles;
	DWORD	Game_HearCycles    			= DWORDOldStats(GEngineStats.STATS_Game_HearCycles);
	//DWORD	Game_KarmaCycles    		= Karma_CollisionFar + Karma_CollisionNear + Karma_Dynamics;
	DWORD	Game_MoveCycles				= DWORDOldStats(GEngineStats.STATS_Game_MoveCycles);
	DWORD	Game_PathCycles    			= DWORDOldStats(GEngineStats.STATS_Game_FindPathCycles);
	DWORD	Game_CScriptCycles  		= DWORDOldStats(GEngineStats.STATS_Game_CScriptCycles);
	DWORD	Game_PhysicsCycles    		= DWORDOldStats(GEngineStats.STATS_Game_PhysicsCycles);
	DWORD	Game_ScriptCycles  			= DWORDOldStats(GEngineStats.STATS_Game_ScriptCycles);
	DWORD	Game_ScriptTickCycles		= DWORDOldStats(GEngineStats.STATS_Game_ScriptCycles);
	DWORD	Game_SeeCycles     			= DWORDOldStats(GEngineStats.STATS_Game_SeePlayerCycles);
	DWORD	Game_TimerCycles			= FTimerManager::FTimerCycles;
	DWORD	Game_TraceCycles			= DWORDOldStats(GEngineStats.STATS_Game_TraceCycles);
	DWORD	Game_TraceActorCycles		= DWORDOldStats(GEngineStats.STATS_Game_TraceActorCycles);
	DWORD	Game_TraceLevelCycles		= DWORDOldStats(GEngineStats.STATS_Game_TraceLevelCycles);
	DWORD	Game_TraceTerrainCycles		= DWORDOldStats(GEngineStats.STATS_Game_TraceTerrainCycles);
	DWORD	Particle_DrawCycles			= DWORDOldStats(GEngineStats.STATS_Particle_DrawCycles);
	DWORD	Particle_TickCycles			= DWORDOldStats(GEngineStats.STATS_Particle_TickCycles);
	DWORD	Particle_ForceCycles		= DWORDOldStats(GEngineStats.STATS_Particle_ForceCycles);
	DWORD	Particle_Cycles				= Particle_TickCycles + Particle_ForceCycles;
	DWORD	Render_BSPCycles			= DWORDOldStats(GEngineStats.STATS_BSP_RenderCycles);
	DWORD	Render_Cycles				= Engine->ClientCycles;
	DWORD	Render_DecoLayerCycles		= DWORDOldStats(GEngineStats.STATS_DecoLayer_RenderCycles);
	DWORD	Render_LightingCycles		= DWORDOldStats(GEngineStats.STATS_Visibility_MeshLightCycles);
	DWORD	Render_MeshCycles			= DWORDOldStats(GEngineStats.STATS_Mesh_DrawCycles) + //mdf: this seems to be adding up to more than it should
											DWORDOldStats(GEngineStats.STATS_Mesh_LODCycles) + 
											DWORDOldStats(GEngineStats.STATS_Mesh_PoseCycles) + 
											DWORDOldStats(GEngineStats.STATS_Mesh_ResultCycles) + 
											DWORDOldStats(GEngineStats.STATS_Mesh_RigidCycles) + 
											DWORDOldStats(GEngineStats.STATS_Mesh_SkelCycles) + 
											DWORDOldStats(GEngineStats.STATS_Mesh_SkinCycles);
	DWORD	Render_OverlayCycles		= DWORDOldStats(GEngineStats.STATS_Frame_RenderOverlayCycles);
	DWORD	Render_ProjectorCycles		= DWORDOldStats(GEngineStats.STATS_Projector_AttachCycles) +
											DWORDOldStats(GEngineStats.STATS_Projector_Projectors) +
											DWORDOldStats(GEngineStats.STATS_Projector_RenderCycles) +
											DWORDOldStats(GEngineStats.STATS_Projector_Triangles);
	
	DWORD	Render_StaticMeshCycles		= DWORDOldStats(GEngineStats.STATS_StaticMesh_RenderCycles);
	DWORD	Render_TerrainCycles		= DWORDOldStats(GEngineStats.STATS_Terrain_RenderCycles);
	DWORD	Render_VisibilityCycles		= DWORDOldStats(GEngineStats.STATS_Visibility_SetupCycles) + 
											DWORDOldStats(GEngineStats.STATS_Visibility_TraverseCycles) + 
											DWORDOldStats(GEngineStats.STATS_Game_UpdateRenderData);
	DWORD	UI_GameCycles				= DWORDOldStats(GEngineStats.STATS_UI_GameCycles);
	DWORD	UI_RenderCycles				= DWORDOldStats(GEngineStats.STATS_UI_RenderCycles);
	DWORD	UI_TotalCycles				= UI_GameCycles + UI_RenderCycles;
	
//	DWORD	Render_Other_Cycles			= -1; //Projector_RenderCycles + Stencil_RenderCycles + Stencil_SkyCycles + ShadowCycles;
	INT	Render_UnknownCycles			= Render_Cycles
											- Render_BSPCycles
											- Render_DecoLayerCycles
											- Render_LightingCycles
											- Render_MeshCycles
											//- Render_OtherCycles
											- Render_StaticMeshCycles
											- Render_TerrainCycles
											- Render_OverlayCycles
											- Render_VisibilityCycles
											- Particle_DrawCycles
											- UI_RenderCycles
											;

	if( !bStatAdjust )
		Render_UnknownCycles -= Frame_StatsCycles;

	if( Render_UnknownCycles < 0 )
		Render_UnknownCycles = 0; // probably due to taking stats off?

	/*		
	if( Render_UnknownCycles > 4*1024*1024 )
	{
	debugf( L"Render_UnknownCycles: %9d  Render_Cycles: %9d  Render_BSPCycles: %9d  Render_DecoLayerCycles: %9d  Render_LightingCycles: %9d  Render_MeshCycles: %9d  Render_StaticMeshCycles: %9d  Render_TerrainCycles: %9d  Render_OverlayCycles: %9d  Render_VisibilityCycles: %9d  UI_RenderCycles: %9d",
				Render_UnknownCycles,
				Render_Cycles,
				Render_BSPCycles,
				Render_DecoLayerCycles,
				Render_LightingCycles,
				Render_MeshCycles,
				Render_StaticMeshCycles,
				Render_TerrainCycles,
				Render_OverlayCycles,
				Render_VisibilityCycles,
				UI_RenderCycles );
	}
	*/
	
	INT	Frame_UnknownCycles	= Frame_Cycles - Game_Cycles - Render_Cycles;
	if( !bStatAdjust )
		Frame_UnknownCycles -= Frame_StatsCycles;

	TCHAR TimerCountStr[256];
	appSprintf( TimerCountStr, L"x(%d)", FTimerManager::NumTimers() );

	//debugf( L"FPS: ", FPS );
	//debugf( L"T: %d  TA: %d  TL: %d  TT: %d", TraceCycles, TraceActorCycles, TraceLevelCycles, TraceTerrainCycles );
	//debugf( L"GEngineStats.STATS_Game_SeePlayerCycles: %d  SeeCycles: %d  SeeCyclesA: %0.3f  GSecondsPerCycle*1000.0f: %0.3f  FrameTime: %0.3f", DWORDOldStats(GEngineStats.STATS_Game_SeePlayerCycles), SeeCycles, SeeCyclesA.GetStatsAvg(), GSecondsPerCycle*1000.0f, FrameTime );
	//debugf( L"NumTimers: %d", FTimerManager::NumTimers() );	
	//debugf( L"GEngineStats.STATS_Game_PhysicsCycles: %d", DWORDOldStats(GEngineStats.STATS_Game_PhysicsCycles) );
	//debugf( L"Frame_Cycles: %lu  FrameSeconds: %0.3f  FrameTime: %0.3f (%d)", Frame_Cycles, Frame_Cycles * GSecondsPerCycle*1000.f, FrameTime, DWORDOldStats(GEngineStats.STATS_Frame_TotalCycles) );

	// display stats
	PosX		= 0;
	PosY		= 0;
	RowStart	= 16;
	RowHeight	= 96;
	NumLines	= 0;
	MaxLines	= 0;

	UCanvas* Canvas	= Viewport->Canvas;
	FDefaultUIRenderer Ren(Canvas->Viewport);

	FLOAT FrameTime = Frame_Cycles;
	UBOOL bAccum = !(Engine->StatFlags & STAT_Lock); // accummulate stats unless locked
	
	#define HEADER( HeaderStr )\
		Header( Ren, STATFONT, TEXT(HeaderStr), Canvas );
		
	#define PROCESS_STAT1(label, stat)\
		AccumulateStat( stat, stat##A, bAccum );\
		DrawAccumulatedStat( Ren, STATFONT, TEXT(label), stat, stat##A, FrameTime );
	
	#define PROCESS_STAT2(label, stat, budget, bRaw, AppendStr )\
		AccumulateStat( stat, stat##A, bAccum );\
		DrawAccumulatedStat( Ren, STATFONT, TEXT(label), stat, stat##A, FrameTime, budget, bRaw, AppendStr );

	NEW_ROW
	HEADER( "   Pct      Min     Max     Avg     Cur" );
	PROCESS_STAT2( "FPS",				Frame_FPS,					999.9f,			true,	L"" );
	PROCESS_STAT2( "frame",				Frame_Cycles,				FrameBudget,	false,	L"" );
	PROCESS_STAT2( "  game",			Game_Cycles,				GameBudget,		false,	L"" );
	PROCESS_STAT1( "    actortick",		Game_ActorTickCycles		);
	PROCESS_STAT1( "      animation",	Game_AnimationCycles		);
//	PROCESS_STAT1( "      karma",		Game_KarmaCycles			);
	PROCESS_STAT1( "      scripttick",	Game_ScriptTickCycles		);
	PROCESS_STAT1( "      cscript",		Game_CScriptCycles			);
	PROCESS_STAT1( "      script",		Game_ScriptCycles			);
	PROCESS_STAT1( "    move",			Game_MoveCycles				);
	PROCESS_STAT1( "    path",			Game_PathCycles				);
	PROCESS_STAT1( "    hear",			Game_HearCycles				);
	PROCESS_STAT1( "    see",			Game_SeeCycles 				);
	PROCESS_STAT1( "    physics",		Game_PhysicsCycles			);
	PROCESS_STAT1( "    trace",			Game_TraceCycles			);
	PROCESS_STAT1( "      terrain",		Game_TraceTerrainCycles		);
	PROCESS_STAT1( "      level",		Game_TraceLevelCycles		);
	PROCESS_STAT1( "      actor",		Game_TraceActorCycles		);
	PROCESS_STAT2( "    ftimers",		Game_TimerCycles,			-1.0f,			false,	TimerCountStr );
	PROCESS_STAT1( "    particletick",	Particle_Cycles,			);
	PROCESS_STAT2( "  render",			Render_Cycles,				RenderBudget,	false,	L"" );
	PROCESS_STAT1( "    mesh",			Render_MeshCycles			);
	PROCESS_STAT1( "    lighting",		Render_LightingCycles		);
	PROCESS_STAT1( "    visibility",	Render_VisibilityCycles		);
	PROCESS_STAT1( "    bsp",			Render_BSPCycles			);
	PROCESS_STAT1( "    staticmesh",	Render_StaticMeshCycles		);
	PROCESS_STAT1( "    terrain",		Render_TerrainCycles		);
	PROCESS_STAT1( "    decolayer",		Render_DecoLayerCycles		);
	PROCESS_STAT1( "    overlay",		Render_OverlayCycles		);
	PROCESS_STAT1( "    projector",		Render_ProjectorCycles		);
	PROCESS_STAT1( "    particle",		Particle_DrawCycles			);
//	PROCESS_STAT1( "    other",			Render_Other_Cycles			);
	PROCESS_STAT2( "    unknown",		Render_UnknownCycles,		999.9f /*Render_UnknownCycles >= 0.0f ? RenderUnknownBudget : 0.0f*/, false, L"" );
	PROCESS_STAT2( "  ui",				UI_TotalCycles,				UIBudget,		false,	L"" );
	PROCESS_STAT2( "  unknown",			Frame_UnknownCycles,		999.9f,	false,	L"" );
	PROCESS_STAT1( "temp1",				Frame_Temp1Cycles 			);
	PROCESS_STAT1( "temp2",				Frame_Temp2Cycles 			);
	PROCESS_STAT1( "temp3",				Frame_Temp3Cycles 			);
	PROCESS_STAT2( "stats",				Frame_StatsCycles, 			999.9f,			false,	L"" );

	NEW_ROW
	Canvas->CurY = RowHeight;
	Ren.Release();
}

//---------------------------------------------------------------------------

void FStats::RenderFrameRate( class UViewport* Viewport, class UEngine* Engine )
{
	DWORD	Frame_Cycles = DWORDOldStats(GEngineStats.STATS_Frame_TotalCycles);
	INT		FrameRate = 1.f / (Frame_Cycles * GSecondsPerCycle);
	FLOAT	FrameTime = 1000.0f / (FLOAT)FrameRate;
	
	// maintain a "window" X seconds long containing all the frame times during that period
	const INT WindowSizeSecs = 1; // size of sample window (secs)
	const FLOAT MaxFrameTime = WindowSizeSecs*1000.0f; // size of sample window (ms)
	
	UCanvas* Canvas	= Viewport->Canvas;

	// add new frame time to end of list
	
	if( ListEnd == ListStart && FrameTimeSamples[ ListStart ] >= 0.0f )
	{
		// about to spam start of list (list full)
		TotalFrameTime -= FrameTimeSamples[ ListStart ];
		ListCount--;
		ListStart++;
		if( ListStart >= ARRAY_COUNT(FrameTimeSamples) )
			ListStart = 0; // wrap
	}

	FrameTimeSamples[ ListEnd ] = FrameTime;
	TotalFrameTime += FrameTime;
	ListCount++;
	
	ListEnd++;
	if( ListEnd >= ARRAY_COUNT(FrameTimeSamples) )
		ListEnd = 0; // wrap
		
	// make sure total frame time is always <= X secs
	while( TotalFrameTime > MaxFrameTime && ListCount > 1 )
	{
		// remove items from start of list until TotalSampleTime < 1.0f
		TotalFrameTime -= FrameTimeSamples[ ListStart ];
		ListCount--;
		ListStart++;
		if( ListStart >= ARRAY_COUNT(FrameTimeSamples) )
			ListStart = 0; // wrap
	}

	FLOAT ThisTotalFrameTime = Min( MaxFrameTime, TotalFrameTime );
	FLOAT AverageFPS = 1000.0f*ListCount / ThisTotalFrameTime;
	
	if( AverageFPS < MinLastSec )
		MinLastSec = AverageFPS;
	if( AverageFPS > MaxLastSec )
		MaxLastSec = AverageFPS;
		
	TotalFrameRate += FrameRate;
	NumFrameSamples++;
		
	if( Engine->StatFlags & STAT_ShowFPSEx )
	{
		INT Width = 0, Height = 0;

		if( FrameRate <= 0 )
			FrameRate = 999;
			
		if(FrameRate < 30)
			Canvas->Color = FColor(255,0,0);
		else if(FrameRate < 60)
			Canvas->Color = FColor(255,255,0);
		else
			Canvas->Color = FColor(0,255,0);

		UFont* Font = STATFONT;

		FLOAT AverageFrameRate = TotalFrameRate/NumFrameSamples;

		FString Dummy = FString::Printf(TEXT("%4u FPS  %4.0f AVG (%0.1f ms)"), FrameRate, AverageFrameRate, 1000.0f / AverageFrameRate );
		Canvas->WrappedStrLenf( Font, Width, Height, *Dummy );
		Canvas->CurX = (Viewport->SizeX - Width - 16)/2;
		Canvas->CurY = 2*Height;
		Canvas->WrappedPrintf( Font, 0, *Dummy );

		Dummy = FString::Printf(TEXT("LastSec: %4.0f  Min: %4.0f  Max: %4.0f"), AverageFPS, MinLastSec, MaxLastSec );
		Canvas->WrappedStrLenf( Font, Width, Height, *Dummy );
		Canvas->CurX = (Viewport->SizeX - Width - 16)/2;
		Canvas->CurY = 3*Height;
		Canvas->WrappedPrintf( Font, 0, *Dummy );
	}

	//if( FrameRate < MinFPS )
	if( MinFPS > 0.0f && NumFrameSamples >= (0.5*MinFPS) && MinLastSec < MinFPS )
	{
		Engine->StatFlags |= STAT_Lock;
		Viewport->Actor->GetLevel()->GetLevelInfo()->bPlayersOnly = true;
	}
}

//---------------------------------------------------------------------------
#endif

//NOTE (mdf): called once per frame from FPlayerSceneNode::Render
void FStats::Render(class UViewport* Viewport, class UEngine* Engine)
{
	guard(FStats::Render);

#if 1 //NEW (mdf) Legend stats
	if( !Engine->StatFlags )
		return;
#endif

#if 1 //NEW (mdf) take last frame's stat cycles out of frame, render time
	Frame_StatsCycles = GStats.DWORDOldStats(GEngineStats.STATS_Stats_RenderCycles);
	
	if( bStatAdjust )
	{
		// This seems to be accurate as long as there is enough other render 
		// overhead, otherwise rendering hit ends up occuring under stat time
		// which causes the frame/client time to be reduced by too much.
		DWORDOldStats(GEngineStats.STATS_Frame_TotalCycles) -= Frame_StatsCycles;
		Engine->ClientCycles -= Frame_StatsCycles;
	}
#endif

#if 1 //NEW (mdf) Legend stats (tbd)
/*
	if( Engine->Audio && Engine->StatFlags & STAT_ShowAudioStats )
		Engine->Audio->PrintStats();
*/
#endif

	UCanvas* Canvas	= Viewport->Canvas;
	FString Dummy;

	// Render the framerate.
#if 1 //NEW (mdf) Legend stats
	if( (Engine->StatFlags & STAT_ShowFPSEx) || (MinFPS > 0.0f) )
		RenderFrameRate( Viewport, Engine );
#endif
		
#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowFPS )
#else
	if(Engine->bShowFrameRate)
#endif	
	{
		FLOAT FrameTime		= DWORDOldStats(GEngineStats.STATS_Frame_TotalCycles) * GSecondsPerCycle;
		FPSAvg				= FPSAvg ? FPSAvg * 0.98f + FrameTime * 0.02f : FrameTime;
		INT	AvgFrameRate	= FPSAvg ? 1.f / FPSAvg : 0,
			FrameRate		= 1.f / FrameTime,
			Width			= 0,
			Height			= 0;

		if(AvgFrameRate < 20)
			Canvas->Color = FColor(255,0,0);
		else if ( AvgFrameRate < 30 )
			Canvas->Color = FColor(255,0,255);
		else if(AvgFrameRate < 40)
			Canvas->Color = FColor(255,255,0);
		else
			Canvas->Color = FColor(0,255,0);

		Canvas->CurX = 0;
		Canvas->CurY = 0;

		Dummy = FString::Printf(TEXT("%u avg FPS"),AvgFrameRate);
		Canvas->WrappedStrLenf(Canvas->MedFont,Width,Height,*Dummy);

		Canvas->CurX = Viewport->SizeX - Width - 16;
		Canvas->CurY = 96;
		Canvas->WrappedPrintf(Canvas->MedFont,0,*FString::Printf(TEXT("%u cur FPS"),FrameRate));
		Canvas->CurX = Viewport->SizeX - Width - 16;
		Canvas->WrappedPrintf(Canvas->MedFont,0,*Dummy);
	}

	INT CurX		= 8;
	Canvas->CurY	= 8;

#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowQuick )
		RenderStatQuick( Viewport, Engine );
#endif

	// Render stats.
#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowRenderStats )
#else
	if( Engine->bShowRenderStats )
#endif
	{
		DRAW_HEADLINE( TEXT("- Frame ---------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Frame_TotalCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Frame_RenderCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Stats_RenderCycles			) ));

		DRAW_HEADLINE( TEXT("- BSP -----------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_BSP_RenderCycles				) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_BSP_Sections					) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_BSP_Nodes					) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_BSP_Triangles				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_BSP_DynamicLightingCycles	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_BSP_DynamicLights			) ));

		DRAW_HEADLINE( TEXT("- LightMap ------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_LightMap_Cycles				) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_LightMap_Updates				) ));

		DRAW_HEADLINE( TEXT("- Projector -----") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Projector_RenderCycles		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Projector_AttachCycles		) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Projector_Projectors			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Projector_Triangles			) ));
		
		DRAW_HEADLINE( TEXT("- Stencil -------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Stencil_RenderCycles			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Stencil_Nodes				) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Stencil_Triangles			) ));

		DRAW_HEADLINE( TEXT("- Visibility ----") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Visibility_SetupCycles		) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Visibility_MaskTests			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Visibility_MaskRejects		) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Visibility_BoxTests			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Visibility_BoxRejects		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Visibility_TraverseCycles	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Visibility_ScratchBytes		) ));
		DRAW_STRING( (TIME_PRINTF ,	TIME_ARGUMENTS ( STATS_Visibility_MeshLightCycles	) ));

		DRAW_HEADLINE( TEXT("- Terrain ------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Terrain_RenderCycles			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Terrain_Sectors				) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Terrain_Triangles			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Terrain_DrawPrimitives		) ));

		DRAW_HEADLINE( TEXT("- DecoLayer ----") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_DecoLayer_RenderCycles		) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_DecoLayer_Triangles			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_DecoLayer_Decorations		) ));

		DRAW_HEADLINE( TEXT("- Particle -----") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Particle_DrawCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Particle_TickCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Particle_RadiatorTick		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Particle_ForceCycles			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Particle_NumForces			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Particle_NumParticles		) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Particle_Flushes				) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Particle_Projectors			) ));

		DRAW_HEADLINE( TEXT("- Mesh ---------") );
		CalcMovingAverage( GEngineStats.STATS_Mesh_SkinCycles	,40);
		CalcMovingAverage( GEngineStats.STATS_Mesh_ResultCycles	,40);
		CalcMovingAverage( GEngineStats.STATS_Mesh_LODCycles	,40);
		CalcMovingAverage( GEngineStats.STATS_Mesh_PoseCycles	,40);
		CalcMovingAverage( GEngineStats.STATS_Mesh_SkelCycles	,40);
		CalcMovingAverage( GEngineStats.STATS_Mesh_RigidCycles	,40);
		CalcMovingAverage( GEngineStats.STATS_Mesh_DrawCycles	,40);		
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_SkinCycles				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_ResultCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_LODCycles				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_PoseCycles				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_SkelCycles				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_RigidCycles				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Mesh_DrawCycles				) ));
		
		DRAW_HEADLINE( TEXT("- StaticMesh ---") );
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_Triangles		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_StaticMesh_RenderCycles	) ));
		Canvas->CurY += 4;
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_Batches					) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_BatchedSortedTriangles	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_BatchedSortedSections		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_StaticMesh_BatchedSortCycles			) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_BatchedUnsortedSections	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_BatchedUnsortedTriangles	) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_StaticMesh_BatchedRenderCycles		) ));
		Canvas->CurY += 4;
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_UnbatchedSortedTriangles	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_UnbatchedSortedSections	) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_StaticMesh_UnbatchedSortCycles		) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_UnbatchedUnsortedSections	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_UnbatchedUnsortedTriangles) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_StaticMesh_UnbatchedRenderCycles		) ));

		DRAW_HEADLINE( TEXT("- Batch --------") );
		DRAW_STRING( (DWORD_PRINTF,	DWORD_ARGUMENTS( STATS_Batch_Batches				) ));
		DRAW_STRING( (DWORD_PRINTF,	DWORD_ARGUMENTS( STATS_Batch_Primitives				) ));
		DRAW_STRING( (TIME_PRINTF,	TIME_ARGUMENTS ( STATS_Batch_RenderCycles			) ));
	}
	
	// Render game stats.
#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowGameStats )
#else
	if( Engine->bShowGameStats )
#endif
	{
		DRAW_HEADLINE( TEXT("- Collision ----") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_StaticMesh_CollisionCycles	) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_CollisionCacheHits) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_CollisionCacheMisses) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_StaticMesh_CollisionCacheFlushes) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_BSP_CollisionCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Terrain_CollisionCycles		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Corona_CollisionCycles		) ));

		DRAW_HEADLINE( TEXT("- Karma --------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_Collision			    ) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_CollisionContactGen	) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_TrilistGen				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_RagdollTrilist			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_Dynamics				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_physKarma				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_physKarma_Con			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_physKarmaRagDoll		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Karma_Temp					) ));
		
		DRAW_HEADLINE( TEXT("- Fluid  --------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Fluid_SimulateCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Fluid_VertexGenCycles	    ) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Fluid_RenderCycles			) ));

		DRAW_HEADLINE( TEXT("- Game ---------") );
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_ScriptCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_ScriptTickCycles		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_ActorTickCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_FindPathCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_SeePlayerCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_SpawningCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_AudioTickCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_CleanupDestroyedCycles	) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_UnusedCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_NetTickCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_CanvasCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_PhysicsCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_MoveCycles				) ));
		DRAW_STRING( (DWORD_PRINTF, DWORD_ARGUMENTS( STATS_Game_NumMoves				) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_MLCheckCycles			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_MPCheckCycles			) ));	
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_UpdateRenderData		) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_HUDPostRender			) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_InteractionPreRender	) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_InteractionPostRender	) ));
		DRAW_STRING( (TIME_PRINTF , TIME_ARGUMENTS ( STATS_Game_ScriptDebugTime	) ));


		/* SCRIPTTIME
		if ( DWORDStats(GEngineStats.STATS_Game_ActorTickCycles) * GSecondsPerCycle * 1000.0f > 30.f )
		{
			debugf(TEXT(" ****************************************************************"));
			debugf(TEXT(" ActorTick Cycles %f"), DWORDStats(GEngineStats.STATS_Game_ActorTickCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" Script Cycles %f"), GScriptCycles * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" ScriptTick Cycles %f"), DWORDStats(GEngineStats.STATS_Game_ScriptTickCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" FindPath Cycles %f"), DWORDStats(GEngineStats.STATS_Game_FindPathCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" SeePlayer Cycles %f"), DWORDStats(GEngineStats.STATS_Game_SeePlayerCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" Spawning Cycles %f"), DWORDStats(GEngineStats.STATS_Game_SpawningCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" CleanupDestroyed Cycles %f"), DWORDStats(GEngineStats.STATS_Game_CleanupDestroyedCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" AudioTick Cycles %f"), DWORDStats(GEngineStats.STATS_Game_AudioTickCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" Canvas Cycles %f"), DWORDStats(GEngineStats.STATS_Game_CanvasCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" Physics Cycles %f"), DWORDStats(GEngineStats.STATS_Game_PhysicsCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" Move Cycles %f"), DWORDStats(GEngineStats.STATS_Game_MoveCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" MLCheck Cycles %f"), DWORDStats(GEngineStats.STATS_Game_MLCheckCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" MPCheck Cycles %f"), DWORDStats(GEngineStats.STATS_Game_MPCheckCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" UpdateRenderData Cycles %f"), DWORDStats(GEngineStats.STATS_Game_UpdateRenderData) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" NetTick Cycles %f"), DWORDStats(GEngineStats.STATS_Game_NetTickCycles) * GSecondsPerCycle * 1000.0f);
			debugf(TEXT(" HUD PostRender Cycles %f"), DWORDStats(GEngineStats.STATS_Game_HUDPostRender) * GSecondsPerCycle * 1000.0f);
		}
		*/
	}

	// Render hardware stats.
#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowHardwareStats )
#else
	if( Engine->bShowHardwareStats )
#endif
	{
		DRAW_HEADLINE( TEXT("- Hardware -----") );
		DRAW_ALL(STATSTYPE_Hardware);
	}

	// Render audio stats.
#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowAudioStats )
#else
	if( Engine->bShowAudioStats )
#endif
	{
		DRAW_HEADLINE( TEXT("- Audio --------") );
		DRAW_ALL(STATSTYPE_Audio);
	}

	// Render network stats.
#if 1 //NEW (mdf) Legend stats
	if( Engine->StatFlags & STAT_ShowNetStats )
#else
	if( Engine->bShowNetStats )
#endif
	{	
		// Update net stats (yeah, this is a hack).
		UNetConnection*	Conn = NULL;
		if( Viewport->Actor->XLevel->NetDriver && ((Conn = Viewport->Actor->XLevel->NetDriver->ServerConnection) != NULL) )
		{
			INT	NumChannels = 0;

			for(INT ChannelIndex = 0;ChannelIndex < UNetConnection::MAX_CHANNELS;ChannelIndex++)
				NumChannels += (Conn->Channels[ChannelIndex] != NULL);

			DWORDStats(GEngineStats.STATS_Net_Ping			) = (DWORD) 1000.0f*Conn->BestLag;
			DWORDStats(GEngineStats.STATS_Net_Channels		) = (DWORD) NumChannels;
			DWORDStats(GEngineStats.STATS_Net_InUnordered	) = (DWORD) Conn->InOrder;
			DWORDStats(GEngineStats.STATS_Net_OutUnordered	) = (DWORD) Conn->OutOrder;
			DWORDStats(GEngineStats.STATS_Net_InPacketLoss	) = (DWORD) Conn->InLoss;
			DWORDStats(GEngineStats.STATS_Net_OutPacketLoss	) = (DWORD) Conn->OutLoss;
			DWORDStats(GEngineStats.STATS_Net_InPackets		) = (DWORD) Conn->InPackets;
			DWORDStats(GEngineStats.STATS_Net_OutPackets	) = (DWORD) Conn->OutPackets;
			DWORDStats(GEngineStats.STATS_Net_InBunches		) = (DWORD) Conn->InBunches;
			DWORDStats(GEngineStats.STATS_Net_OutBunches	) = (DWORD) Conn->OutBunches;
			DWORDStats(GEngineStats.STATS_Net_InBytes		) = (DWORD) Conn->InRate;
			DWORDStats(GEngineStats.STATS_Net_OutBytes		) = (DWORD) Conn->OutRate;
			DWORDStats(GEngineStats.STATS_Net_Speed			) = (DWORD) Conn->CurrentNetSpeed;			
		}
		DRAW_HEADLINE( TEXT("- Net ----------") );
		DRAW_ALL(STATSTYPE_Net);
	}

	if(bShowAssetMemStats )
	{
		//UObject* OldCheckObject = CheckObject;

		if (GIsEditor)
		{
			for (TObjectIterator<AActor> It; It; ++It)
			{
				if( *It && It->bSelected )
				{
					CheckObject = *It;
					break;
				}
			}
		}
		/*
		else
		{
			CheckObject = Engine->memStatObject;
		}
		*/
	
		if((CheckObject != OldCheckObject || MemCount == NULL) && CheckObject)
		{
			OldCheckObject = CheckObject;
			if(MemCount)
			{
				delete MemCount;
				MemCount = NULL;
			}
			MemCount = new FCountAssetMem(CheckObject);
		}


		DRAW_HEADLINE( TEXT("- AssetMem ------") );
		if (CheckObject)
		{
			DRAW_STRING( FString::Printf(TEXT("Total Memory for '%s': %.2f kb"), CheckObject->GetFullName(), (MemCount->StaticNum + MemCount->Num) / 1024.0f) );
			DRAW_STRING( FString::Printf(TEXT("Static Memory for '%s': %.2f kb"), CheckObject->GetFullName(), MemCount->StaticNum / 1024.0f) );
			for (INT i = 0; i < MemCount->StaticClassCounts.Num(); i++)
			{
				DRAW_STRING( FString::Printf(TEXT("'%s': %.2f kb"), MemCount->StaticClassCounts(i).Class->GetName(), MemCount->StaticClassCounts(i).Num / 1024.0f) );
			}
			DRAW_STRING( FString::Printf(TEXT("Instance Memory: %.2f kb"), MemCount->Num / 1024.0f) );
			for (INT i = 0; i < MemCount->ClassCounts.Num(); i++)
			{
				DRAW_STRING( FString::Printf(TEXT("'%s': %.2f kb"), MemCount->ClassCounts(i).Class->GetName(), MemCount->ClassCounts(i).Num / 1024.0f) );
			}
		}
		else
		{
			DRAW_STRING( FString::Printf(TEXT(" NO TARGET! 'TraceForMemStats' to target")));
		}
	}
	// SL MODIFY END

	if(bShowMemStats )
	{
		DRAW_HEADLINE( TEXT("- Memory ------") );

		FString tempString;
		FStringOutputDevice StrOut;
		Engine->Exec( TEXT("MEMINFO"), StrOut );
		tempString = *StrOut;
		const TCHAR* Str = *tempString;

		TCHAR tmp[20];

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("%s Phys Mem Usage"), tmp));

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("%s Total Phys Mem"), tmp));

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("%s Page File Usage"), tmp));

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("%s Total Page File"), tmp));

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("%s Virtual Mem Usage"), tmp));

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("%s Total Virtual Mem"), tmp));

		ParseToken(Str, tmp, 20, 0);
		DRAW_STRING( FString::Printf(TEXT("0.%s  Memory Load"), tmp));
	}

	unguard;
}

FEngineStats::FEngineStats()
{
	guard(FEngineStats::FEngineStats);
	
	//NEW (mdf) spam: debugf( L"FEngineStats::FEngineStats()" );
	appMemset( &STATS_FirstEntry, 0xFF, (DWORD) &STATS_LastEntry - (DWORD) &STATS_FirstEntry );
	unguard;
}

void FEngineStats::Init()
{
	guard(FEngineStats::Init);

	//NEW (mdf) spam: debugf( L"FEngineStats::Init()" );

	// If already initialized retrieve indices from GStats.
	if( GStats.Registered[STATSTYPE_Render] )
	{
		INT* Dummy = &STATS_Frame_TotalCycles;
		for( INT i=0; i<GStats.Stats[STATSTYPE_Render].Num(); i++ )
			*(Dummy++) = GStats.Stats[STATSTYPE_Render](i).Index;

		Dummy = &STATS_Game_ScriptCycles;
		for( INT i=0; i<GStats.Stats[STATSTYPE_Game].Num(); i++ )
			*(Dummy++) = GStats.Stats[STATSTYPE_Game](i).Index;

		Dummy = &STATS_Net_Ping;
		for( INT i=0; i<GStats.Stats[STATSTYPE_Net].Num(); i++ )
			*(Dummy++) = GStats.Stats[STATSTYPE_Net](i).Index;

		return;
	}

	// Register all stats with GStats.
#if 1 //NEW (mdf) Legend stats
	STATS_Batch_Batches							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Batches"			), TEXT("Batch"		), STATSUNIT_Default				);
	STATS_Batch_Primitives						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Primitives"		), TEXT("Batch"		), STATSUNIT_Default				);
	STATS_Batch_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Batch"		), STATSUNIT_MSec					);

	STATS_BSP_CollisionCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("BSP"				), TEXT("Collision"	),	STATSUNIT_MSec			);
	STATS_BSP_DynamicLightingCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DynamicLighting"	), TEXT("BSP"		), STATSUNIT_MSec					);
	STATS_BSP_DynamicLights						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DynamicLights"		), TEXT("BSP"		), STATSUNIT_Default				);
	STATS_BSP_Nodes								= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Nodes"				), TEXT("BSP"		), STATSUNIT_Default				);
	STATS_BSP_RenderCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("BSP"		), STATSUNIT_MSec					);
	STATS_BSP_Sections							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Sections"			), TEXT("BSP"		), STATSUNIT_Default				);
	STATS_BSP_Triangles							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("BSP"		), STATSUNIT_Default				);

	STATS_Corona_CollisionCycles				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Corona"					), TEXT("Collision"	),	STATSUNIT_MSec			);

	STATS_DecoLayer_Decorations					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Decorations"		), TEXT("DecoLayer"	), STATSUNIT_Default				);
	STATS_DecoLayer_RenderCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("DecoLayer"	), STATSUNIT_MSec					);
	STATS_DecoLayer_Triangles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("DecoLayer"	), STATSUNIT_Default				);

	STATS_Fluid_RenderCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Fluid"		), STATSUNIT_MSec					);
	STATS_Fluid_SimulateCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Simulate"			), TEXT("Fluid"		), STATSUNIT_MSec					);
	STATS_Fluid_VertexGenCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("VertexGen"			), TEXT("Fluid"		), STATSUNIT_MSec					);

	STATS_Frame_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Frame"		), STATSUNIT_MSec					);
	STATS_Frame_RenderOverlayCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Overlay"			), TEXT("Frame"		), STATSUNIT_MSec					);
	STATS_Frame_Temp1Cycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Temp1"				), TEXT("Frame"		), STATSUNIT_MSec					);
	STATS_Frame_Temp2Cycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Temp2"				), TEXT("Frame"		), STATSUNIT_MSec					);
	STATS_Frame_Temp3Cycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Temp3"				), TEXT("Frame"		), STATSUNIT_MSec					);
	STATS_Frame_TotalCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Frame"				), TEXT("Frame"		), STATSUNIT_MSec					);

	STATS_Game_ActorTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Actor"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_AnimationCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Animation"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_AudioTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Audio"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_CanvasCycles						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Canvas"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_CleanupDestroyedCycles			= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("CleanupDestroyed"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_FindPathCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Path"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_HUDPostRender					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("HUD PostRender"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_InteractionPostRender			= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Interaction Post"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_InteractionPreRender				= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Interaction Pre"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_MLCheckCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("MLChecks"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_MoveCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Move"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_MPCheckCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("MPChecks"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_NetTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Net"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_NumMoves							= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Move"				), TEXT("Game"		), STATSUNIT_Combined_Default_MSec	);
	STATS_Game_PhysicsCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Physics"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_ScriptCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Script"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_ScriptDebugTime					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("ScriptDebug"		), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_ScriptTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("ScriptTick"		), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_SeePlayerCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("See"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_SpawningCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Spawning"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_UnusedCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Unused"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_UpdateRenderData					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("RenderData"		), TEXT("Game"		), STATSUNIT_MSec					);

	STATS_Game_HearCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Hear"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_CScriptCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("CScript"			), TEXT("Game"		), STATSUNIT_MSec					);

	STATS_Game_TraceCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Trace"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_TraceActorCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Actor"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_TraceLevelCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Level"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_TraceTerrainCycles				= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Terrain"			), TEXT("Game"		), STATSUNIT_MSec					);

	STATS_Karma_Collision						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Collision"				), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_CollisionContactGen				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("  ContactGen"			), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_Dynamics						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Dynamics"				), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_physKarma						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("physKarma"				), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_physKarma_Con					= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("physKarma Constraint"  ), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_physKarmaRagDoll				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("physKarmaRagdoll"		), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_RagdollTrilist					= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("RagdollTrilist"		), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_Temp							= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Temp"					), TEXT("Karma"	    ), STATSUNIT_MSec				);
	STATS_Karma_TrilistGen						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("  TrilistGen"			), TEXT("Karma"	    ), STATSUNIT_MSec				);

	STATS_LightMap_Cycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Time"				), TEXT("LightMap"	), STATSUNIT_MSec					);
	STATS_LightMap_Updates						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Updates"			), TEXT("LightMap"	), STATSUNIT_Default				);

	STATS_Matinee_TickCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Tick"				), TEXT("Matinee"	), STATSUNIT_MSec					);

	STATS_Mesh_DrawCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Draw"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_LODCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("LOD"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_PoseCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Pose"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_ResultCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Result"			), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_RigidCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Rigid"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_SkelCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Skel"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_SkinCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Skin"				), TEXT("Mesh"		), STATSUNIT_MSec					);

	STATS_Net_Channels							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Channels"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_InBunches							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bunches"			), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_InBytes							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bytes"				), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_InPacketLoss						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("PacketLoss"		), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_InPackets							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Packets"			), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_InUnordered						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Unorderd"			), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_NumPV								= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("PV"				), TEXT("Game"		), STATSUNIT_Default					);
	STATS_Net_NumReps							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Reps"				), TEXT("Game"		), STATSUNIT_Default					);
	STATS_Net_NumRPC							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("RPC"				), TEXT("Game"		), STATSUNIT_Default					);
	STATS_Net_OutBunches						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bunches"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_OutBytes							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bytes"				), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_OutPacketLoss						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("PacketLoss"		), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_OutPackets						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Packets"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_OutUnordered						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Unordered"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_Ping								= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Ping"				), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_Speed								= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Speed"				), TEXT("Net"		), STATSUNIT_Default					);

	STATS_Projector_AttachCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Attach"			), TEXT("Projector"	), STATSUNIT_MSec						);
	STATS_Projector_Projectors					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Projectors"		), TEXT("Projector"	), STATSUNIT_Default					);
	STATS_Projector_RenderCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Projector"	), STATSUNIT_MSec						);
	STATS_Projector_Triangles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("Projector"	), STATSUNIT_Default					);

	STATS_Particle_DrawCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DrawCycles"		), TEXT("Particle"	), STATSUNIT_MSec						);
	STATS_Particle_TickCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("TickCycles"		), TEXT("Particle"	), STATSUNIT_MSec						);
	STATS_Particle_ForceCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("ForceCycles"		), TEXT("Particle"	), STATSUNIT_MSec						);
	STATS_Particle_NumForces					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("NumForces"			), TEXT("Particle"	), STATSUNIT_Default						);
	STATS_Particle_NumParticles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("NumParticles"		), TEXT("Particle"	), STATSUNIT_Default						);
	STATS_Particle_Flushes						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Flushes"			), TEXT("Particle"	), STATSUNIT_Default						);
	STATS_Particle_Projectors					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Projectors"		), TEXT("Particle"	), STATSUNIT_Default						);

	STATS_UI_GameCycles							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UIGame"			), TEXT("Game"	), STATSUNIT_Default					);
	STATS_UI_RenderCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UIRender"			), TEXT("Game"	), STATSUNIT_Default					);
	
	STATS_StaticMesh_BatchedRenderCycles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedRender"				), TEXT("StaticMesh"), STATSUNIT_MSec				);
	STATS_StaticMesh_BatchedSortCycles			= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedSort"				), TEXT("StaticMesh"), STATSUNIT_MSec				);
	STATS_StaticMesh_BatchedSortedSections		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedSortedSections"		), TEXT("StaticMesh"), STATSUNIT_Default			);
	STATS_StaticMesh_BatchedSortedTriangles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedSortedTriangles"	), TEXT("StaticMesh"), STATSUNIT_Default			);
	STATS_StaticMesh_BatchedUnsortedSections	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedUnsortedSections"	), TEXT("StaticMesh"), STATSUNIT_Default			);
	STATS_StaticMesh_BatchedUnsortedTriangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedUnsortedTriangles"	), TEXT("StaticMesh"), STATSUNIT_Default			);
	STATS_StaticMesh_Batches					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Batches"					), TEXT("StaticMesh"), STATSUNIT_Default			);
	STATS_StaticMesh_CollisionCacheFlushes		= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh Cache Flushes"	), TEXT("" ),			STATSUNIT_Default			);
	STATS_StaticMesh_CollisionCacheHits			= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh Cache Hits"		), TEXT("" ),			STATSUNIT_Default			);
	STATS_StaticMesh_CollisionCacheMisses		= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh Cache Misses"	), TEXT("" ),			STATSUNIT_Default			);
	STATS_StaticMesh_CollisionCycles			= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh"				), TEXT("Collision" ),	STATSUNIT_MSec				);
	STATS_StaticMesh_RenderCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"), TEXT("StaticMesh"), STATSUNIT_MSec );
	STATS_StaticMesh_Triangles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"), TEXT("StaticMesh"), STATSUNIT_Default );
	STATS_StaticMesh_UnbatchedRenderCycles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedRender"			), TEXT("StaticMesh"), STATSUNIT_MSec		);
	STATS_StaticMesh_UnbatchedSortCycles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedSort"				), TEXT("StaticMesh"), STATSUNIT_MSec		);
	STATS_StaticMesh_UnbatchedSortedSections	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedSortedSections"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedSortedTriangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedSortedTriangles"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedUnsortedSections	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedUnsortedSections"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedUnsortedTriangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedUnsortedTriangles"), TEXT("StaticMesh"), STATSUNIT_Default	);

	STATS_Stats_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Stats Render"		), TEXT("Frame"		), STATSUNIT_MSec					);

	STATS_Stencil_Nodes							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Nodes"				), TEXT("Stencil"	), STATSUNIT_Default				);
	STATS_Stencil_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Stencil"	), STATSUNIT_MSec					);
	STATS_Stencil_Triangles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("Stencil"	), STATSUNIT_Default				);

	STATS_Terrain_CollisionCycles				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Terrain"					), TEXT("Collision"	),	STATSUNIT_MSec			);
	STATS_Terrain_DrawPrimitives				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DrawPrimitives"	), TEXT("Terrain"	), STATSUNIT_Default				);
	STATS_Terrain_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Terrain"	), STATSUNIT_MSec					);
	STATS_Terrain_Sectors						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Sectors"			), TEXT("Terrain"	), STATSUNIT_Default				);
	STATS_Terrain_Triangles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("Terrain"	), STATSUNIT_Default				);

	STATS_Visibility_BoxRejects					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BoxRejects"		), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_BoxTests					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BoxTests"			), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_MaskRejects				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("MaskRejects"		), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_MaskTests					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("MaskTests"			), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_MeshLightCycles			= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("MeshLight"			), TEXT("Visibility"), STATSUNIT_MSec					);
	STATS_Visibility_ScratchBytes				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("ScratchBytes"		), TEXT("Visibility"), STATSUNIT_KByte					);
	STATS_Visibility_SetupCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Setup"				), TEXT("Visibility"), STATSUNIT_MSec					);
	STATS_Visibility_TraverseCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Traverse"			), TEXT("Visibility"), STATSUNIT_MSec					);
#else
/*
	STATS_Frame_TotalCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Frame"				), TEXT("Frame"		), STATSUNIT_MSec					);
	STATS_Frame_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Frame"		), STATSUNIT_MSec					);
	
	STATS_BSP_RenderCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("BSP"		), STATSUNIT_MSec					);
	STATS_BSP_Sections							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Sections"			), TEXT("BSP"		), STATSUNIT_Default				);
	STATS_BSP_Nodes								= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Nodes"				), TEXT("BSP"		), STATSUNIT_Default				);
	STATS_BSP_Triangles							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("BSP"		), STATSUNIT_Default				);
	STATS_BSP_DynamicLightingCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DynamicLighting"	), TEXT("BSP"		), STATSUNIT_MSec					);
	STATS_BSP_DynamicLights						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DynamicLights"		), TEXT("BSP"		), STATSUNIT_Default				);

	STATS_LightMap_Updates						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Updates"			), TEXT("LightMap"	), STATSUNIT_Default				);
	STATS_LightMap_Cycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Time"				), TEXT("LightMap"	), STATSUNIT_MSec					);

	STATS_Projector_RenderCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Projector"	), STATSUNIT_MSec					);
	STATS_Projector_AttachCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Attach"			), TEXT("Projector"	), STATSUNIT_MSec					);
	STATS_Projector_Projectors					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Projectors"		), TEXT("Projector"	), STATSUNIT_Default				);
	STATS_Projector_Triangles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("Projector"	), STATSUNIT_Default				);
			
	STATS_Stencil_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Stencil"	), STATSUNIT_MSec					);
	STATS_Stencil_Nodes							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Nodes"				), TEXT("Stencil"	), STATSUNIT_Default				);
	STATS_Stencil_Triangles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("Stencil"	), STATSUNIT_Default				);

	STATS_Visibility_SetupCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Setup"				), TEXT("Visibility"), STATSUNIT_MSec					);
	STATS_Visibility_MaskTests					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("MaskTests"			), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_MaskRejects				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("MaskRejects"		), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_BoxTests					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BoxTests"			), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_BoxRejects					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BoxRejects"		), TEXT("Visibility"), STATSUNIT_Default				);
	STATS_Visibility_TraverseCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Traverse"			), TEXT("Visibility"), STATSUNIT_MSec					);
	STATS_Visibility_ScratchBytes				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("ScratchBytes"		), TEXT("Visibility"), STATSUNIT_KByte					);
	STATS_Visibility_MeshLightCycles			= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("MeshLight"			), TEXT("Visibility"), STATSUNIT_MSec					);

	STATS_Terrain_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Terrain"	), STATSUNIT_MSec					);
	STATS_Terrain_Sectors						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Sectors"			), TEXT("Terrain"	), STATSUNIT_Default				);
	STATS_Terrain_Triangles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("Terrain"	), STATSUNIT_Default				);
	STATS_Terrain_DrawPrimitives				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("DrawPrimitives"	), TEXT("Terrain"	), STATSUNIT_Default				);

	STATS_DecoLayer_RenderCycles				= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("DecoLayer"	), STATSUNIT_MSec					);
	STATS_DecoLayer_Triangles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"			), TEXT("DecoLayer"	), STATSUNIT_Default				);
	STATS_DecoLayer_Decorations					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Decorations"		), TEXT("DecoLayer"	), STATSUNIT_Default				);

	STATS_Matinee_TickCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Tick"				), TEXT("Matinee"	), STATSUNIT_MSec					);

	STATS_Mesh_SkinCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Skin"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_ResultCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Result"			), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_LODCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("LOD"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_SkelCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Skel"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_PoseCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Pose"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_RigidCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Rigid"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	STATS_Mesh_DrawCycles						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Draw"				), TEXT("Mesh"		), STATSUNIT_MSec					);
	
	STATS_StaticMesh_Triangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"), TEXT("StaticMesh"), STATSUNIT_Default );
	STATS_StaticMesh_RenderCycles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Triangles"), TEXT("StaticMesh"), STATSUNIT_MSec );

	STATS_StaticMesh_Batches					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Batches"					), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_BatchedSortedSections		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedSortedSections"		), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_BatchedSortedTriangles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedSortedTriangles"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_BatchedSortCycles			= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedSort"				), TEXT("StaticMesh"), STATSUNIT_MSec		);
	STATS_StaticMesh_BatchedUnsortedSections	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedUnsortedSections"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_BatchedUnsortedTriangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedUnsortedTriangles"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_BatchedRenderCycles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("BatchedRender"				), TEXT("StaticMesh"), STATSUNIT_MSec		);

	STATS_StaticMesh_UnbatchedSortedSections	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedSortedSections"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedSortedTriangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedSortedTriangles"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedSortCycles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedSort"				), TEXT("StaticMesh"), STATSUNIT_MSec		);
	STATS_StaticMesh_UnbatchedUnsortedSections	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedUnsortedSections"	), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedUnsortedTriangles	= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedUnsortedTriangles"), TEXT("StaticMesh"), STATSUNIT_Default	);
	STATS_StaticMesh_UnbatchedRenderCycles		= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("UnbatchedRender"			), TEXT("StaticMesh"), STATSUNIT_MSec		);

	STATS_Batch_Batches							= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Batches"			), TEXT("Batch"		), STATSUNIT_Default				);
	STATS_Batch_Primitives						= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Primitives"		), TEXT("Batch"		), STATSUNIT_Default				);
	STATS_Batch_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Batch"		), STATSUNIT_MSec					);

	STATS_Stats_RenderCycles					= GStats.RegisterStats( STATSTYPE_Render, STATSDATATYPE_DWORD, TEXT("Stats Render"		), TEXT("Frame"		), STATSUNIT_MSec					);

	STATS_StaticMesh_CollisionCycles			= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh"				), TEXT("Collision" ),	STATSUNIT_MSec			);
	STATS_StaticMesh_CollisionCacheHits			= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh Cache Hits"		), TEXT("" ),			STATSUNIT_Default		);
	STATS_StaticMesh_CollisionCacheMisses		= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh Cache Misses"	), TEXT("" ),			STATSUNIT_Default		);
	STATS_StaticMesh_CollisionCacheFlushes		= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("StaticMesh Cache Flushes"	), TEXT("" ),			STATSUNIT_Default		);
	STATS_BSP_CollisionCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("BSP"						), TEXT("Collision"	),	STATSUNIT_MSec			);
	STATS_Terrain_CollisionCycles				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Terrain"					), TEXT("Collision"	),	STATSUNIT_MSec			);
	STATS_Corona_CollisionCycles				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Corona"					), TEXT("Collision"	),	STATSUNIT_MSec			);

	STATS_Karma_Collision						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Collision"				), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_CollisionContactGen				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("  ContactGen"			), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_TrilistGen						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("  TrilistGen"			), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_RagdollTrilist					= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("RagdollTrilist"		), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_Dynamics						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Dynamics"				), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_physKarma						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("physKarma"				), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_physKarma_Con					= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("physKarma Constraint"  ), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_physKarmaRagDoll				= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("physKarmaRagdoll"		), TEXT("Karma"	    ), STATSUNIT_MSec					);
	STATS_Karma_Temp							= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Temp"					), TEXT("Karma"	    ), STATSUNIT_MSec					);

	STATS_Game_ScriptCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Script"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_ScriptTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("ScriptTick"		), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_ActorTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Actor"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_FindPathCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Path"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_SeePlayerCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("See"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_SpawningCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Spawning"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_AudioTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Audio"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_CleanupDestroyedCycles			= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("CleanupDestroyed"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_UnusedCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Unused"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_NetTickCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Net"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_CanvasCycles						= GStats.RegisterStats( STATSTYPE_Game,   STATSDATATYPE_DWORD, TEXT("Canvas"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_NumMoves							= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Move"				), TEXT("Game"		), STATSUNIT_Combined_Default_MSec	);
	STATS_Game_MoveCycles						= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Move"				), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_PhysicsCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Physics"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_MLCheckCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("MLChecks"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_MPCheckCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("MPChecks"			), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_UpdateRenderData					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("RenderData"		), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_HUDPostRender					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("HUD PostRender"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_InteractionPreRender				= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Interaction Pre"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_InteractionPostRender			= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Interaction Post"	), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Game_ScriptDebugTime					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("ScriptDebug"		), TEXT("Game"		), STATSUNIT_MSec					);
	STATS_Fluid_SimulateCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Simulate"			), TEXT("Fluid"		), STATSUNIT_MSec					);
	STATS_Fluid_VertexGenCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("VertexGen"			), TEXT("Fluid"		), STATSUNIT_MSec					);
	STATS_Fluid_RenderCycles					= GStats.RegisterStats( STATSTYPE_Game,	  STATSDATATYPE_DWORD, TEXT("Render"			), TEXT("Fluid"		), STATSUNIT_MSec					);

	STATS_Net_Ping								= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Ping"				), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_Channels							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Channels"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_InUnordered						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Unorderd"			), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_OutUnordered						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Unordered"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_InPacketLoss						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("PacketLoss"		), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_OutPacketLoss						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("PacketLoss"		), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_InPackets							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Packets"			), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_OutPackets						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Packets"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_InBunches							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bunches"			), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_OutBunches						= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bunches"			), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_InBytes							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bytes"				), TEXT("Net"		), STATSUNIT_Combined_Default_Default	);
	STATS_Net_OutBytes							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Bytes"				), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_Speed								= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Speed"				), TEXT("Net"		), STATSUNIT_Default					);
	STATS_Net_NumReps							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("Reps"				), TEXT("Game"		), STATSUNIT_Default				);
	STATS_Net_NumRPC							= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("RPC"				), TEXT("Game"		), STATSUNIT_Default				);
	STATS_Net_NumPV								= GStats.RegisterStats( STATSTYPE_Net,	  STATSDATATYPE_DWORD, TEXT("PV"				), TEXT("Game"		), STATSUNIT_Default				);
*/
#endif

	// Initialized.
	GStats.Registered[STATSTYPE_Render] = 1;
	GStats.Registered[STATSTYPE_Game]	= 1;
	GStats.Registered[STATSTYPE_Net]	= 1;

	unguard;
}

#if 1 //NEW (mdf) Legend stats
void FStats::ResetFrameTimeWindow()
{
	TotalFrameRate = 0.0f;
	
	ListStart = 0;
	ListEnd = 0; //ARRAY_COUNT(FrameTimeSamples)-1;
	ListCount = 0;

	for( INT ii=0; ii<ARRAY_COUNT(FrameTimeSamples); ii++ )
		FrameTimeSamples[ ii ] = -1.0f;
		
	TotalFrameTime = 0.0f;
	NumFrameSamples = 0;
	MinLastSec =  9999.f;
	MaxLastSec = -9999.f;
}

void FStats::ClearStats()
{
	guard(FStats::ClearStats);

	// NOTE: let's try to keep this sorted by group and within groups except for misc at the bottom
	Frame_CyclesA.ClearStats();
	Frame_FPSA.ClearStats();
	Frame_StatsCyclesA.ClearStats();
	Frame_Temp1CyclesA.ClearStats();
	Frame_Temp2CyclesA.ClearStats();
	Frame_Temp3CyclesA.ClearStats();
	Frame_UnknownCyclesA.ClearStats();

	Game_ActorTickCyclesA.ClearStats();
	Game_AnimationCyclesA.ClearStats();
	Game_CyclesA.ClearStats();
	Game_HearCyclesA.ClearStats();
	Game_KarmaCyclesA.ClearStats();
	Game_MoveCyclesA.ClearStats();
	Game_CScriptCyclesA.ClearStats();
	Game_PathCyclesA.ClearStats();
	Game_ScriptCyclesA.ClearStats();
	Game_ScriptTickCyclesA.ClearStats();
	Game_SeeCyclesA.ClearStats();
	Game_PhysicsCyclesA.ClearStats();
	Game_TickCyclesA.ClearStats();
	Game_TimerCyclesA.ClearStats();
	Game_TraceCyclesA.ClearStats();
	Game_TraceActorCyclesA.ClearStats();
	Game_TraceLevelCyclesA.ClearStats();
	Game_TraceTerrainCyclesA.ClearStats();

	Particle_CyclesA.ClearStats();
	Particle_DrawCyclesA.ClearStats();

	Render_BSPCyclesA.ClearStats();
	Render_CyclesA.ClearStats();
	Render_DecoLayerCyclesA.ClearStats();
	Render_LightingCyclesA.ClearStats();
	Render_MeshCyclesA.ClearStats();
	Render_OverlayCyclesA.ClearStats();
	Render_ProjectorCyclesA.ClearStats();
	Render_StaticMeshCyclesA.ClearStats();
	Render_TerrainCyclesA.ClearStats();
	Render_VisibilityCyclesA.ClearStats();
	Render_UnknownCyclesA.ClearStats();

	UI_TotalCyclesA.ClearStats();
	
	unguard;
}
#endif
