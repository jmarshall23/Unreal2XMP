/*=============================================================================
	UnSecurity.cpp: See .UC for for info
	Copyright 1997-2001 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Joe Wilcox
=============================================================================*/

#include "EnginePrivate.h"
#include "UnLinker.h"

IMPLEMENT_CLASS(ASecurity);

//
// Perform various aspects of Cheat Protection
//
void ASecurity::execNativePerform( FFrame& Stack, RESULT_DECL )
{
	guard(ASecurity::execNativePerform::_Init);

	P_GET_INT(SecType)		// Type of security to perform
	P_GET_STR(Param1);	// Parameter #1
	P_GET_STR(Param2);	// Parameter #2

	P_FINISH;

	TArray<UObject*>	ObjLoaders = UObject::GetLoaderList(); 

	switch (SecType)
	{
		case 0:	//  Return a QuickMD5 for a selected package
			{

				for( INT i=0; i<ObjLoaders.Num(); i++ )
				{
	
					ULinker * Linker = CastChecked<ULinker>( ObjLoaders(i) );
					if (!appStricmp (Linker->LinkerRoot->GetName(), *Param1) )
					{
						eventServerCallback( SecType, Linker->QuickMD5());
						return;
					}
				}

				eventServerCallback(255,TEXT("Package Not Loaded"));
				return;
				break;
			}

		case 1: // Return a Code MD5 on a function
			{
			
				FString Test = FString::Printf(TEXT("function %s"), *Param1);

				for( TObjectIterator<UStruct> It ; It ; ++It )
				{

					if (!appStricmp(It->GetFullName(),*Test) )
					{
						eventServerCallback(SecType, It->FunctionMD5());
						return;
									
					}
				}

				eventServerCallback(255,TEXT("Function Not Found"));
				return;
				break;
			}

		case 2:	// Return a full MD5 on a file
			{
				FArchive* MD5Ar = GFileManager->CreateFileReader( *Param1 );
				int BytesToRead;
				if( !MD5Ar )
				{
						eventServerCallback(255,TEXT("File was not found"));
						return;
				}

				BYTE* MD5Buffer = (BYTE*)appMalloc(32767, TEXT(""));
	
				FMD5Context PMD5Context;
				appMD5Init( &PMD5Context );
			
				while ( MD5Ar->Tell() < MD5Ar->TotalSize() )
				{
					BytesToRead = MD5Ar->TotalSize() - MD5Ar->Tell();
					if (BytesToRead>32767)
						BytesToRead=32767;

					MD5Ar->Serialize(MD5Buffer, BytesToRead);
					appMD5Update( &PMD5Context, MD5Buffer, BytesToRead);
				}
				BYTE Digest[16];
				appMD5Final( Digest, &PMD5Context );


				// Convert to a string

				FString FullMD5;
				for (int i=0; i<16; i++)
					FullMD5 += FString::Printf(TEXT("%02x"), Digest[i]);	

				eventServerCallback(SecType, FullMD5);

				// Free the buffer
	
				appFree(MD5Buffer);

				delete MD5Ar;
				break;
			}

		case 3:
			{

				FString Packages;
				for( INT i=0; i<ObjLoaders.Num(); i++ )
				{
					ULinker * Linker = CastChecked<ULinker>( ObjLoaders(i) );
					Packages += Linker->LinkerRoot->GetName();
					if (i<ObjLoaders.Num()-1)
						Packages += TEXT("|");
				}
				eventServerCallback(SecType, Packages);
			}
	}

	unguardexec;
}

