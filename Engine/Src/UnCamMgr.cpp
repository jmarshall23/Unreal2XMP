/*=============================================================================
	UnCamMgr.cpp: Unreal viewport manager, generic implementation.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"

/*-----------------------------------------------------------------------------
	UClient implementation.
-----------------------------------------------------------------------------*/

//
// Constructor.
//
UClient::UClient()
{
	guard(UClient::UClient);

	// Hook in.
	UTexture::__Client = this;

	unguard;
}
void UClient::PostEditChange()
{
	guard(UClient::PostEditChange);
	Super::PostEditChange();
	Brightness = Clamp(Brightness,0.f,1.f);
    
	Contrast = Clamp( Contrast, 0.f, 2.f );
	Gamma = Clamp( Gamma, 0.5f, 2.5f );
	Engine->UpdateGamma();

	for(TObjectIterator<UTexture> It;It;++It)
        It->bRealtimeChanged = 1;

	SaveConfig();

	unguard;
}
void UClient::Destroy()
{
	guard(UClient::Destroy);
	UTexture::__Client = NULL;
	Super::Destroy();
	unguard;
}

//
// Command line.
//
UBOOL UClient::Exec( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UClient::Exec);
	if( ParseCommand(&Cmd,TEXT("BRIGHTNESS")) )
	{
		if( *Cmd == '+' )
			Brightness = Brightness >= 0.9f ? 0.f : Brightness + 0.1f;
		else
		if( *Cmd )
			Brightness = Clamp<FLOAT>( appAtof(Cmd), 0.f, 1.f );
		else
			Brightness = 0.5f;
		Engine->UpdateGamma();
		SaveConfig();
		if( Viewports.Num() && Viewports(0)->Actor )//!!ugly
			Viewports(0)->Actor->eventClientMessage( *FString::Printf(TEXT("Brightness %i"), (INT)(Brightness*10)), NAME_None );
		return 1;
	}
	else
	if( ParseCommand(&Cmd,TEXT("CONTRAST")) )
	{
		if( *Cmd == '+' )
			Contrast = Contrast >= 0.9f ? 0.f : Contrast + 0.1f;
		else
		if( *Cmd )
			Contrast = Clamp<FLOAT>( appAtof(Cmd), 0.f, 2.f );
		else
			Contrast = 0.5f;
		Engine->UpdateGamma();
		SaveConfig();
		if( Viewports.Num() && Viewports(0)->Actor )//!!ugly
			Viewports(0)->Actor->eventClientMessage( *FString::Printf(TEXT("Contrast %i"), (INT)(Contrast*10)), NAME_None );
		return 1;
	}
	else
	if( ParseCommand(&Cmd,TEXT("GAMMA")) )
	{
		if( *Cmd == '+' )
			Gamma = Gamma >= 2.4f ? 0.5f : Gamma + 0.1f;
		else
		if( *Cmd )
			Gamma = Clamp<FLOAT>( appAtof(Cmd), 0.5f, 2.5f );
		else
			Gamma = 1.7f;
		Engine->UpdateGamma();
		SaveConfig();
		if( Viewports.Num() && Viewports(0)->Actor )//!!ugly
			Viewports(0)->Actor->eventClientMessage( *FString::Printf(TEXT("Gamma %1.1f"), Gamma), NAME_None );
		return 1;
	}
	else return 0;
	unguard;
}

//
// Init.
//
void UClient::Init( UEngine* InEngine )
{
	guard(UClient::Init);
	Engine = InEngine;
	unguard;
}

//
// Flush.
//
void UClient::Flush( UBOOL AllowPrecache )
{
	guard(UClient::Flush);

	for( INT i=0; i<Viewports.Num(); i++ )
		if( Viewports(i)->RenDev )
			Viewports(i)->RenDev->Flush( Viewports(i));

	unguard;
}

//
// Update Gamma.
//
void UClient::UpdateGamma()
{
	guard(UClient::UpdateGamma);

	for( INT i=0; i<Viewports.Num(); i++ )
		if( Viewports(i)->RenDev )
			Viewports(i)->RenDev->UpdateGamma( Viewports(i) );
	
	unguard;
}

//
// Restore Gamma.
//
void UClient::RestoreGamma()
{
	guard(UClient::RestoreGamma);

	for( INT i=0; i<Viewports.Num(); i++ )
		if( Viewports(i)->RenDev )
			Viewports(i)->RenDev->RestoreGamma();
	
	unguard;
}

//
// Serializer.
//
void UClient::Serialize( FArchive& Ar )
{
	guard(UClient::Serialize);
	Super::Serialize( Ar );

	// Only serialize objects, since this can't be loaded or saved.
	Ar << Viewports;

	unguard;
}

INT UClient::GetTextureLODBias (ELODSet LODSet)
{
	guard(UClient::GetTextureLODBias);
    check (LODSet < LODSET_MAX);

    return (TextureLODSet[LODSet] - 4);

	unguard;
}

IMPLEMENT_CLASS(UClient);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

