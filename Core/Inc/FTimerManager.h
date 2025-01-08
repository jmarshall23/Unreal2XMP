/*=============================================================================
	FTimerManager.h: Generic timer functionality interface.
	Copyright (C) 2000 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Paul DuBois, InfiniteMachine.
		* Merged, cleaned-up and consolidated by Aaron R Leiby, Legend Entertainment.

=============================================================================*/

extern CORE_API FLOAT GTimerAcc;	// Time accumulator.	(FTimerManager is a singleton, so it's safe to make this static.)

/*----------------------------------------------------------------------------
	FTimerInfo.
----------------------------------------------------------------------------*/

struct CORE_API FTimerInfo
{
	FNotifyInfo	NotifyInfo;
	FLOAT		RingTime;
	FLOAT		Frequency;	// if<=0, only ring once

	FTimerInfo(){}
	FTimerInfo( FNotifyInfo _NotifyInfo, FLOAT _Frequency, UBOOL bRepeat=false, FLOAT InitialOffset=0.0f )
	{
		NotifyInfo = _NotifyInfo;
		RingTime   = GTimerAcc + _Frequency + InitialOffset;
		Frequency  = bRepeat ? _Frequency : 0.0f;
	}
	FTimerInfo( UObject* _Target, FName _NotifyFunc, FLOAT _Frequency, UBOOL bRepeat=false, FLOAT InitialOffset=0.0f )
	{
		NotifyInfo = FNotifyInfo(_Target,_NotifyFunc);
		RingTime   = GTimerAcc + _Frequency + InitialOffset;
		Frequency  = bRepeat ? _Frequency : 0.0f;
	}
	FTimerInfo( UObject* _Target, FString _NotifyFunc, FLOAT _Frequency, UBOOL bRepeat=false, FLOAT InitialOffset=0.0f )
	{
		NotifyInfo = FNotifyInfo(_Target,_NotifyFunc);
		RingTime   = GTimerAcc + _Frequency + InitialOffset;
		Frequency  = bRepeat ? _Frequency : 0.0f;
	}

	friend FArchive& operator<<( FArchive& Ar, FTimerInfo &T )
	{
		guard(FTimerInfo::operator<<);
		return Ar << T.NotifyInfo << T.RingTime << T.Frequency;
		unguard;
	}
};

inline bool operator<( const FTimerInfo& A, const FTimerInfo& B )
	{ return A.RingTime < B.RingTime; }

/*----------------------------------------------------------------------------
	FTimerManager.
----------------------------------------------------------------------------*/

class CORE_API FTimerManager
{
public:
	virtual void AddTimer( FNotifyInfo NotifyInfo, FLOAT Frequency, UBOOL bRepeat=false, FLOAT RandomSpread=0.0f, UBOOL bForceUpdate=false );
	//virtual INT  RemoveTimer( FNotifyInfo NotifyInfo );
	virtual INT FTimerManager::RemoveTimer( UObject* Target, FName FunctionName );
	virtual INT  RemoveAllTimers( UObject* Target );
	virtual FLOAT TimeRemaining( FNotifyInfo NotifyInfo );
	virtual void Tick( FLOAT DeltaSeconds );
	static FTimerManager* GetInstance();
	static void FreeInstance();
	static INT NumTimers() { return GetInstance()->Timers.Num()-1; }
	static void Serialize( FArchive& Ar );

	// Fix ARL: Use DestroyObservers instead?
	void CleanupDestroyed()
	{
		guard(FTimerManager::CleanupDestroyed);
		for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
			if( !It->NotifyInfo.IsValid() || It->NotifyInfo.GetTarget()->IsPendingKill() )
			{
				//debugf( L"CleanupDestroyed detroying timer for %s %s", It->NotifyInfo.GetTarget()->GetName(), It->NotifyInfo.GetFunction()->GetName() );
				It.RemoveCurrent();
			}

		unguard;
	}

	UBOOL ValidateRefs()
	{
		guard(FTimerManager::ValidateRefs);
		debugf(TEXT("FTimerManager::ValidateRefs"));

		UBOOL bResult = true;
		for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
		{
			//if( !It->NotifyInfo.IsValid() || It->NotifyInfo.GetTarget()->IsPendingKill() )
			if( !It->NotifyInfo.IsValid() || It->NotifyInfo.GetTarget()->GetFlags() & RF_Unreachable )
			{
				// if purging garbage, silently clean up refs since actors won't have been properly destroyed
				It.RemoveCurrent();
				if( !UObject::PurgingGarbage() )
				{
					// not purging garbage -- warn about a potentially nasty situation
					debugf( L"WARNING: FTimerManager has a reference to destroyed actor '%s' for timer function '%s'!", It->NotifyInfo.IsValid() ? It->NotifyInfo.GetTarget()->GetName() : L"NA", It->NotifyInfo.GetFunction()->IsValid() ? It->NotifyInfo.GetFunction()->GetName() : L"NA" );
					bResult = false;
				}
			}
		}
							
		return bResult;
		unguard;
	}

	static INT FTimerCycles;		// mdf-tbd?
	static INT FTimerScriptCycles;	// mdf-tbd?

	//!!mdf-tbr : temporary hack to check for bogus timer data!
	static FTimerManager* _GetInstance() { return _Instance; }

private:
	static FTimerManager* _Instance;
	THeap<FTimerInfo> Timers;
	FTimerManager(){}												// use GetInstance().
	~FTimerManager(){ /*Fix ARL: Timers.Empty();*/ }				// use FreeInstance().
	friend FArchive& operator<<( FArchive& Ar, FTimerManager &M )	// use Serialize().
	{
		guard(FTimerManager::operator<<);
		return Ar << GTimerAcc << M.Timers;
		unguard;
	}
};

