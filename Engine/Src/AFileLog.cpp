/*=============================================================================
	AFileLog.cpp: Unreal Tournament 2003 mod author logging
	Copyright 1997-2002 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Joe Wilcox
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	Stat Log Implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(AFileLog);

void AFileLog::execOpenLog( FFrame& Stack, RESULT_DECL )
{
	guard(AFileLog::execOpenLog);

	P_GET_STR(FName);
	P_FINISH;

	// Strip all pathing characters from the name

	for (INT i=0;i<appStrlen(*FName);i++)
	{
		if ( (*FName)[i]=='\\' || (*FName)[i]=='.')
			( (TCHAR*) (*FName) )[i] = '_';
	}

	FName+=TEXT(".txt");

	LogFileName = FName;

	debugf(TEXT("Opening user log %s"),*LogFileName);

	LogAr = (INT) GFileManager->CreateFileWriter( *LogFileName, FILEWRITE_EvenIfReadOnly + FILEWRITE_Append );
	unguardexec;
}

void AFileLog::execCloseLog( FFrame& Stack, RESULT_DECL )
{
	guard(AFileLog::execCloseLog);
	P_FINISH;

	if( LogAr )
		delete (FArchive*)LogAr;
	LogAr = 0;
	unguardexec;
}


void AFileLog::execLogf( FFrame& Stack, RESULT_DECL )
{
	guard(AFileLog::execLogf);
	P_GET_STR(Data);
	P_FINISH;

	FString LogString = Data + TEXT("\r\n");
	
	ANSICHAR AnsiStr[1024];

	INT i;
	for( i=0; i<LogString.Len(); i++ )
		AnsiStr[i] = ToAnsi((*LogString)[i] );

	AnsiStr[i] = 0;
	((FArchive*)LogAr)->Serialize( AnsiStr, i );

	unguardexec;
}

