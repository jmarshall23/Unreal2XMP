/*-----------------------------------------------------------------------------
	ActionEvent interfaces.
-----------------------------------------------------------------------------*/

#include "SmallObj.h"

class UI_API ActionEvent : public Loki::SmallObject
{
public:	// should be private, but needed for serialization.
	ActionEvent(){}
public:
	ActionEvent( DWORD InType ): NextAction(NULL), Type(InType){}
	virtual ~ActionEvent() { if( NextAction ) delete NextAction; }
	virtual void OnEvent( void* Parms=NULL ){ if(NextAction) NextAction->OnEvent(Parms); }	// Fix ARL: Reusing Parms is probably not a good idea since any return info will get lost.
	virtual void Reset(){}	// Override to handle level transition resetting of cached timestamps, etc.
	virtual void Tick( float DeltaTime ){}
	virtual UBOOL NeedsUpdate(){ return true; }	// Override to implement better caching techniques.
	virtual UBOOL operator==( const ActionEvent& Other ) const
		{ return this==&Other; }
	virtual BYTE GetID() const { return 0; }
	UBOOL IsLessThan( const ActionEvent* A ) const { return Type > A->Type; }	// Sorts from smallest type to largest type.
	friend FArchive& operator<<( FArchive &Ar, ActionEvent* &A );
	virtual void Serialize( FArchive &Ar )
	{
		Ar << Type << NextAction;
	}
public:
	ActionEvent* NextAction;
	DWORD Type;	// AccessorMask: see UComponent.h
};

class UI_API CachedEvent : public ActionEvent
{
public:	// should be private, but needed for serialization.
	CachedEvent(){}
public:
	CachedEvent( DWORD Type ): ActionEvent(Type){}
	UBOOL NeedsUpdate();
private:
	FLOAT LastFrameIndex;
};

class UI_API EventSender : public ActionEvent
{
public:	// should be private, but needed for serialization.
	EventSender(){}
public:
	EventSender( DWORD Type, class UComponent* InTarget, uiEvent InEvent ): ActionEvent(Type), Target(InTarget), Event(InEvent){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 1; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(EventSender*)&Other; else return false; }
	UBOOL operator==( const EventSender& Other ) const
		{ return Target==Other.Target && Event==Other.Event; }
	virtual void Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	class UComponent* Target;
	uiEvent Event;
};

// NOTE[aleiby]: All this extra mess is simply so we can store the callback functions as c-style void* pointers.
// This allows us to store pointers to member functions without keeping type information around via templates.
// C++ makes you jump through a few hoops to do this since you can't just directly cast the function pointer.

class NativeFunctionCaller;

#define UI_NATIVE_CALLBACK(type,target,clas,func) \
	NativeCallbackCreator<clas>::Create(ACCESSOR_##type,target,&clas::func)

template< class T > struct NativeCallbackCreator
{
	typedef void(UI_CALLBACK T::*FuncPtr)(void*);
	static NativeFunctionCaller* Create( DWORD Type, UComponent* Target, FuncPtr Func )
		{ return new NativeFunctionCaller(Type,Target,*(void**)&Func); }
};

class NativeFunctionCaller : public ActionEvent
{
	typedef void(UI_CALLBACK *FuncPtr)(UComponent*,void*);
public:	// should be private, but needed for serialization.
	NativeFunctionCaller(){}
public:
	NativeFunctionCaller( DWORD Type, UComponent* InTarget, void* InFunction )
		: ActionEvent(Type), Target(InTarget), Function(*(FuncPtr*)&InFunction){}
	void OnEvent( void* Parms=NULL ){ ActionEvent::OnEvent(Parms); if(Target&&Function) Function(Target,Parms); }
	BYTE GetID() const { return 2; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(NativeFunctionCaller*)&Other; else return false; }
	UBOOL operator==( const NativeFunctionCaller& Other ) const
		{ return Target==Other.Target && Function==Other.Function; }
	virtual void UI_API Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	UComponent* Target;	// was void* but this is more convienient (at least for serialization).
	FuncPtr Function;
};

class UI_API ConsoleCommandEvent : public ActionEvent
{
public:	// should be private, but needed for serialization.
	ConsoleCommandEvent(){}
public:
	ConsoleCommandEvent( DWORD Type, FString InCommand ): ActionEvent(Type), Command(InCommand){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 3; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(ConsoleCommandEvent*)&Other; else return false; }
	UBOOL operator==( const ConsoleCommandEvent& Other ) const
		{ return Command==Other.Command; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << Command;
		ActionEvent::Serialize(Ar);
	}
private:
	FString Command;
};

class UI_API ScriptEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	ScriptEvent(){}
public:
	ScriptEvent( DWORD Type, UObject* InTarget, FString InFuncName ): CachedEvent(Type), Target(InTarget), FuncName(InFuncName){}
	void OnEvent( void* Parms=NULL ){ ActionEvent::OnEvent(Parms); FInterfaceNotifyInfo(Target,FuncName).SendNotification(Parms); }	// Fix ARL: Cache FInterfaceNotifyInfo for speed?  -- This may break calling functions overridden in states.
	BYTE GetID() const { return 4; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(ScriptEvent*)&Other; else return false; }
	UBOOL operator==( const ScriptEvent& Other ) const
		{ return Target==Other.Target && FuncName==Other.FuncName; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << Target << FuncName;
		ActionEvent::Serialize(Ar);
	}
private:
	UObject* Target;
	FString FuncName;
};

class UI_API PlayerEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	PlayerEvent(){}
public:
	PlayerEvent( DWORD Type, FString InFuncName ): CachedEvent(Type), FuncName(InFuncName){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 5; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(PlayerEvent*)&Other; else return false; }
	UBOOL operator==( const PlayerEvent& Other ) const
		{ return FuncName==Other.FuncName; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << FuncName;
		ActionEvent::Serialize(Ar);
	}
protected:
	FString FuncName;
};

class UI_API PlayerPawnEvent : public PlayerEvent
{
public:	// should be private, but needed for serialization.
	PlayerPawnEvent(){}
public:
	PlayerPawnEvent( DWORD Type, FString InFuncName ): PlayerEvent(Type,InFuncName){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 6; }
};

class UI_API WeaponEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	WeaponEvent(){}
public:
	WeaponEvent( DWORD Type, FString InFuncName ): CachedEvent(Type), FuncName(InFuncName){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 7; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(WeaponEvent*)&Other; else return false; }
	UBOOL operator==( const WeaponEvent& Other ) const
		{ return FuncName==Other.FuncName; }
	virtual void Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	FString FuncName;
};

class UI_API PlayerIDEvent : public ActionEvent
{
public:	// should be private, but needed for serialization.
	PlayerIDEvent(){}
public:
	PlayerIDEvent( DWORD Type, FString InFuncName, UComponent* InOwner ): ActionEvent(Type), FuncName(InFuncName), Owner(InOwner) {}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 8; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(PlayerIDEvent*)&Other; else return false; }
	UBOOL operator==( const PlayerIDEvent& Other ) const
		{ return FuncName==Other.FuncName && Owner==Other.Owner; }
	virtual void Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	FString FuncName;
	UComponent* Owner;
};

class UI_API KeyframeEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	KeyframeEvent(){}
public:
	KeyframeEvent( DWORD Type ): CachedEvent(Type), Keyframes(), iCurrent(0), Delta(0.f), KeyframeDeltaSum(-1.f) {}
	void AddFrame( FLOAT Value, FLOAT Delta ){ new(Keyframes)Keyframe(Value,Delta); }
	void OnEvent( void* Parms=NULL );
	void Reset(){ iCurrent=0; Delta=0.f; }
	BYTE GetID() const { return 9; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(KeyframeEvent*)&Other; else return false; }
	UBOOL operator==( const KeyframeEvent& Other ) const
		{ return Keyframes==Other.Keyframes; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << Keyframes;
		iCurrent = 0;
		Delta = 0.0f;
		KeyframeDeltaSum = -1.0f;
		ActionEvent::Serialize(Ar);
	}
private:
	struct Keyframe
	{
		FLOAT Value;
		FLOAT Delta;
		Keyframe(): Value(0), Delta(0){}
		Keyframe(FLOAT InV, FLOAT InD): Value(InV), Delta(InD){}
		UBOOL operator==( const Keyframe& Other ) const
			{ return Value==Other.Value && Delta==Other.Delta; }
		UBOOL operator!=( const Keyframe& Other ) const
			{ return !((*this)==Other); }
		friend FArchive& operator<<( FArchive& Ar, Keyframe& A )
			{ return Ar << A.Value << A.Delta; }
	};
	TArray<Keyframe> Keyframes;
	INT iCurrent;
	FLOAT Delta, KeyframeDeltaSum;
};

#include <string.h>
class UI_API ExpressionEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	ExpressionEvent(){}
public:
	ExpressionEvent( DWORD Type, FString InExpression, UComponent* InOwner );
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 10; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(ExpressionEvent*)&Other; else return false; }
	UBOOL operator==( const ExpressionEvent& Other ) const
		{ return Owner==Other.Owner && strcmp(Expression,Other.Expression)==0; }
	virtual void Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	UComponent* Owner;
	FEAST::IPrsNode* root;
	char Expression[1024];	//!!
	FLOAT LastFrameIndex;
};

class UI_API GameFlagEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	GameFlagEvent(){}
public:
	GameFlagEvent( DWORD Type, FString _FlagName, UBOOL _Default=1 )
		: CachedEvent(Type), FlagName(_FlagName), Default(_Default), Game(NULL){}
	void OnEvent( void* Parms=NULL );
	void Reset(){ Game=NULL; }
	BYTE GetID() const { return 11; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(GameFlagEvent*)&Other; else return false; }
	UBOOL operator==( const GameFlagEvent& Other ) const
		{ return FlagName==Other.FlagName && Default==Other.Default; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << FlagName << Default;
		Game = NULL;
		// The rest will get set automatically.
		ActionEvent::Serialize(Ar);
	}
private:
	FString FlagName;
	UBOOL Default;
	AGameInfo* Game;
	INT FlagOffset;
	BITFIELD FlagMask;
};

class UI_API GRIFlagEvent : public CachedEvent
{
public:	// should be private, but needed for serialization.
	GRIFlagEvent(){}
public:
	GRIFlagEvent( DWORD Type, FString _FlagName, UBOOL _Default=1 )
		: CachedEvent(Type), FlagName(_FlagName), Default(_Default), GRI(NULL){}
	void OnEvent( void* Parms=NULL );
	void Reset(){ GRI=NULL; }
	BYTE GetID() const { return 12; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(GRIFlagEvent*)&Other; else return false; }
	UBOOL operator==( const GRIFlagEvent& Other ) const
		{ return FlagName==Other.FlagName && Default==Other.Default; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << FlagName << Default;
		GRI = NULL;
		// The rest will get set automatically.
		ActionEvent::Serialize(Ar);
	}
private:
	FString FlagName;
	UBOOL Default;
	AGameReplicationInfo* GRI;
	INT FlagOffset;
	BITFIELD FlagMask;
};

class UI_API PlayerCommandEvent : public ActionEvent
{
public:	// should be private, but needed for serialization.
	PlayerCommandEvent(){}
public:
	PlayerCommandEvent( DWORD Type, FString InCommand ): ActionEvent(Type), Command(InCommand){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 13; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(PlayerCommandEvent*)&Other; else return false; }
	UBOOL operator==( const PlayerCommandEvent& Other ) const
		{ return Command==Other.Command; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << Command;
		ActionEvent::Serialize(Ar);
	}
private:
	FString Command;
};

class UI_API EventDisabler : public ActionEvent
{
public:	// should be private, but needed for serialization.
	EventDisabler(){}
public:
	EventDisabler( DWORD Type, class UComponent* InTarget, uiEvent InEvent ): ActionEvent(Type), Target(InTarget), Event(InEvent){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 14; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(EventDisabler*)&Other; else return false; }
	UBOOL operator==( const EventDisabler& Other ) const
		{ return Target==Other.Target && Event==Other.Event; }
	virtual void Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	class UComponent* Target;
	uiEvent Event;
};

class UI_API EventEnabler : public ActionEvent
{
public:	// should be private, but needed for serialization.
	EventEnabler(){}
public:
	EventEnabler( DWORD Type, class UComponent* InTarget, uiEvent InEvent ): ActionEvent(Type), Target(InTarget), Event(InEvent){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 15; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(EventEnabler*)&Other; else return false; }
	UBOOL operator==( const EventEnabler& Other ) const
		{ return Target==Other.Target && Event==Other.Event; }
	virtual void Serialize( FArchive &Ar );
public:	// should be private, but exposed for serialization.
	class UComponent* Target;
	uiEvent Event;
};

class UI_API TriggerEvent : public ActionEvent
{
public:	// should be private, but needed for serialization.
	TriggerEvent(){}
public:
	TriggerEvent( DWORD Type, FName InEvent ): ActionEvent(Type), Event(InEvent){}
	void OnEvent( void* Parms=NULL );
	BYTE GetID() const { return 16; }
	UBOOL operator==( const ActionEvent& Other ) const
		{ if(GetID()==Other.GetID()) return *this==*(TriggerEvent*)&Other; else return false; }
	UBOOL operator==( const TriggerEvent& Other ) const
		{ return Event==Other.Event; }
	virtual void Serialize( FArchive &Ar )
	{
		Ar << Event;
		ActionEvent::Serialize(Ar);
	}
private:
	FName Event;
};


/*-----------------------------------------------------------------------------
	ComponentNotifyInfo.
-----------------------------------------------------------------------------*/

class UI_API ComponentNotifyInfo : public Loki::SmallObject
{
public:
	ComponentNotifyInfo(){}
	ComponentNotifyInfo( UComponent* _C, ActionEvent* _A ): C(_C), A(_A){}
	ComponentNotifyInfo operator=( INT Value )	// for initialization purposes in auto-generated headers (as native UnrealScript function parameters).
		{ C=NULL; A=NULL; return *this; }
	UBOOL operator==( const ComponentNotifyInfo& Other ) const
		{ return C==Other.C && ((A && Other.A) ? *A==*Other.A : A==Other.A); }
	UBOOL IsLessThan( const ComponentNotifyInfo* Other ) const;
	inline UBOOL IsValid() const
		{ return C!=NULL && A!=NULL; }
	operator UBOOL() const
		{ return IsValid(); }
	void SendNotification( void* Parms=NULL )
		{ if(IsValid()) A->OnEvent(Parms); }
	UComponent* GetTarget() const
		{ return C; }
	ActionEvent* GetActionEvent() const
		{ return A; }
	virtual void Serialize( FArchive& Ar );
	friend FArchive& operator<<( FArchive& Ar, ComponentNotifyInfo* &C )
	{
		Ar << *(DWORD*)&C;
		if( C )
		{
			if( Ar.IsLoading() )
				C = new ComponentNotifyInfo();
			C->Serialize(Ar);
		}
		return Ar;
	}
public:	// should be private, but exposed for serialization.
	UComponent* C;
	ActionEvent* A;
};



