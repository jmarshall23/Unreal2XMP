
#define MOUSECURSOR_PARTICLESYSTEMS 0


//////////////////////
// Native variables //
//////////////////////

friend FArchive& operator<<( FArchive& Ar, EMouseAction &A )
{
	return Ar << *(BYTE*)A;
}


TMap<EMouseAction,NotifyList>* MouseActionListenerTable;
TArray<UComponent*>* LastRelevantComponents;

static FPoint MousePos;


///////////////////////
// UObject overrides //
///////////////////////

void Constructed();
void Destroy();
void Serialize( FArchive &Ar );


////////////////
// Interfaces //
////////////////

void Tick( FLOAT DeltaTime );
void AddMouseActionListener( EMouseAction Action, UComponent* C, ActionEvent* NotifyObj );
void RemoveMouseActionListener( EMouseAction Action, UComponent* C, ActionEvent* NotifyObj );
void RemoveAllMouseActionListeners( EMouseAction Action, UComponent* C );

void DispatchMousePress();
void DispatchMouseRelease();

INT GetMouseIndex();


/////////////
// Helpers //
/////////////

void FindRelevantListeners();


///////////////
// Interface //
///////////////

virtual void SetConsole( UUIConsole* C );


/////////////////////////
// Component overrides //
/////////////////////////

UComponent* GetComponentAtI( const FPoint& P );
FPoint GetScreenCoordsI();
void Update(UPDATE_DEF);
void Import( SectionData Section );

UBOOL ContainsI( const FPoint& P ){ return false; }
UBOOL ContainsAbsI( const FPoint& P ){ return false; }


/////////////////
// MouseEvents //
/////////////////

void UI_CALLBACK MouseMove_Event( void* Parms );
void UI_CALLBACK MousePress_Event( void* Parms );
void UI_CALLBACK MouseRelease_Event( void* Parms );

virtual void MouseAction();


#if MOUSECURSOR_PARTICLESYSTEMS

////////////////////////////
// ParticleSystem support //
////////////////////////////

void SetEnabledI( UBOOL bEnabled );
FVector CalcMouseTrailLocation();

#endif


