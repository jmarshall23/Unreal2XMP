/*=============================================================================
	UnCon.cpp: Implementation of UConsole class
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"

/*------------------------------------------------------------------------------
	UConsole object implementation.
------------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UConsole);
NATIVE_INFO(UConsole) GEngineUConsoleNatives[] =
{
	MAP_NATIVE(UConsole, execConsoleCommand)
	MAP_NATIVE(UConsole, execConsoleCommandEx)
	{NULL, NULL}
};
IMPLEMENT_NATIVE_HANDLER(Engine,UConsole);

/*------------------------------------------------------------------------------
	Console.
------------------------------------------------------------------------------*/

//
// Constructor.
//
UConsole::UConsole()
{}

//
// Init console.
//
void UConsole::_Init( UViewport* InViewport )
{
	guard(UConsole::_Init);
	VERIFY_CLASS_SIZE(UConsole);

	// Set properties.
	Viewport		= InViewport;

	// Init scripting.
	InitExecution();

	// Create plug-ins.
	TArray<UObject*> &DefaultPlugIn=CastChecked<UConsole>(GetClass()->GetDefaultObject())->PlugIn;	// NOTE[aleiby]: Use the default since TArrays don't get copied over to instances.
	for( INT i=0; i<DefaultPlugIn.Num(); i++ )
	{
		UClass* PlugInClass=CastChecked<UClass>(DefaultPlugIn(i));
		if( PlugInClass && PlugInClass->IsChildOf( UConsolePlugIn::StaticClass() ) )
		{
			//debugf(TEXT("Adding console plug-in: %s"), PlugInClass->GetName() );
			StaticLoadObject( UClass::StaticClass(), NULL, PlugInClass->GetPathName(), NULL, LOAD_NoWarn, NULL );
			UObject* Obj = StaticConstructObject( PlugInClass, this );
			PlugIn.AddItem( Obj );
			AddExecListener( Obj );
		}
		else debugf( NAME_Warning, TEXT("%s is not a ConsolePlugIn."), PlugInClass ? PlugInClass->GetName() : TEXT("None") );
	}

	unguard;
}

void UConsole::Destroy()
{
	guard(UConsole::Destroy);
	for( INT i=0; i<PlugIn.Num(); i++ )
	{
		RemoveExecListener( PlugIn(i) );
		delete PlugIn(i);
	}
	PlugIn.Empty();
	ExecListeners.Empty();
	Super::Destroy();
	unguard;
}

void UConsole::AddExecListener( UObject* Listener )
{
	guard(UConsole::AddExecListener);
	if( Listener )
		ExecListeners.AddItem( Listener );
	unguard;
}
void UConsole::RemoveExecListener( UObject* Listener )
{
	guard(UConsole::RemoveExecListener);
	ExecListeners.RemoveItem( Listener );
	unguard;
}

/*------------------------------------------------------------------------------
	Viewport console output.
------------------------------------------------------------------------------*/

//
// Print a message on the playing screen.
// Time = time to keep message going, or 0=until next message arrives, in 60ths sec
//
void UConsole::Serialize( const TCHAR* Data, EName ThisType )
{
	guard(UConsole::Serialize);
	GLog->Log( Data );
	eventMessage( Data, 6.0 );
	unguard;
}

void UConsole::execConsoleCommand( FFrame& Stack, RESULT_DECL )
{
	guardSlow(UConsole::execConsoleCommand);

	P_GET_STR(S);
	P_FINISH;

	*(DWORD*)Result = ConsoleCommand(*S);

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( UConsole, INDEX_NONE, execConsoleCommand );

void UConsole::execConsoleCommandEx( FFrame& Stack, RESULT_DECL )
{
	guardSlow(UConsole::execConsoleCommandEx);

	P_GET_STR(S);
	P_FINISH;

	ConsoleCommand(*S,(FString*)Result);

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( UConsole, INDEX_NONE, execConsoleCommandEx );

void UConsole::execAddExecListener( FFrame& Stack, RESULT_DECL )
{
	guardSlow(UConsole::execAddExecListener);

	P_GET_OBJECT(UObject,Listener);
	P_FINISH;

	AddExecListener( Listener );

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( UConsole, INDEX_NONE, execAddExecListener );

void UConsole::execRemoveExecListener( FFrame& Stack, RESULT_DECL )
{
	guardSlow(UConsole::execRemoveExecListener);

	P_GET_OBJECT(UObject,Listener);
	P_FINISH;

	RemoveExecListener( Listener );

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( UConsole, INDEX_NONE, execRemoveExecListener );

/*------------------------------------------------------------------------------
	Rendering.
------------------------------------------------------------------------------*/

UBOOL UConsole::GetDrawWorld()
{
	guard(UConsole::GetDrawWorld);

	return !bNoDrawWorld;
	unguard;
}

//
// Called before rendering the world view.  Here, the
// Viewport console code can affect the screen's Viewport,
// for example by shrinking the view according to the
// size of the status bar.
//
void UConsole::PreRender( FSceneNode* Frame )
{
	guard(UConsole::PreRender);

	// Prevent status redraw due to changing.
	Tick( Viewport->CurrentTime - Viewport->LastUpdateTime );

	unguard;
}

//
// Refresh the player console on the specified Viewport.  This is called after
// all in-game graphics are drawn in the rendering loop, and it overdraws stuff
// with the status bar, menus, and chat text.
//
void UConsole::PostRender( FSceneNode* Frame )
{
	guard(UConsole::PostRender);

	eventPostRender( Frame->Viewport->Canvas );
	for( INT i=0; i<PlugIn.Num(); i++ )
		CastChecked<UConsolePlugIn>(PlugIn(i))->eventPostRender( Frame->Viewport->Canvas );

	FrameX = Frame->Viewport->SizeX;
	FrameY = Frame->Viewport->SizeY;

	unguard;
}

UBOOL UConsole::HandleExec( const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor )
{
	guard(UConsole::HandleExec);

	if( ScriptConsoleExec(Cmd,Ar,Executor) )
	{
		return 1;
	}
	else
	{
		for( INT i=0; i<ExecListeners.Num(); i++ )
			if( ExecListeners(i)->ScriptConsoleExec(Cmd,Ar,Executor) )
				return 1;
		return 0;
	}

	unguard;
}

/*------------------------------------------------------------------------------
	ConsolePlugIn.
------------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UConsolePlugIn);

/*------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------*/
