/*=============================================================================
 FStatAccumulator.h

 See FStatAccumulator.cpp.
=============================================================================*/

#ifndef _INC_FStatAccumulator
#define _INC_FStatAccumulator

#if 1 //NEW: Logging
class ENGINE_API FStatAccumulator
{
private:
	DOUBLE  Total;							// grand total since we started collecting stats
	DWORD	NumStats;						// grand total number of stats collected
	FLOAT 	MinStat;						// minimum since stats were cleared
	FLOAT 	MaxStat;						// maximum since stats were cleared

public:
	FStatAccumulator();
	~FStatAccumulator();

	// accept new stat and return current running average
	inline FLOAT AddStat( FLOAT Val );		// add a stat and return the current average
	inline FLOAT GetStatsAvg() const;		// returns current average or 0.0 if NumStats is 0
	inline FLOAT GetStatsMin() const;		// returns current min or 0.0 if NumStats is 0
	inline FLOAT GetStatsMax() const; 		// returns current max or 0.0 if NumStats is 0
	inline void  ClearStats();				// clear min, max, total, num				
};
#endif

#endif //_INC_FStatAccumulator_
// end of FStatAccumulator.h
