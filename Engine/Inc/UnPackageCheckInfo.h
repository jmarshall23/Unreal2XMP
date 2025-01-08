/*=============================================================================
	UnPackageCheckInfo : Stores allowable MD5s for each package
	Copyright 2001 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Joe wilcox
=============================================================================*/

/*
	UnPackageCheckInfo - Stores allowable MD5 info a given package
*/

class ENGINE_API UPackageCheckInfo : public UObject
{
	DECLARE_CLASS(UPackageCheckInfo,UObject,0,Engine);
	NO_DEFAULT_CONSTRUCTOR(UPackageCheckInfo);

public:

	// Variables.

	FString			PackageID;			// The GUID these MD5's apply for
	TArray<FString> AllowedIDs;			// A list of allowed MD5s
	UBOOL			Native;				// Did this entry come from the master server or from the ini file 
	int				RevisionLevel;		// The revision level of this CheckInfo

	void Serialize( FArchive& Ar );			
	//void PostLoad();

	virtual UBOOL VerifyID(FString CurrentId);	// Check to see if an MD5 is active
};

