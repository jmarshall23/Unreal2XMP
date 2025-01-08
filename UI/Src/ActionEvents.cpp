
#include "UI.h"
#include "ImportUI.h"

// ActionEvent serialization support.
FArchive& operator<<( FArchive &Ar, ActionEvent* &A )
{
	Ar << *(DWORD*)&A;
	if( A )
	{
		INT Tag;
		if( Ar.IsSaving() )
			Tag = A->GetID();
		Ar << Tag;

		if( Ar.IsLoading() )
		{
			// Messy: must be updated every time you create a new ActionEvent, but I don't feel like making meta-classes for these buggers.
			switch(Tag)
			{
			case  0: A = new ActionEvent				(); break;
			case  1: A = new EventSender				(); break;
			case  2: A = new NativeFunctionCaller		(); break;
			case  3: A = new ConsoleCommandEvent		(); break;
			case  4: A = new ScriptEvent				(); break;
			case  5: A = new PlayerEvent				(); break;
			case  6: A = new PlayerPawnEvent			(); break;
			case  7: A = new WeaponEvent				(); break;
			case  8: A = new PlayerIDEvent				(); break;
			case  9: A = new KeyframeEvent				(); break;
			case 10: A = new ExpressionEvent			(); break;
			case 11: A = new GameFlagEvent				(); break;
			case 12: A = new GRIFlagEvent				(); break;
			case 13: A = new PlayerCommandEvent			(); break;
			case 14: A = new EventDisabler				(); break;
			case 15: A = new EventEnabler				(); break;
			case 16: A = new TriggerEvent				(); break;
			default: appErrorf(TEXT("ActionEvent serialization: Invalid Tag! '%d'"),Tag);
			}
		}
		A->Serialize(Ar);
	}
	return Ar;
}

// Additional CachedEvent implementation.
UBOOL CachedEvent::NeedsUpdate()
{
	FLOAT FrameIndex = GetGConsole()->FrameIndex;
	if( FrameIndex != LastFrameIndex )
	{
		LastFrameIndex = FrameIndex;
		return true;
	}
	return false;
}

// Additional EventSender implementation.
void EventSender::OnEvent( void* Parms )
{
	guard(EventSender::OnEvent);
	NOTE(debugf(TEXT("EventSender::OnEvent -- Target='%s' Event='%s'"),*GetFullNameSafe(Target),*LookupEventName(Event)));
	ActionEvent::OnEvent(Parms);
	if( Target && Event!=EVENT_None )
		Target->SendEvent( Event );
	unguard;
}

void EventSender::Serialize( FArchive &Ar )
{
	guard(EventSender::Serialize);
	Ar << Event;
	SERIALIZE_PATHED_COMPONENT(EventSender,Target);
	ActionEvent::Serialize(Ar);
	unguard;
}

// Additional EventDisabler implementation.
void EventDisabler::OnEvent( void* Parms )
{
	guard(EventDisabler::OnEvent);
	NOTE(debugf(TEXT("EventDisabler::OnEvent -- Target='%s' Event='%s'"),*GetFullNameSafe(Target),*LookupEventName(Event)));
	ActionEvent::OnEvent(Parms);
	if( Target && Event!=EVENT_None )
		Target->DisableEvent( Event );
	unguard;
}

void EventDisabler::Serialize( FArchive &Ar )
{
	guard(EventDisabler::Serialize);
	Ar << Event;
	SERIALIZE_PATHED_COMPONENT(EventDisabler,Target);
	ActionEvent::Serialize(Ar);
	unguard;
}

// Additional EventEnabler implementation.
void EventEnabler::OnEvent( void* Parms )
{
	guard(EventEnabler::OnEvent);
	NOTE(debugf(TEXT("EventEnabler::OnEvent -- Target='%s' Event='%s'"),*GetFullNameSafe(Target),*LookupEventName(Event)));
	ActionEvent::OnEvent(Parms);
	if( Target && Event!=EVENT_None )
		Target->EnableEvent( Event );
	unguard;
}

void EventEnabler::Serialize( FArchive &Ar )
{
	guard(EventEnabler::Serialize);
	Ar << Event;
	SERIALIZE_PATHED_COMPONENT(EventEnabler,Target);
	ActionEvent::Serialize(Ar);
	unguard;
}

// Additional NativeFunctionCaller implementation.
void NativeFunctionCaller::Serialize( FArchive &Ar )
{
	guard(NativeFunctionCaller::Serialize);
	//ARL Function
	SERIALIZE_PATHED_COMPONENT(NativeFunctionCaller,Target);
	ActionEvent::Serialize(Ar);
	unguard;
}

// Additional ConsoleCommandEvent implementation.
void ConsoleCommandEvent::OnEvent( void* Parms )
{
	guard(ConsoleCommandEvent::OnEvent);
	NOTE(debugf(TEXT("ConsoleCommandEvent::OnEvent: %s"),*Command));
	ActionEvent::OnEvent(Parms);
	if( Parms )
	{
		FString FullCommand=Command;
		FString Left=TEXT("");
		FString Right=TEXT("");
		if( FullCommand.Split(TEXT("%s"),&Left,&Right) )
			FullCommand = Left + *(FString*)Parms + Right;
		GetGConsole()->ConsoleCommand( *FullCommand );
	}
	else
	{
		GetGConsole()->ConsoleCommand( *Command );
	}
	unguard;
}

// Additional PlayerEvent implementation.
void PlayerEvent::OnEvent( void* Parms )
{
	guard(PlayerEvent::OnEvent);
	NOTE(debugf(TEXT("PlayerEvent::OnEvent -- FuncName='%s'"),*FuncName));
	ActionEvent::OnEvent(Parms);
	FInterfaceNotifyInfo(GetGConsole()->Viewport->Actor,FuncName).SendNotification(Parms);
	unguard;
}

// Additional PlayerPawnEvent implementation.
void PlayerPawnEvent::OnEvent( void* Parms )
{
	guard(PlayerPawnEvent::OnEvent);
	NOTE(debugf(TEXT("PlayerPawnEvent::OnEvent -- FuncName='%s'"),*FuncName));
	ActionEvent::OnEvent(Parms);
	if( GetGConsole() && GetGConsole()->Viewport->Actor->Pawn )
		FInterfaceNotifyInfo(GetGConsole()->Viewport->Actor->Pawn,FuncName).SendNotification(Parms);
	unguard;
}

// Additional WeaponEvent implementation.
void WeaponEvent::OnEvent( void* Parms )
{
	guard(WeaponEvent::OnEvent);
	NOTE(debugf(TEXT("WeaponEvent::OnEvent -- FuncName='%s'"),*FuncName));
	ActionEvent::OnEvent(Parms);
	if (!GetGConsole()) return;
	APawn* Pawn = GetGConsole()->Viewport->Actor->Pawn;
	if (!Pawn) return;
	AWeapon* Weapon = Pawn->Weapon;
	if (!Weapon) return;
	FInterfaceNotifyInfo(Weapon,FuncName).SendNotification(Parms);
	unguard;
}

void WeaponEvent::Serialize( FArchive &Ar )
{
	guard(WeaponEvent::Serialize);
	Ar << FuncName;
	ActionEvent::Serialize(Ar);
	unguard;
}

// Additional PlayerIDEvent implementation.
void PlayerIDEvent::OnEvent( void* Parms )
{
	guard(PlayerIDEvent::OnEvent);
	NOTE(debugf(TEXT("PlayerIDEvent::OnEvent -- FuncName='%s'"),*FuncName));
	ActionEvent::OnEvent(Parms);
	if (!Owner) return;
	UPlayerID* Parent = Owner->GetParentType<UPlayerID>();
	if (!Parent) return;
	APawn* Pawn = Parent->CurrentPawn;
	if (!Pawn) return;
	FInterfaceNotifyInfo(Pawn,FuncName).SendNotification(Parms);
	unguard;
}

void PlayerIDEvent::Serialize( FArchive &Ar )
{
	guard(PlayerIDEvent::Serialize);
	Ar << FuncName;
	SERIALIZE_PATHED_COMPONENT(PlayerIDEvent,Owner);
	ActionEvent::Serialize(Ar);
	unguard;
}

// Additional KeyframeEvent implementation.
void KeyframeEvent::OnEvent( void* Parms )
{
	if(!Parms || Keyframes.Num()==0) return;

	// Fix ARL: Optimize!
	if( KeyframeDeltaSum < 0.f )
		for( INT i=0; i<Keyframes.Num(); i++ )
			KeyframeDeltaSum += Keyframes(i).Delta;
	if( KeyframeDeltaSum < 0.f )
		return;
	// Note: GRealDeltaTime has been seen as high as 19 seconds just after level transitions
	Delta += appFmod( GRealDeltaTime, KeyframeDeltaSum );	//can use GGameDeltaTime to test with slomo.
	
	#define NEXT_KEYFRAME ((iCurrent+1)%Keyframes.Num())
	#define PREV_KEYFRAME ((iCurrent+Keyframes.Num()-1)%Keyframes.Num())
	while(Delta>Keyframes(iCurrent).Delta)
		{ Delta-=Keyframes(iCurrent).Delta; iCurrent=NEXT_KEYFRAME; }
	*(FLOAT*)Parms=appBlend(Keyframes(PREV_KEYFRAME).Value,Keyframes(iCurrent).Value,((FLOAT)Delta)/Keyframes(iCurrent).Delta);
}

// ExpressionEvent implementation.
#include "..\..\feast\inc\FeastExpressions.h"
#include "..\..\feast\inc\StandardFeastNodes.h"
#include "UIFeastNodes.h"
ExpressionEvent::ExpressionEvent( DWORD Type, FString InExpression, UComponent* InOwner )
: CachedEvent(Type), Owner(InOwner)
{
	strcpy(Expression,appToAnsi(*InExpression));
	root = ParseExpression(Expression);
}
void ExpressionEvent::OnEvent( void* Parms )
{
	DataType Result = *(FLOAT*)Parms;
	if(Parms) EvaluateExpression( root, Result, Owner );
	Result.AsFloat( *(FLOAT*)Parms );
}
void ExpressionEvent::Serialize( FArchive &Ar )
{
	SERIALIZE_PATHED_COMPONENT(ExpressionEvent,Owner);

	INT Length;
	if( Ar.IsSaving() )
		Length = strlen( Expression );
	Ar << Length;

	Ar.Serialize( Expression, Length );

	if( Ar.IsLoading() )
		root = ParseExpression(Expression);

	ActionEvent::Serialize(Ar);
}

// Additional GameFlagEvent implementation.
void GameFlagEvent::OnEvent( void* Parms )
{
	if(!Game)
	{
		Game = GetGConsole()->GetLevel()->Game;
		if(Game)
		{
			UBoolProperty* Property = Cast<UBoolProperty>(FindField<UProperty>( Game->GetClass(), *FlagName ));
			if(Property)
			{
				FlagOffset = Property->Offset;
				FlagMask = Property->BitMask;
				NOTE(debugf(TEXT("GameFlagEvent: Property=%s Game=%s FlagOffset=%d FlagMask=%d"),Property->GetName(),Game->GetName(),FlagOffset,FlagMask));
			}
			else
			{
				//debugf(NAME_Warning,TEXT("GameFlagEvent: BoolProperty %s not found in %s!"),*FlagName,Game->GetName());
				FlagOffset = -1;
			}
		}
		else
		{
			//debugf(NAME_Warning,TEXT("GameFlagEvent: GameInfo not found!!"));
			FlagOffset = -1;
		}
	}

	if(FlagOffset>=0)
		*(UBOOL*)Parms = *(BITFIELD*)((BYTE*)Game + FlagOffset) & FlagMask;
	else
		*(UBOOL*)Parms = Default;

	NOTE(debugf(TEXT("GameFlagEvent: %s"),*(UBOOL*)Parms?TEXT("True"):TEXT("False")));
}

// Additional GRIFlagEvent implementation.
void GRIFlagEvent::OnEvent( void* Parms )
{
	if(!GRI)
	{
		GRI = GetGConsole()->Viewport->Actor->GameReplicationInfo;
		if(GRI)
		{
			UBoolProperty* Property = Cast<UBoolProperty>(FindField<UProperty>( GRI->GetClass(), *FlagName ));
			if(Property)
			{
				FlagOffset = Property->Offset;
				FlagMask = Property->BitMask;
				NOTE(debugf(TEXT("GRIFlagEvent: Property=%s GRI=%s FlagOffset=%d FlagMask=%d"),Property->GetName(),GRI->GetName(),FlagOffset,FlagMask));
			}
			else
			{
				//debugf(NAME_Warning,TEXT("GRIFlagEvent: BoolProperty %s not found in %s!"),*FlagName,GRI->GetName());
				FlagOffset = -1;
			}
		}
		else
		{
			//debugf(NAME_Warning,TEXT("GRIFlagEvent: GameReplicationInfo not found!!"));
			FlagOffset = -1;
		}
	}

	if(FlagOffset>=0)
		*(UBOOL*)Parms = *(BITFIELD*)((BYTE*)GRI + FlagOffset) & FlagMask;
	else
		*(UBOOL*)Parms = Default;

	NOTE(debugf(TEXT("GRIFlagEvent: %s"),*(UBOOL*)Parms?TEXT("True"):TEXT("False")));
}

// Additional ConsoleCommandEvent implementation.
void PlayerCommandEvent::OnEvent( void* Parms )
{
	guard(PlayerCommandEvent::OnEvent);
	NOTE(debugf(TEXT("PlayerCommandEvent::OnEvent: %s"),*Command));
	ActionEvent::OnEvent(Parms);
	UPlayer* Player = GetGConsole()->Viewport->Actor->Player;
	if (!Player) return;
	FStringOutputDevice StrOut;
//	FString FullCommand = Parms ? FString::Printf(TEXT("%s %s"),*Command,**(FString*)Parms) : Command;
//	Player->Exec( *FullCommand, StrOut );
	Player->Exec( *Command, StrOut );
	if (Parms) *(FString*)Parms = *StrOut;
	unguard;
}

// Additional TriggerEvent implementation.
void TriggerEvent::OnEvent( void* Parms )
{
	guard(TriggerEvent::OnEvent);
	NOTE(debugf(TEXT("TriggerEvent::OnEvent: %s"),*Name));
	ActionEvent::OnEvent(Parms);
	APlayerController* PlayerOwner = GetGConsole()->Viewport->Actor;
	if (PlayerOwner)
		PlayerOwner->eventTriggerEvent(Event, PlayerOwner, PlayerOwner->Pawn);
	unguard;
}

// Additional ComponentNotifyInfo implementation.
UBOOL ComponentNotifyInfo::IsLessThan( const ComponentNotifyInfo* Other ) const
{
	return IsValid() && Other->IsValid() ? GetTarget()->GetAbsDrawOrder() < Other->GetTarget()->GetAbsDrawOrder() : false;
}

void ComponentNotifyInfo::Serialize( FArchive& Ar )
{
	guard(ComponentNotifyInfo::Serialize);
	SERIALIZE_PATHED_COMPONENT(ComponentNotifyInfo,C);
	Ar << A;
	unguard;
}


