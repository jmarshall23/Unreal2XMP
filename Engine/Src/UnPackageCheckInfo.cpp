/*=============================================================================
	UnPackageCheckInfo : Stores allowable MD5s for each package
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Joe Wilcox
	
=============================================================================*/

#include "EnginePrivate.h"

IMPLEMENT_CLASS(UPackageCheckInfo);

UBOOL UPackageCheckInfo::VerifyID(FString CurrentId)
{
	// Look for this id in the chain

	for ( int i=0; i<AllowedIDs.Num(); i++ )
	{
		if ( CurrentId == AllowedIDs(i) )
			return true;
	}

	return false;
}

void UPackageCheckInfo::Serialize( FArchive& Ar )
{

	guard(UPackageCheckInfo::Serialize);

	Super::Serialize( Ar );

	if( Ar.IsLoading() )		// Load in the block
	{
		FString TmpStr;
		Ar << PackageID;		// Get the Package's GUID
		Ar << AllowedIDs;		// Save the array
		if( Ar.Ver() > 120 )
		{
			Ar << Native;
			Ar << RevisionLevel;
		}
		else
		{
			Native=false;
			RevisionLevel=0;
		}

	}
	else if ( Ar.IsSaving() )// && Native)						// Save out the block
	{
		Ar << PackageID;
		Ar << AllowedIDs;
		Ar << Native;
		Ar << RevisionLevel;
	}

	unguard;

}

