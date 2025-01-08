/*=============================================================================
	UnIn.cpp: Unreal input system.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"

/*-----------------------------------------------------------------------------
	Internal.
-----------------------------------------------------------------------------*/

class FInputVarCache
{
public:
	INT Count;
	UProperty* Properties[ZEROARRAY];
	static FInputVarCache* Get( UClass* Class, FMemCache::FCacheItem*& Item )
	{
		QWORD CacheId = MakeCacheID( CID_InputMap, Class );
		FInputVarCache* Result = (FInputVarCache*)GCache.Get(CacheId,Item);
		if( !Result )
		{
			INT Count=0, Temp=0;
			TFieldIterator<UProperty,CLASS_IsAUProperty> It(Class);
			for( ; It; ++It )
				if( It->PropertyFlags & CPF_Input )
					Count++;
			Result = (FInputVarCache*)GCache.Create(CacheId,Item,sizeof(FInputVarCache)+Count*sizeof(UProperty*));
			Result->Count = Count;
			for( It=TFieldIterator<UProperty,CLASS_IsAUProperty>(Class); It; ++It )
				if( It->PropertyFlags & CPF_Input )
					Result->Properties[Temp++] = *It;
		}
		return Result;
	}
};

/*-----------------------------------------------------------------------------
	Implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UInput);

/*-----------------------------------------------------------------------------
	UInput creation and destruction.
-----------------------------------------------------------------------------*/

//
// Temporary.
// !!should be moved to an UnrealScript definition.
//
void UInput::StaticInitInput()
{
	guard(UInput::StaticInitInput);
	FArchive ArDummy;

	// Create input alias struct.
	UStruct* AliasStruct = new(StaticClass(),TEXT("Alias"))UStruct( NULL );
	AliasStruct->SetPropertiesSize( sizeof(FName) + sizeof(FString));
	new(AliasStruct,TEXT("Alias"),  RF_Public)UNameProperty( EC_CppProperty, 0,             TEXT(""), CPF_Config );
	new(AliasStruct,TEXT("Command"),RF_Public)UStrProperty ( EC_CppProperty, sizeof(FName), TEXT(""), CPF_Config );
	AliasStruct->Link( ArDummy, 0 );

	// Add alias list to class.
	UStructProperty* Q = new(StaticClass(),TEXT("Aliases"),RF_Public)UStructProperty( CPP_PROPERTY(Aliases), TEXT("Aliases"), CPF_Config, AliasStruct );
	Q->ArrayDim = ALIAS_MAX;

	// Add key list.
	UEnum* InputKeys = FindObjectChecked<UEnum>( UConsole::StaticClass(), TEXT("EInputKey") );
	for( INT i=0; i<IK_MAX; i++ )
	{
		if( InputKeys->Names(i)!=NAME_None )
		{
			const TCHAR* Str = *InputKeys->Names(i);
			new(StaticClass(),Str+3,RF_Public)UStrProperty( EC_CppProperty, (INT)&((UInput*)NULL)->Bindings[i], TEXT("RawKeys"), CPF_Config );
		}
	}
	StaticClass()->Link( ArDummy, 0 );

	// Load defaults.
	StaticClass()->GetDefaultObject()->LoadConfig( 1 );

	unguard;
}

//
// Constructor.
//
UInput::UInput()
{
	guard(UInput::UInput);
	InputKeys = FindObjectChecked<UEnum>( UConsole::StaticClass(), TEXT("EInputKey") );
	unguard;
}

//
// Class initializer.
//
void UInput::StaticConstructor()
{
	guard(UInput::StaticConstructor);
	unguard;
}

//
// Serialize
//
void UInput::Serialize( FArchive& Ar )
{
	guard(UInput::Serialize);
	Super::Serialize( Ar );
	Ar << InputKeys;
	unguard;
}

//
// Find a button.
//
BYTE* UInput::FindButtonName( AActor* Actor, const TCHAR* ButtonName ) const
{
	guard(UInput::FindButtonName);
	check(Viewport);
	check(Actor);
	FName Button( ButtonName, FNAME_Find );
	if( Button != NAME_None )
	{
		FMemCache::FCacheItem* Item;
		FInputVarCache* Cache = FInputVarCache::Get( Actor->GetClass(), Item );
		INT i;
		for( i=0; i<Cache->Count; i++ )
			if
			(	Cache->Properties[i]->GetFName()==Button
			&&	Cast<UByteProperty>(Cache->Properties[i]) )
				break;
		Item->Unlock();
		if( i<Cache->Count )
			return (BYTE*)Actor + Cache->Properties[i]->Offset;
	}
	return NULL;
	unguard;
}

//
// Find an axis.
//
FLOAT* UInput::FindAxisName( AActor* Actor, const TCHAR* ButtonName ) const
{
	guard(UInput::FindAxisName);
	check(Viewport);
	check(Actor);
	FName Button( ButtonName, FNAME_Find );
	if( Button != NAME_None )
	{
		FMemCache::FCacheItem* Item;
		FInputVarCache* Cache = FInputVarCache::Get( Actor->GetClass(), Item );
		INT i;
		for( i=0; i<Cache->Count; i++ )
			if
			(	Cache->Properties[i]->GetFName()==Button
			&&	Cast<UFloatProperty>(Cache->Properties[i]) )
				break;
		Item->Unlock();
		if( i<Cache->Count )
			return (FLOAT*)((BYTE*)Actor + Cache->Properties[i]->Offset);
	}
	return NULL;
	unguard;
}

//
// Execute input commands.
//
void UInput::ExecInputCommands( const TCHAR* Cmd, FOutputDevice& Ar )
{
	guard(UInput::ExecInputCommands);
	TCHAR Line[256];
	while( ParseLine( &Cmd, Line, ARRAY_COUNT(Line)) )
	{
		const TCHAR* Str = Line;
		if( Action==IST_Press || (Action==IST_Release && ParseCommand(&Str,TEXT("OnRelease"))) )
			Viewport->Exec( Str, Ar );
		else
			Exec( Str, Ar );
	}
	unguard;
}

//
// Init.
//
void UInput::Init( UViewport* InViewport )
{
	guard(UInput::Init);

	// Set objects.
	Viewport = InViewport;

	// Reset.
	ResetInput();

	debugf( NAME_Init, TEXT("Input system initialized for %s"), Viewport->GetName() );
	unguard;
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

//
// Execute a command.
//
UBOOL UInput::Exec( const TCHAR* Str, FOutputDevice& Ar )
{
	guard(UInput::Exec);
	TCHAR Temp[256];
	static UBOOL InAlias=0;
	if( ParseCommand( &Str, TEXT("BUTTON") ) )
	{
		// Normal button.
		BYTE* Button;
		if
		(	Viewport->Actor
		&&	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
		{
			if	( (Button=FindButtonName(Viewport->Actor,Temp))!=NULL )
			{
				if( GetInputAction() == IST_Press )
					*Button = 1;
				else if( GetInputAction()==IST_Release && *Button )
					*Button = 0;
			}
			else if	( Viewport->Actor->Pawn
					&& (Button=FindButtonName(Viewport->Actor->Pawn,Temp))!=NULL )
			{
				if( GetInputAction() == IST_Press )
					*Button = 1;
				else if( GetInputAction()==IST_Release && *Button )
					*Button = 0;
			}
			else Ar.Log( TEXT("Bad Button command") );
		}
		else Ar.Log( TEXT("Bad Button command") );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("PULSE") ) )
	{
		// Normal button.
		BYTE* Button;
		if
		(	Viewport->Actor
		&&	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
		{
			if	( (Button=FindButtonName(Viewport->Actor,Temp))!=NULL )
			{
				if( GetInputAction() == IST_Press )
					*Button = 1;
			}
			else if	( Viewport->Actor->Pawn
					&& (Button=FindButtonName(Viewport->Actor->Pawn,Temp))!=NULL )
			{
				if( GetInputAction() == IST_Press )
					*Button = 1;
			}
			else Ar.Log( TEXT("Bad Button command") );
		}
		else Ar.Log( TEXT("Bad Button command") );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("TOGGLE") ) )
	{
		// Toggle button.
		BYTE* Button;
		if
		(	Viewport->Actor
		&&	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 )
		&&	((Button=FindButtonName(Viewport->Actor,Temp))!=NULL) )
		{
			if( GetInputAction() == IST_Press )
				*Button ^= 0x80;
		}
		else Ar.Log( TEXT("Bad Toggle command") );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("AXIS") ) )
	{
		// Axis movement.
		FLOAT* Axis;
		if
		(	Viewport->Actor
		&&	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) 
		&& (Axis=FindAxisName(Viewport->Actor,Temp))!=NULL )
		{
			FLOAT Speed=1.f, SpeedBase=0.f, DeadZone=0.f;
			INT Invert=1;
			Parse( Str, TEXT("SPEED="), Speed );
			Parse( Str, TEXT("SPEEDBASE="), SpeedBase );
			Parse( Str, TEXT("INVERT="), Invert );
			Parse( Str, TEXT("DEADZONE="), DeadZone );

			if( SpeedBase > 0.f )
			{
				if( Abs(Speed) > DeadZone ) 
				{
					if( Speed > 0 )
						Speed = (Speed-DeadZone)/(1.f-DeadZone);
					else
						Speed = -(-Speed-DeadZone)/(1.f-DeadZone);

					*Axis += SpeedBase * Speed * Invert;
				}
			} else {
				if( GetInputAction() == IST_Axis )
				{
					*Axis += 0.01f * GetInputDelta() * Speed * Invert;
				}
				else if( GetInputAction() == IST_Hold )
				{
					*Axis += GetInputDelta() * Speed * Invert;
				}
			}
		}
		else Ar.Logf( TEXT("%s Bad Axis command"),Str );
		return 1;
	}
	else if ( ParseCommand( &Str, TEXT("COUNT") ) )
	{
		BYTE *Count;
		if
		(	Viewport->Actor
		&&	ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) 
		&& (Count=FindButtonName(Viewport->Actor,Temp))!=NULL )
		{
			*Count += 1;
		}
		else Ar.Logf( TEXT("%s Bad Count command"),Str );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("KEYNAME") ) )
	{
		INT keyNo = appAtoi(Str);
		Ar.Log( GetKeyName(EInputKey(keyNo)) );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("LOCALIZEDKEYNAME") ) )
	{
		INT Key = appAtoi(Str);
		Ar.Log( GetLocalizedKeyName(EInputKey(Key)) );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("KEYBINDING") ) )
	{
		EInputKey iKey;
		if( FindKeyName(Str,iKey) && Bindings[iKey].Len() )
			Ar.Log( *Bindings[iKey] );

		return 1;
	}
	else if( ParseCommand( &Str, TEXT("GETKEYS") ) )
	{
		FString Keys=TEXT("");
		INT Index=-1; FString Binding=TEXT("");
		Parse( Str, TEXT("INDEX="), Index );
		FString(Str).Split(TEXT("BINDING="),NULL,&Binding);	// NOTE[aleiby]: Parse doesn't handle pipes | very well.
		if (Index>=0)
		{
			for (INT i=0;i<ARRAY_COUNT(Bindings);i++)
			{
				if (Binding==Bindings[i])
				{
					if (--Index<0)
					{
						Keys = GetLocalizedKeyName(EInputKey(i));
						break;
					}
				}
			}
		}
		else
		{
			for (INT i=0;i<ARRAY_COUNT(Bindings);i++)
			{
				if (Binding==Bindings[i])
				{
					if (Keys) Keys+=TEXT("|");
					Keys += GetLocalizedKeyName(EInputKey(i));
				}
			}
		}
		Ar.Log( *Keys );
		return 1;
	}
	else if( ParseCommand( &Str, TEXT("DUMPKEYS") ) )
	{
		for (INT i=0;i<ARRAY_COUNT(Bindings);i++)
			Ar.Logf(TEXT("%s=%s"),GetKeyName(EInputKey(i)),*Bindings[i]);
		return 1;
	}
	else if( !InAlias && ParseToken( Str, Temp, ARRAY_COUNT(Temp), 0 ) )
	{
		FName Name(Temp,FNAME_Find);
		if( Name!=NAME_None )
		{
			for( INT i=0; i<ARRAY_COUNT(Aliases); i++ )
			{
				if( Aliases[i].Alias==Name )
				{
					guard(ExecAlias);
					InAlias=1;
					ExecInputCommands( *Aliases[i].Command, Ar );
					InAlias=0;
					unguard;
					return 1;
				}
			}
		}
	}
	return 0;
	unguard;
}

/*-----------------------------------------------------------------------------
	Key and axis movement processing.
-----------------------------------------------------------------------------*/

//
// Preprocess input to maintain key tables.
//
UBOOL UInput::PreProcess( EInputKey iKey, EInputAction State, FLOAT Delta )
{
	guard(UInput::PreProcess);
	switch( State )
	{
		case IST_Press:
			if( KeyDownTable[iKey] )
				return 0;
			KeyDownTable[iKey] = 1;
			return 1;
		case IST_Release:
			if( !KeyDownTable[iKey] )
				return 0;
			KeyDownTable[iKey] = 0;
			return 1;
		default:
			return 1;
	}
	unguard;
}

//
// Process input. Returns 1 if handled, 0 if not.
//
UBOOL UInput::Process( FOutputDevice& Ar, EInputKey iKey, EInputAction State, FLOAT Delta )
{
	guard(UInput::Process);
	check(iKey>=0&&iKey<IK_MAX);

	// Make sure there is a binding.
	if( Bindings[iKey].Len() )
	{
		// Process each line of the binding string.
		SetInputAction( State, Delta );
		ExecInputCommands( *Bindings[iKey], Ar );
		SetInputAction( IST_None );
		return 1;
	}
	else return 0;
	unguard;
}

//
// Direct axis input of variable influence.
//
void UInput::DirectAxis( EInputKey iKey, FLOAT Length, FLOAT Delta )
{
	FString Binding = *Bindings[iKey];
	FString Left	= Binding;
	FString Right	= Binding;

	while( Binding.Len() )
	{
		if ( !Binding.Split(TEXT("|"),&Left,&Right) )
		{
			Left	= Binding;
			Binding	= TEXT("");
		}
		else
			Binding = Right;

		FString Command = FString::Printf(TEXT("%s Speed=%f"), *Left, Length);
		SetInputAction( IST_Hold, Delta );
		ExecInputCommands( *Command, *GLog );
		SetInputAction( IST_None );
	}
}

/*-----------------------------------------------------------------------------
	Input reading.
-----------------------------------------------------------------------------*/

//
// Read input for the viewport.
//
void UInput::ReadInput( FLOAT DeltaSeconds, FOutputDevice& Ar )
{
	guard(UInput::ReadInput);

	if (GIsRunning)
	{
		FMemCache::FCacheItem*     Item  = NULL;
		FInputVarCache* Cache = FInputVarCache::Get( Viewport->Actor->GetClass(), Item );

		// Update everything with IST_Hold.
		if( DeltaSeconds!=-1.f )
			for( INT i=0; i<IK_MAX; i++ )
				if( KeyDownTable[i] )
					Process( *GLog, (EInputKey)i, IST_Hold, DeltaSeconds );

		// Scale the axes.
        FLOAT Scale = (DeltaSeconds != -1.f) ? (20.f / DeltaSeconds) : 0.f;

		for( INT i=0; i<Cache->Count; i++ )
			if( Cast<UFloatProperty>(Cache->Properties[i]) )
				*(FLOAT*)((BYTE*)Viewport->Actor + Cache->Properties[i]->Offset) *= Scale;

		Item->Unlock();
	}

	unguard;
}

/*-----------------------------------------------------------------------------
	Input resetting.
-----------------------------------------------------------------------------*/

//
// Reset the input system's state.
//
void UInput::ResetInput()
{
	guard(UInput::ResetInput);
	check(Viewport);

	// Reset all keys.
	for( INT i=0; i<IK_MAX; i++ )
		KeyDownTable[i] = 0;

	// Reset all input bytes.
	for( TFieldIterator<UByteProperty> ItB(Viewport->Actor->GetClass()); ItB; ++ItB )
		if( ItB->PropertyFlags & CPF_Input )
			*(BYTE *)((BYTE*)Viewport->Actor + ItB->Offset) = 0;

	// Reset all input floats.
	for( TFieldIterator<UFloatProperty> ItF(Viewport->Actor->GetClass()); ItF; ++ItF )
		if( ItF->PropertyFlags & CPF_Input )
			*(FLOAT *)((BYTE*)Viewport->Actor + ItF->Offset) = 0;

	// Set the state.
	SetInputAction( IST_None );

	// Reset viewport input.
	Viewport->UpdateInput( 1, 0 );

	unguard;
}

/*-----------------------------------------------------------------------------
	Utility functions.
-----------------------------------------------------------------------------*/

//
// Return the name of a key.
//
const TCHAR* UInput::GetKeyName( EInputKey Key ) const
{
	guard(UInput::GetKeyName);
	if( Key>=0 && Key<IK_MAX )
		if( appStrlen(*InputKeys->Names(Key)) > 3 )
			return *InputKeys->Names(Key)+3;
	return TEXT("");
	unguard;
}


//
// Return the localized name of a key.
//
const TCHAR* UInput::GetLocalizedKeyName( EInputKey Key ) const
{
	guard(UInput::GetLocalizedKeyName);
	if( Key>=0 && Key<IK_MAX )
#if WIN32
	{
		TCHAR* KeyName = appStaticString1024();

		if(	Key == IK_LeftMouse
		||	Key == IK_RightMouse
		||	Key == IK_MiddleMouse
		||	Key == IK_Pause
		||	Key == IK_CapsLock
		||	Key == IK_Tab
		||	Key	== IK_Enter
		||	Key == IK_Shift
		||	Key == IK_NumLock
		||	Key == IK_Escape
		||  ( (Key >= IK_Joy1   ) && (Key <= IK_Joy16)          )
		||  ( (Key >= IK_MouseX ) && (Key <= IK_MouseWheelDown) )
		||  ( (Key >= IK_JoyX   ) && (Key <= IK_JoyR)           )
		||	( (Key >= IK_NumPad0) && (Key <= IK_GreySlash)		)
		||	( (Key >= IK_Space)   && (Key <= IK_Help)			)
		)
			appStrncpy( KeyName, Localize(TEXT("KeyNames"), GetKeyName( Key ), TEXT("Engine"), NULL ), 1024 );
		else	
			appStrncpy( KeyName, Viewport->GetLocalizedKeyName( Key ), 1024);		
		
		// Sanity fallback.
		if( appStrcmp( KeyName, TEXT("UNKNOWN")) == 0 )
			appStrncpy( KeyName, GetKeyName( Key ), 1024);

		return appStrupr(KeyName);
	}
#else
	{
		return GetKeyName( Key );
	}
#endif
	return TEXT("");
	unguard;
}



//
// Find the index of a named key.
//
UBOOL UInput::FindKeyName( const TCHAR* KeyName, EInputKey& iKey ) const
{
	guard(UInput::FindKeyName);
	TCHAR Temp[256];
	appSprintf( Temp, TEXT("IK_%s"), KeyName );
	FName N( Temp, FNAME_Find );
	if( N != NAME_None )
		return InputKeys->Names.FindItem( N, *(INT*)&iKey );
	return 0;
	unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

