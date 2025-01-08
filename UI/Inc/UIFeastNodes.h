//=============================================================================
// UIFeastNodes.h
//=============================================================================

static UComponent* GetComponent( FString Name, UComponent* Helper )
{
	Name=Name.Caps();
	if( Name==TEXT("PARENT")	) return Helper->GetParent();
	if( Name==TEXT("ROOT")		) return Helper->GetRoot();
	if( Name==TEXT("STATE")		) return DynamicCastChecked<UMultiStateComponent>(Helper)->GetState();
//	if( Name==TEXT("CONSOLE")	) return Helper->GetConsole();
//	if( Name==TEXT("PLAYER")	) return Helper->GetPlayerOwner();

	return Helper->FindLooseComponent(uiName(Name));
};



#pragma warning (push)
#pragma warning (disable : 4706) // assignment within conditional expression
#pragma warning (disable : 4800) // forcing value to bool 'true' or 'false' (performance warning)

template<> struct FeastNode<UNIQUE_ID>
{
	DEF_ID			(28);
	DEF_TYPE		(NT_NonTerminal);
	DEF_NAME		("Field");
	DEF_RULE		("postfix_expr");
	DEF_PRODUCTION	("postfix_expr:1 '.' IDENTIFIER:2");

	EVALUATE_NT_START(2)
		UComponent* C;
		FString objName, propName;

		if( Operands[0].AsString(objName) )
		{
			C=GetComponent( objName, (UComponent*)UserData );
			if(!C){ debugf(NAME_Warning, TEXT("EvaluateField: Invalid component name '%s'"), *objName);	return false; }
		}
		else
		{
			if( !Operands[0].AsComponent(C)	){ debugf(NAME_Warning, TEXT("EvaluateField: Invalid component"));	return false; }
		}
		if( !Operands[1].AsString(propName)	){ debugf(NAME_Warning, TEXT("EvaluateField: Invalid property name"));	return false; }

		NOTE(debugf(TEXT("EvaluateField: Object: '%s'  Property: '%s'"), *C->GetName(), *propName ));

		propName=propName.Caps();
		if( propName==TEXT("WIDTH")				){ Result = C->GetWidth();										return true; }
		if( propName==TEXT("HEIGHT")			){ Result = C->GetHeight();										return true; }
		if( propName==TEXT("X")					){ Result = C->GetX();											return true; }
		if( propName==TEXT("Y")					){ Result = C->GetY();											return true; }
		if( propName==TEXT("ScreenX")			){ Result = C->GetScreenCoords().X;								return true; }
		if( propName==TEXT("ScreenY")			){ Result = C->GetScreenCoords().Y;								return true; }
		if( propName==TEXT("ALPHA")				){ Result = C->GetAlpha();										return true; }
		if( propName==TEXT("R")					){ Result = C->GetR();											return true; }
		if( propName==TEXT("G")					){ Result = C->GetG();											return true; }
		if( propName==TEXT("B")					){ Result = C->GetB();											return true; }
		if( propName==TEXT("A")					){ Result = C->GetAlpha();										return true; }
		if( propName==TEXT("SliderValue")		){ Result = DynamicCastChecked<USlider>(C)->GetValue();			return true; }
		if( propName==TEXT("SliderNormValue")	){ Result = DynamicCastChecked<USlider>(C)->Value;				return true; }
		if( propName==TEXT("SliderIntValue")	){ Result = DynamicCastChecked<USlider>(C)->GetIntValue();		return true; }
		if( propName==TEXT("SliderStringValue")	){ Result = DynamicCastChecked<USlider>(C)->GetStringValue();	return true; }
		if( C=GetComponent(propName,C)			){ Result = C;													return true; }	// yes assignment not compare.

		debugf(NAME_Warning, TEXT("EvaluateField: Property not found '%s'"),propName);
	EVALUATE_NT_END
};

#pragma warning (pop)

