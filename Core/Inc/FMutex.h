/*=============================================================================
	FMutex.h: Simple thead locking using semaphores provided by the OS.
	Copyright 2001 Legend Entertainment. All Rights Reserved.

	Revision history:
		* Written by Aaron Leiby
=============================================================================*/

#ifndef _INC_FMUTEX
#define _INC_FMUTEX

/*----------------------------------------------------------------------------
	Includes.
----------------------------------------------------------------------------*/

#include <windows.h>

/*----------------------------------------------------------------------------
	Macros.
----------------------------------------------------------------------------*/

#define SYNCHRONIZED(func) \
	static FSemaphore Mutex(#func); \
	FMutexLock Lock(Mutex);

/*----------------------------------------------------------------------------
	Example usage.
----------------------------------------------------------------------------*/

#if 0 //example

INT MyReEntrantFunction()
{
	SYNCHRONIZED(MyReEntrantFunction)	// lock remains for entirety of scope.
	//
	// function body
	//
	return 5+7;	// even works here still.
}

#endif

/*----------------------------------------------------------------------------
	FSemaphore.
----------------------------------------------------------------------------*/

class CORE_API FSemaphore
{
public:

	FSemaphore( const char* const InName=0 )
	{
		Handle = ::CreateMutex(0,0,InName);
		if(!Handle)
		{
			if(GetLastError() == ERROR_ALREADY_EXISTS)
			{
				// mutex is already created, try to open it.
				Handle = ::OpenMutex(MUTEX_ALL_ACCESS|SYNCHRONIZE,0,InName);
				if(!Handle)
					appThrowf( TEXT("Failed to open Mutex: %s"), InName );
			}
			else appThrowf( TEXT("Failed to create Mutex: %s"), InName );
		}
	}
	~FSemaphore()
	{
		if(!::CloseHandle((HANDLE)Handle))
			appThrowf( TEXT("Failed to close Mutex: %d"), Handle );
	}

	FSemaphore& Request(const DWORD InTimeout=0)
	{
		if(::WaitForSingleObject((HANDLE)Handle,InTimeout?InTimeout:INFINITE)==WAIT_FAILED)
			appThrowf( TEXT("Mutex wait failed: %d %d"), Handle, InTimeout );
		return *this;
	}
	FSemaphore& Release()
	{
		if(!::ReleaseMutex((HANDLE)Handle))
			appThrowf( TEXT("Mutex failed to release: %d"), Handle );
		return *this;
	}

private:
	// disable copy constructor and assignment operator.
	FSemaphore(const FSemaphore&){}
	FSemaphore operator=(const FSemaphore&){}

	DWORD Handle;
};

/*----------------------------------------------------------------------------
	FMutexLock.
----------------------------------------------------------------------------*/

class CORE_API FMutexLock
{
public:

	FMutexLock( FSemaphore& InMutex, const DWORD InTimeout=0 )
		: Mutex(InMutex)
		{ Mutex.Request(InTimeout); }
	~FMutexLock()
		{ Mutex.Release(); }

private:
	// disable copy constructor and assignment operator.
	FMutexLock(const FMutexLock&){}
	FMutexLock operator=(const FMutexLock&){}

	FSemaphore& Mutex;
};

#endif // _INC_FMUTEX

