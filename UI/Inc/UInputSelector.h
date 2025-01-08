
///////////////
// Overrides //
///////////////

void Update(UPDATE_DEF);
void Import( SectionData Section );
void HandleEvent( uiEvent Event );


///////////////////
// Mouse support //
///////////////////

virtual void SetMouse( UMouseCursor* M );


/////////////////////
// Listening state //
/////////////////////

public:		void GotoState_Listening()	{ bListening=true;	}
public:		void GotoState_None()		{ bListening=false;	}

protected:
	FColor	Listening_GetColor();
	void	Listening_Click(){}
	void	Listening_StartListening(){}
	void	Listening_StopListening();
	void	Listening_ProcessKey( INT Key, FString KeyName );
	UBOOL	Listening_KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta );

	FColor	Global_GetColor(){ return Super::GetColorI(); }
	void	Global_Click();
	void	Global_StartListening();
	void	Global_StopListening(){}
	void	Global_ProcessKey( INT Key, FString KeyName ){}
	UBOOL	Global_KeyEvent( EInputKey Key, EInputAction Action, FLOAT Delta ){ return false; }

public:		FColor	GetColorI()								{ if( bListening ) return Listening_GetColor();				else return Global_GetColor();	}
private:	void	UI_CALLBACK Click(void*)				{ if( bListening ) Listening_Click();						else Global_Click();			}
private:	void	StartListening()						{ if( bListening ) Listening_StartListening();				else Global_StartListening();	}
private:	void	StopListening()							{ if( bListening ) Listening_StopListening();				else Global_StopListening();	}
protected:	void	ProcessKey( INT Key, FString KeyName )	{ if( bListening ) Listening_ProcessKey( Key, KeyName );	else Global_ProcessKey( Key, KeyName ); }
public:		void	UI_CALLBACK KeyEvent( void* Parms )
{
	KeyEventParms* P = (KeyEventParms*)Parms;

	if( bListening )
		P->bHandled = Listening_KeyEvent( P->Key, P->Action, P->Delta );
	else
		P->bHandled = Global_KeyEvent( P->Key, P->Action, P->Delta );
}

