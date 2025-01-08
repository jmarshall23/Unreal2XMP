#include "U2.h"

////////////////
// Interfaces //
////////////////

// implements auto-hide feature
//------------------------------------------------------------------------------
void UWeaponPanel::Tick( float DeltaTime )
{
	guard(UWeaponPanel::Tick);
	NOTE(debugf(TEXT("(%s)UWeaponPanel::Tick( %f )"), GetFullName(), DeltaTime ));

	TimeShown += DeltaTime;

	if( bShown && TimeShown > TimeShownMax )
		eventHide();

	unguard;
}


IMPLEMENT_CLASS(UWeaponPanel);

