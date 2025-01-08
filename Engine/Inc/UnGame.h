/*=============================================================================
	UnGame.h: Unreal game class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Unreal game engine.
-----------------------------------------------------------------------------*/

//
// The Unreal game engine.
//
class ENGINE_API UGameEngine : public UEngine
{
	DECLARE_CLASS(UGameEngine,UEngine,CLASS_Config|CLASS_Transient,Engine)

	// Variables.
	ULevel*			GLevel;
	ULevel*			GEntry;
	UPendingLevel*	GPendingLevel;
	FURL			LastURL;
	TArrayNoInit<FString> ServerActors;
	TArrayNoInit<FString> ServerPackages;

	TArray<UPackageCheckInfo*> PackageValidation;	// The Database of allowed MD5s
	UObject* MD5Package;

	UBOOL bCheatProtection;

    FStringNoInit   MainMenuClass;
	FStringNoInit	InitialMenuClass;
    FStringNoInit   ConnectingMenuClass;
    FStringNoInit   DisconnectMenuClass;
	FStringNoInit	LoadingClass;

	// Constructors.
	UGameEngine();

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

	// UEngine interface.
	void Init();
	void Tick( FLOAT DeltaSeconds );
	void Draw( UViewport* Viewport, UBOOL Blit=1, BYTE* HitData=NULL, INT* HitSize=NULL );
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );
	void MouseDelta( UViewport*, DWORD, FLOAT, FLOAT );
	void MousePosition( class UViewport*, DWORD, FLOAT, FLOAT );
	void MouseWheel( UViewport* Viewport, DWORD Buttons, INT Delta );
	void Click( UViewport*, DWORD, FLOAT, FLOAT );
	void UnClick( UViewport*, DWORD, INT, INT );
	void SetClientTravel( UPlayer* Viewport, const TCHAR* NextURL, UBOOL bItems, ETravelType TravelType );
	FLOAT GetMaxTickRate();
	INT ChallengeResponse( INT Challenge );
	void SetProgress( const TCHAR* Str1, const TCHAR* Str2, FLOAT Seconds );

	// UGameEngine interface.
	virtual UBOOL Browse( FURL URL, const TMap<FString,FString>* TravelInfo, FString& Error );
	virtual ULevel* LoadMap( const FURL& URL, UPendingLevel* Pending, const TMap<FString,FString>* TravelInfo, FString& Error );
	virtual void SaveGame( INT Position );
	virtual void CancelPending();
	virtual void PaintProgress();
	virtual void UpdateConnectingMessage();
	virtual void BuildServerMasterMap( UNetDriver* NetDriver, ULevel* InLevel );
	virtual void NotifyLevelChange();
	void FixUpLevel();

	//@@Cheat Protection

	UBOOL CheckForRogues();					// Check to see if any rogue packages (not in Package Map) exist
	void AuthorizeClient(ULevel* Level);	// Have the server authorize any packages with Code in them
	
	void ServerUpdateMD5();	// Update the MD5 Database with any MD5's from ServerPackages

	UBOOL ValidatePackage(const TCHAR* GUID, const TCHAR* MD5);

	int PackageRevisionLevel();	// Figures out the top most package revision level
	void SaveMD5Database();		// Save the MD5 Database back out
	void AddMD5(const TCHAR* GUID, const TCHAR* MD5, int Revision);
	void DefaultMD5();	// Make sure the MD5's of everything in EditPackages is added

	FString InitSpecial(const TCHAR* Special);	// Performs a full MD5 on a given filename

};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/


