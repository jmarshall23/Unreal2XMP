/*=============================================================================
	FTimerManager.cpp: Generic timer functionality implementation.
	Copyright (C) 2000 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Created by Paul DuBois, InfiniteMachine.
		* Merged, cleaned-up and consolidated by Aaron R Leiby, Legend Entertainment.
		* Various fixes, added functionality by Mike Fox, Legend Entertainment.
=============================================================================*/

#include "CorePrivate.h"
#include "FTimerManager.h"	// Generic timer support.

CORE_API FLOAT GTimerAcc=0.0f;	// Time accumulator

FTimerManager* FTimerManager::_Instance = NULL;

//mdf-tbd:
INT FTimerManager::FTimerCycles = 0.0f;
INT FTimerManager::FTimerScriptCycles = 0.0f;

//#define ENABLETRACKSPIKES 1 // mdf-tbr:
//#define FTDEBUG(func) func
#define FTDEBUG(func)

FTimerManager* FTimerManager::GetInstance()
{
	guard(FTimerManager::GetInstance);
	if(!_Instance) _Instance=new FTimerManager();
	return _Instance;
	unguard;
}

void FTimerManager::FreeInstance()
{
	guard(FTimerManager::FreeInstance);

	FTDEBUG( debugf( L"FTimerManager::FreeInstance" ) );

	if(_Instance){ delete _Instance; _Instance=NULL; }
	unguard;
}

void FTimerManager::Serialize( FArchive& Ar )
{
	guard(FTimerManager::Serialize);

	if( GIsEditor )
	{
		// (mdf) hack to fix timer garbage in some levels (including some effects packages)
		// always write out NULL data
		FTimerManager* Dummy = NULL;
		Ar << *(DWORD*)&Dummy;

		if( Dummy )
		{
	   		if( !Ar.IsSaving() ) // pbly redundent since we couldn't have obtained a non-NULL Dummy above if saving
			{
				// read bogus data (but we won't write it out on saving)
				Dummy = new FTimerManager();
				Ar << *Dummy;
				delete Dummy;
			}
		}
	}
	else 
	{
		// in game
		if( Ar.IsSaving() )
		{
			// saving when not in editor should be fine (saved game)
			Ar << *(DWORD*)&_Instance;

		   	if( _Instance )
				Ar << *_Instance;
		}
		else if( Ar.IsLoading() )
		{
			FTimerManager* Dummy = NULL;
			Ar << *(DWORD*)&Dummy;

			// NOTE #1: (mdf) we should never write a NULL timer into _Instance (e.g.
			// if you load a level and associated particle effects (levels) are 
			// loaded these will contain FTimerManagers (which should be NULL).
			// NOTE #2: (mdf) we always delete the current timers (if any) in
			// LoadMap (see call to FTimerManager::FreeInstance() there).
		   	if( Dummy )
			{
				if( !_Instance )
					_Instance = new FTimerManager();
				Ar << *_Instance;
			}
		}
		else
		{
			Ar << *(DWORD*)&_Instance;

			//debugf( TEXT("FTimerManager (trans) _Instance: %08x"), _Instance ); //!!mdf - tbr:

			if( _Instance )
				Ar << *_Instance;
		}
	}
	
	unguard;
}

void FTimerManager::Tick( FLOAT DeltaSeconds )
{
	guard(FTimerManager::Tick);

	FTDEBUG( debugf( L"FTimerManager::Tick -- GTimerAcc: %0.3f IN  Timers: %d", GTimerAcc, Timers.Num() ) );
	//for( INT ii=0; ii<Timers.Num(); ii++ )
	//	debugf( L"   %d: %s.%s", ii, Timers(ii).NotifyInfo.GetTarget() ? Timers(ii).NotifyInfo.GetTarget()->GetFullName() : L"NA", Timers(ii).NotifyInfo.GetFunction() ? Timers(ii).NotifyInfo.GetFunction()->GetName() : L"NA" );
		
	GTimerAcc += DeltaSeconds;

	clock(FTimerManager::FTimerCycles);

	if( !Timers.IsEmpty() )
	{
		FTimerInfo Timer = Timers.Top();

		while( Timer.RingTime < GTimerAcc )
		{
			FTDEBUG( debugf( L"FTimerManager::Tick -- checking %s.%s GTimerAcc=%0.3f RingTime=%0.3f RepeatFrequency=%0.3f RemainingTime=%0.3f", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), GTimerAcc, Timer.RingTime, Timer.Frequency, (Timer.RingTime - GTimerAcc) ) );

			// update timer and move it to the top of the heap if it has a repeat interval, otherwise remove it
			if( Timer.Frequency > 0.0 )
			{
				Timer.RingTime += Timer.Frequency;
				if( Timer.RingTime < GTimerAcc )
				{
					// Note (mdf): otherwise timer will get pushed down in heap and we'll get 2 
					// notifications this tick. This can happen any time DeltaSeconds is
					// high relative to the timer's frequency, such as on startup or 
					// during periods of low framerate.
					Timer.RingTime = GTimerAcc;
				}
				Timers.ReplaceTop(Timer);
				//FTDEBUG( debugf( L"FTimerManager::Tick -- replaced %s.%s RingTime=%0.3f RepeatFrequency=%0.3f RemainingTime=%0.3f", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), Timer.RingTime, Timer.Frequency, (Timer.RingTime - GTimerAcc) ) );
			}
			else
			{
				//FTDEBUG( debugf( L"FTimerManager::Tick -- removing %s.%s RingTime=%0.3f RepeatFrequency=%0.3f RemainingTime=%0.3f", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), Timer.RingTime, Timer.Frequency, (Timer.RingTime - GTimerAcc) ) );
				Timers.RemoveTop();
			}

			// Send notification.
			FTDEBUG( debugf( L"FTimerManager::Tick -- time up for %s.%s GTimerAcc=%0.3f RingTime=%0.3f RepeatFrequency=%0.3f RemainingTime=%0.3f", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), GTimerAcc, Timer.RingTime, Timer.Frequency, (Timer.RingTime - GTimerAcc) ) );
			FTDEBUG( debugf( L"Timeup %s.%s (DS=%0.3f)", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), DeltaSeconds ) );

			//ValidateTimer( Timer ); //!!mdf-tbr:
			
#ifdef ENABLETRACKSPIKES
			INT OldFTimerScriptCycles = FTimerManager::FTimerScriptCycles;
			FTDEBUG( debugf( L"FTM -- calling %s.%s", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName() ) );
#endif
			clock(FTimerManager::FTimerScriptCycles);
			//guard(FTimerManager::Tick - SendNotification);
			FTDEBUG( debugf( L"==> sending notification to %s.%s", Timer.NotifyInfo.GetTarget()->GetFullName(), Timer.NotifyInfo.GetFunction()->GetName() ) );
			Timer.NotifyInfo.SendNotification();
			//unguard;
			unclock(FTimerManager::FTimerScriptCycles);

#ifdef ENABLETRACKSPIKES
			static FLOAT MaxFTimerScriptTime = -1.0f;
			FLOAT ThisFTimerScriptTime = GSecondsPerCycle*1000.0f * (FTimerManager::FTimerScriptCycles - OldFTimerScriptCycles);
			if( ThisFTimerScriptTime > MaxFTimerScriptTime )
			{
				//FTDEBUG( debugf( L"FTM!! (%s.%s): %04.3f (new max)", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), ThisFTimerScriptTime ) );
				MaxFTimerScriptTime = ThisFTimerScriptTime;
			}
			else if( ThisFTimerScriptTime > 0.50f )
			{
				FTDEBUG( debugf( L"FTM! (%s.%s): %04.3f (spike)", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), ThisFTimerScriptTime ) );
			}
			else
			{
				FTDEBUG( debugf( L"FTM (%s.%s): %04.3f", Timer.NotifyInfo.GetTarget()->GetName(), Timer.NotifyInfo.GetFunction()->GetName(), ThisFTimerScriptTime ) );
			}
#endif

			// Get next.
			if( !Timers.IsEmpty() )
				Timer = Timers.Top();
			else break;
		}
	}

	unclock(FTimerManager::FTimerCycles);
	unguard;
}

void FTimerManager::AddTimer( FNotifyInfo NotifyInfo, FLOAT Frequency, UBOOL bRepeat, FLOAT RandomSpread, UBOOL bForceUpdate )
{
	guard(FTimerManager::AddTimer);

	FTDEBUG( debugf( L"AddTimer %s.%s", NotifyInfo.GetTarget()->GetName(), NotifyInfo.GetFunction()->GetName() ) );
	
	// see if we already have a timer for this NotifyInfo (matching object, function names)
	UBOOL bAddTimer = true;
	for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
	{
		if( It->NotifyInfo == NotifyInfo )
		{
			// unless bForceUpdate given, keep existing looping timers if new settings match
			if( bForceUpdate || It->Frequency <= 0.0 || !bRepeat || Frequency != It->Frequency )
			{
				FTDEBUG( debugf( L"  FTimerManager::AddTimer -- removing %s.%s (%0.1f %d)", It->NotifyInfo.GetTarget()->GetName(), It->NotifyInfo.GetFunction()->GetName(), Frequency, bRepeat ) );
				// note (mdf): probably about as effecient to remove/add as it is to update
				It.RemoveCurrent();
			}
			else
			{
				FTDEBUG( debugf( L"  FTimerManager::AddTimer -- keeping %s.%s (%0.1f %d)", It->NotifyInfo.GetTarget()->GetName(), It->NotifyInfo.GetFunction()->GetName(), Frequency, bRepeat ) );
				bAddTimer = false;
			}
			break;
		}
	}
	
	if( bAddTimer )
	{
		FTDEBUG( debugf( L"  AddTimer adding timer for %s.%s", NotifyInfo.GetTarget()->GetName(), NotifyInfo.GetFunction()->GetName() ) );
	
		// add it
		FLOAT InitialOffset;
		if( RandomSpread > 0.0f )
			InitialOffset = appFrand()*RandomSpread; // offset forward from 0..RandomSpread seconds
		else
			InitialOffset = 0.0f;

		FTDEBUG( debugf( L"FTimerManager::AddTimer -- adding %s.%s (%0.1f %d Offset=%0.3f)", NotifyInfo.GetTarget()->GetName(), NotifyInfo.GetFunction()->GetName(), Frequency, bRepeat, InitialOffset ) );
		Timers.Add( FTimerInfo( NotifyInfo, Frequency, bRepeat, InitialOffset ) );
	}

	//FTDEBUG( debugf( L"    Timers:" ) );
	//for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
	//	FTDEBUG( debugf( L"      %s.%s", It->NotifyInfo.GetTarget()->GetName(), It->NotifyInfo.GetFunction()->GetName() ) );

	unguard;
}

//NOTE (mdf): in some cases RemoveTimer was failing if the timer was added in 
// a particular state then RemoveTimer is called as that state exits -- the
// timer isn't found apparently because FindFunction is using the pending
// state? Since you can't override timer functions anyway -- I changed this
// to look for the timer by name in the heap instead of trying to match
// NotifyInfos.

//INT FTimerManager::RemoveTimer( FNotifyInfo NotifyInfo )
INT FTimerManager::RemoveTimer( UObject* Target, FName FunctionName )
{
	guard(FTimerManager::RemoveTimer);

	//FTDEBUG( debugf( L"RemoveTimer %s.%s", NotifyInfo.GetTarget() ? NotifyInfo.GetTarget()->GetName() : L"NA", NotifyInfo.GetFunction() ? NotifyInfo.GetFunction()->GetName() : L"NA") );
	FTDEBUG( debugf( L"RemoveTimer %s.%s", Target ? Target->GetName() : L"NA", *FunctionName ) );
	
	INT NumRemoved=0;
	for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
	{
		//if( It->NotifyInfo == NotifyInfo )
		if( It->NotifyInfo.GetTarget() == Target && It->NotifyInfo.GetFunction()->GetName() == *FunctionName )
		{
			It.RemoveCurrent(); 
			NumRemoved++; 
		}
	}
		
	FTDEBUG( debugf( L"    FTimerManager::RemoveTimer removed %d timer(s)", NumRemoved ) );

	//FTDEBUG( debugf( L"    Timers:" ) );
	//for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
		//FTDEBUG( debugf( L"      %s.%s", It->NotifyInfo.GetTarget()->GetName(), It->NotifyInfo.GetFunction()->GetName() ) );

	return NumRemoved;
	unguard;
}

INT FTimerManager::RemoveAllTimers( UObject* Target )
{
	guard(FTimerManager::RemoveAllTimers);

	FTDEBUG( debugf( L"RemoveAllTimer %s", Target->GetName() ) );
	
	INT NumRemoved=0;
	for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
	{
		if( It->NotifyInfo.GetTarget() == Target )
		{
			It.RemoveCurrent(); 
			NumRemoved++; 
		}
	}

	/*!!mdf-tbr:
	We're getting infrequent crashes due to (apparently) invalid timers remaining
	in the heap. THEORY: if an element is removed above, the heap gets reordered 
	and the iterator may skip over entries which belong to Target which are still 
	in the heap, corrupting it. This might also happen in CleanupDestroyed when
	it is trying to remove timers which are associated with deleted Actors. The
	quick fix will be to have the heap iterator reset the Index whenever 
	RemoveCurrent is called on it (alternatively we could switch to a sorted list
	which would allow for multiple removals per iteration and which would probably
	be about as efficient as a heap for the numbers of timers that we are 
	typically using). For now though, I'd like to add an assertion here in order
	to confirm that this is indeed the problem. */
	
	//NOTE: so far this doesn't seem to be the problem any more (underlying problem fixed with THeap changes?)
	if( NumRemoved )
	{
		for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
		{
			if( Target == It->NotifyInfo.GetTarget() )
			{
				debugf( NAME_Warning, L"(ASSERT) entry still in heap for Target %s in RemoveAllTimers!", Target->GetName() );
				check( 0 );
			}
		}
	}

	FTDEBUG( debugf( L"    FTimerManager::RemoveAllTimers removed %d timer(s) for target %s", NumRemoved, Target->GetName() ) );

	return NumRemoved;
	unguard;
}

FLOAT FTimerManager::TimeRemaining( FNotifyInfo NotifyInfo )
{
	guard(FTimerManager::TimeRemaining);
	FLOAT fRetVal = -1.0f;

	for( THeap<FTimerInfo>::TUnsortedIterator It(Timers); It; ++It )
		if( It->NotifyInfo == NotifyInfo )
			{ fRetVal = Max( 0.0f, (It->RingTime - GTimerAcc) ); break; }

	return fRetVal;
	unguard;
}

void UObject::execAddTimer( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execAddTimer);

	clock(FTimerManager::FTimerCycles);
	P_GET_NAME(NotifyFunc);
	P_GET_FLOAT(Frequency);
	P_GET_UBOOL_OPTX(bRepeat,false);
	P_GET_FLOAT_OPTX(RandomSpread,0.0f);
	P_GET_UBOOL_OPTX(bForceUpdate,false);
	P_FINISH;

	if( !IsValid() || IsPendingKill() || !GetOuter() || GetOuter()->IsPendingKill() || !GetOuter()->IsValid() )
	{
		debugf( L"Warning (assert): called AddTimer (%s) for invalid object", *NotifyFunc );
		check( 0 );
	}
	FTDEBUG( debugf( L"execAddTimer: %s %s", GetName(), *NotifyFunc ) );	

	FTimerManager::GetInstance()->AddTimer( FNotifyInfo(this,NotifyFunc), Frequency, bRepeat, RandomSpread, bForceUpdate );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execAddTimer );

void UObject::execAddTimerS( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execAddTimerS);

	clock(FTimerManager::FTimerCycles);
	P_GET_STR(NotifyFunc);
	P_GET_FLOAT(Frequency);
	P_GET_UBOOL_OPTX(bRepeat,false);
	P_GET_FLOAT_OPTX(RandomSpread,0.0f);
	P_GET_UBOOL_OPTX(bForceUpdate,false);
	P_FINISH;

	if( !IsValid() || IsPendingKill() || !GetOuter() || GetOuter()->IsPendingKill() || !GetOuter()->IsValid() )
	{
		debugf( L"Warning (assert): called AddTimerS (%s) for invalid object", *NotifyFunc );
		check( 0 );
	}
	FTDEBUG( debugf( L"execAddTimerS: %s %s", GetName(), *NotifyFunc ) );	

	FTimerManager::GetInstance()->AddTimer( FNotifyInfo(this,NotifyFunc), Frequency, bRepeat, RandomSpread, bForceUpdate );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execAddTimerS );

void UObject::execRemoveTimer( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execRemoveTimer);
	
	clock(FTimerManager::FTimerCycles);
	P_GET_NAME(NotifyFunc);
	P_FINISH;

	if( !IsValid() || IsPendingKill() || !GetOuter() || GetOuter()->IsPendingKill() || !GetOuter()->IsValid() )
	{
		debugf( L"Warning (assert): called RemoveTimer (%s) for invalid object", *NotifyFunc );
		check( 0 );
	}
	
	FTDEBUG( debugf( L"execRemoveTimer: %s %s", GetName(), *NotifyFunc ) );	

	//FTimerManager::GetInstance()->RemoveTimer( FNotifyInfo(this,NotifyFunc) );
	FTimerManager::GetInstance()->RemoveTimer( this, NotifyFunc );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execRemoveTimer );

void UObject::execRemoveTimerS( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execRemoveTimerS);

	clock(FTimerManager::FTimerCycles);
	P_GET_STR(NotifyFunc);
	P_FINISH;

	if( !IsValid() || IsPendingKill() || !GetOuter() || GetOuter()->IsPendingKill() || !GetOuter()->IsValid() )
	{
		debugf( L"Warning (assert): called RemoveTimerS (%s) for invalid object", *NotifyFunc );
		check( 0 );
	}
	FTDEBUG( debugf( L"execRemoveTimerS: %s %s", GetName(), *NotifyFunc ) );	

	//FTimerManager::GetInstance()->RemoveTimer( FNotifyInfo(this,NotifyFunc) );
	FTimerManager::GetInstance()->RemoveTimer( this, StringToName( NotifyFunc, true ) );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execRemoveTimerS );

void UObject::execRemoveAllTimers( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execRemoveAllTimers);

	clock(FTimerManager::FTimerCycles);
	P_FINISH;

	//FTDEBUG( debugf( L"execRemoveAllTimers: %s", GetName() ) );	

	FTimerManager::GetInstance()->RemoveAllTimers( this );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execRemoveAllTimers );

void UObject::execTimeRemaining( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execTimeRemaining);
	
	clock(FTimerManager::FTimerCycles);
	P_GET_NAME(NotifyFunc);
	P_FINISH;

	*(FLOAT*)Result = FTimerManager::GetInstance()->TimeRemaining( FNotifyInfo(this,NotifyFunc) );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execTimeRemaining );

void UObject::execTimeRemainingS( FFrame& Stack, RESULT_DECL )
{
	guard(UObject::execTimeRemainingS);
	
	clock(FTimerManager::FTimerCycles);
	P_GET_STR(NotifyFunc);
	P_FINISH;

	*(FLOAT*)Result = FTimerManager::GetInstance()->TimeRemaining( FNotifyInfo(this,NotifyFunc) );

	unclock(FTimerManager::FTimerCycles);
	unguardexec;
}
IMPLEMENT_FUNCTION( UObject, INDEX_NONE, execTimeRemainingS );

