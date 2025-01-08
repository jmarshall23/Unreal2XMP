/*=============================================================================
 FStatAccumulator.cpp

 Helper class for accumulating stats so we can track the running average, min
 and max for stats.
=============================================================================*/

#include "EnginePrivate.h"
#include "FStatAccumulator.h"

#if 1 //NEW: Logging

FStatAccumulator::FStatAccumulator()
{
	ClearStats();
}

FStatAccumulator::~FStatAccumulator()
{
	//nothing?
}

inline FLOAT FStatAccumulator::AddStat( FLOAT Val )
{ 
	Total += Val; 
	NumStats++;

	if( Val > MaxStat ) 
		MaxStat = Val; 
	if( Val < MinStat ) 
		MinStat = Val; 

	return (FLOAT)(Total/NumStats);
}

inline FLOAT FStatAccumulator::GetStatsAvg() const
{ 
	if( NumStats == 0 )
		return 0.0f;
	else
		return Total/NumStats; 
}

inline FLOAT FStatAccumulator::GetStatsMin() const 		
{
	if( NumStats == 0 ) 
		return 0.0f;
	else
		return MinStat;
}

inline FLOAT FStatAccumulator::GetStatsMax() const 		
{
	if( NumStats == 0 ) 
		return 0.0f;
	else
		return MaxStat;
}

inline void FStatAccumulator::ClearStats()
{ 
	MinStat = FLT_MAX; 
	MaxStat = FLT_MIN; 
	Total = 0.0; 
	NumStats = 0; 
}
#endif

// end of FStatAccumulator.cpp
