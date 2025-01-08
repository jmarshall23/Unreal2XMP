//=============================================================================
// $Author: Mbaldwin $
// $Date: 10/29/02 11:41p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	WeaponPanel.uc
// Author:	Mike Baldwin
// Date:	25 June 2001
//------------------------------------------------------------------------------
// Description:	Used to store various needed UnrealScript functions for the UI.
//------------------------------------------------------------------------------
// Notes:
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------


class WeaponPanel extends UIHelper
	native;

const	ShowPanelEvent		= "ShowWeaponPanel";
const	HidePanelEvent		= "HideWeaponPanel";
const	ShowTrayEvent		= "ShowWeaponTray";
const	HideTrayEvent		= "HideWeaponTray";
const	AddAllEvent			= "AddAllWeapons";
const	RemoveAllEvent		= "RemoveAllWeapons";
const	HiliteAllEvent		= "HiliteAllWeapons";
const	UnHiliteAllEvent	= "UnHiliteAllWeapons";

const	AddPrefix			= "AddWeapon";
const	RemovePrefix		= "RemoveWeapon";
const	HilitePrefix		= "HiliteWeapon";
const	UnHilitePrefix		= "UnHiliteWeapon";
const	Sep					= "_";

const	UnknownID			= "??";

var() bool  bShown;					// Is the weapon panel showing?
var() byte	bTrayShown[6];			// Is a particular weapon tray showing?  (Hack: Inventory groups go 1..5)
var() float TimeShownMax;			// Max time before weapon panel hides itself

var float TimeShown;		// Time that the weapon panel has been up

function U2PlayerController GetU2Player(){ return U2PlayerController(GetPlayerOwner()); }

event Constructed()
{
	SortTrays();	// in case the user screws them all up in the ini file.
}

// Show the weapons panel and whatever trays are already displayed
event Show()
{
	Sync();			 // keep panel in sync with the players inventory

	if( bShown == false )
	{
		GetPlayerOwner().SendEvent(ShowPanelEvent);
		GetUIConsole().AddTickListener( Self );
		bShown = true;
	}

	// Reset the timer
	TimeShown = 0;
}

// Hide weapons panel and all trays
event Hide()
{
	local int i;

	if( bShown )
	{
		GetPlayerOwner().SendEvent(HidePanelEvent);
		GetUIConsole().RemoveTickListener( Self );
		bShown = false;
	}

	for( i=0; i < ArrayCount(bTrayShown); i++ )
		HideTray(i);
}


function Toggle()
{
	if( bShown )
		Hide();
	else
		Show();
}


// Show one weapons tray
function ShowTray(int Which)
{
	if( bTrayShown[Which] == 0 )
	{
		GetPlayerOwner().SendEvent(ShowTrayEvent $ Which);
		bTrayShown[Which] = 1;
	}
}


// Hide one weapons tray
function HideTray(int Which)
{
	if( bTrayShown[Which] == 1 )
	{
		GetPlayerOwner().SendEvent(HideTrayEvent $ Which);
		bTrayShown[Which] = 0;
	}
}


// update the weapons panel with the player's current inventory
function Sync()
{
	local Inventory Inv;
	local Pawn P;

	P = GetPlayerOwner().Pawn;
	if( P == None ) return;

	GetPlayerOwner().SendEvent(RemoveAllEvent);

	// remove all weapons from the trays
	for( Inv = P.Inventory; Inv != None; Inv = Inv.Inventory )
		DoRemoveWeapon( Weapon(Inv), true );

	// register all weapons
	for( Inv = P.Inventory; Inv != None; Inv = Inv.Inventory )
		if( Weapon(Inv) != None )
			RegisterWeapon( Weapon(Inv) );

	// put back in weapon icons
	for( Inv = P.Inventory; Inv != None; Inv = Inv.Inventory )
		DoAddWeapon( Weapon(Inv), true, true );
}


// Highlight a weapon in the weapon panel
// Used to display NextWeapon, PrevWeapon, SwitchWeapon, and SwitchToBestWeapon
function HighlightWeapon( Weapon NewWeapon )
{
	local int i;
	local int WhichTray;
	local int WhichPosition;

	if( NewWeapon == None ) return;

	// show weapon panel
	Show();
	
	// unhightlight current weapon
	GetPlayerOwner().SendEvent(UnHiliteAllEvent);
	
	// get index of newly selected weapon
	if( GetWeaponIndex( NewWeapon.ItemName, WhichTray, WhichPosition ) )
	{
		// close all other trays
		for( i=1; i < ArrayCount(bTrayShown); i++ )
			if( i != WhichTray )
				HideTray(i);
	
		// show the tray, if necessary
		ShowTray(WhichTray);
	
		// hightlight the weapon we're switching to
		GetPlayerOwner().SendEvent(HilitePrefix $ WhichTray $ Sep $ WhichPosition);
	}
}


function DoAddWeapon(Weapon W, bool bSendUIEvents, optional bool bSyncing)
{
	local int i,j;

	if( W != None )
	{
		RegisterWeapon(W);

		// we may be adding a weapon that sits in an existing place in the ui,
		// turn on the "leftmost" element in the ui.
		if( bSendUIEvents && GetWeaponIndex(W.ItemName, i, j) )
		{
			if( bSyncing )
				// we're syncing - so no weapons are being added, so there's no weapon index re-ordering
				// so its safe to use the weapon tray/index as the event
				GetPlayerOwner().SendEvent(AddPrefix $ i $ Sep $ j);			
			else
				// weapon reordering going on and we've potentially added a weapon
				// send an event to open the last weapon in this tray
				GetPlayerOwner().SendEvent(AddPrefix $ i $ Sep $ WeaponsInTray(i));
		}
	}
}


function DoRemoveWeapon(Weapon W, bool bSendUIEvents)
{
	local int i,j;
	if( W != None )
	{
		// we may be removing a weapon who's place in the ui will be taken over be another weapon
		// remove the "leftmost" element in the ui.
		if( bSendUIEvents && GetWeaponIndex(W.ItemName, i, j) )
			GetPlayerOwner().SendEvent(RemovePrefix $ i $ Sep $ WeaponsInTray(i));
		UnRegisterWeapon(W);
	}
}

function RegisterWeapon( Weapon W )
{
	local int i,j;

	if( GetWeaponIndex( W.ItemName, i, j ) )
	{
		// use existing (user configured) settings or previously used weapon
		W.InventoryGroup = i;
		W.GroupOffset = GetWeaponPriority(i,j);
		SetWeapon(W,i,j);
	}
	else
		SetWeapon( W, W.InventoryGroup, AddWeapon(W.ItemName,W.InventoryGroup,W.GroupOffset) );
}

function UnRegisterWeapon( Weapon W )
{
	local int i,j;
	if( GetWeaponIndex( W.ItemName, i, j ) )
		SetWeapon(None,i,j);	// clear reference.
	else
		Log( "Warning: Cannot unregister unknown weapon"@W, 'DevSpam' );
}

function bool GetWeaponIndex( string WeaponName, out int i, out int j )
{
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	i=1; for( j=1; j<U2Player.WeaponOrder1.length; j++ ) if( U2Player.WeaponOrder1[j].WeaponName == WeaponName ) return true;
	i=2; for( j=1; j<U2Player.WeaponOrder2.length; j++ ) if( U2Player.WeaponOrder2[j].WeaponName == WeaponName ) return true;
	i=3; for( j=1; j<U2Player.WeaponOrder3.length; j++ ) if( U2Player.WeaponOrder3[j].WeaponName == WeaponName ) return true;
	i=4; for( j=1; j<U2Player.WeaponOrder4.length; j++ ) if( U2Player.WeaponOrder4[j].WeaponName == WeaponName ) return true;
	i=5; for( j=1; j<U2Player.WeaponOrder5.length; j++ ) if( U2Player.WeaponOrder5[j].WeaponName == WeaponName ) return true;
	return false;
}

function int AddWeapon( string WeaponName, int iTray, int Priority )
{
	local int j;
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	switch(iTray)
	{
	case 1: j=Max(U2Player.WeaponOrder1.length,1); U2Player.WeaponOrder1.length=j+1; U2Player.WeaponOrder1[j].WeaponName = WeaponName; U2Player.WeaponOrder1[j].Priority = Priority; return SortTrayPerc(1,j);
	case 2: j=Max(U2Player.WeaponOrder2.length,1); U2Player.WeaponOrder2.length=j+1; U2Player.WeaponOrder2[j].WeaponName = WeaponName; U2Player.WeaponOrder2[j].Priority = Priority; return SortTrayPerc(2,j);
	case 3: j=Max(U2Player.WeaponOrder3.length,1); U2Player.WeaponOrder3.length=j+1; U2Player.WeaponOrder3[j].WeaponName = WeaponName; U2Player.WeaponOrder3[j].Priority = Priority; return SortTrayPerc(3,j);
	case 4: j=Max(U2Player.WeaponOrder4.length,1); U2Player.WeaponOrder4.length=j+1; U2Player.WeaponOrder4[j].WeaponName = WeaponName; U2Player.WeaponOrder4[j].Priority = Priority; return SortTrayPerc(4,j);
	case 5: j=Max(U2Player.WeaponOrder5.length,1); U2Player.WeaponOrder5.length=j+1; U2Player.WeaponOrder5[j].WeaponName = WeaponName; U2Player.WeaponOrder5[j].Priority = Priority; return SortTrayPerc(5,j);
	default: warn("Invalid tray:"@iTray); return 0;
	}
}

function int WeaponsInTray( int iTray )
{
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	switch(iTray)
	{
	case 1: return Max(0, U2Player.WeaponOrder1.length-1);
	case 2: return Max(0, U2Player.WeaponOrder2.length-1);
	case 3: return Max(0, U2Player.WeaponOrder3.length-1);
	case 4: return Max(0, U2Player.WeaponOrder4.length-1);
	case 5: return Max(0, U2Player.WeaponOrder5.length-1);
	default: warn("Invalid tray:"@iTray); return 0;
	}
}

function SortTrays()
{
	SortTray(1);
	SortTray(2);
	SortTray(3);
	SortTray(4);
	SortTray(5);
}

function SortTray( int iTray )
{
	local int j;
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	if (!U2Player) return;
	switch(iTray)
	{
	case 1: for( j=1; j<U2Player.WeaponOrder1.length; j++ ) SortTrayPerc(1,j); break;
	case 2: for( j=1; j<U2Player.WeaponOrder2.length; j++ ) SortTrayPerc(2,j); break;
	case 3: for( j=1; j<U2Player.WeaponOrder3.length; j++ ) SortTrayPerc(3,j); break;
	case 4: for( j=1; j<U2Player.WeaponOrder4.length; j++ ) SortTrayPerc(4,j); break;
	case 5: for( j=1; j<U2Player.WeaponOrder5.length; j++ ) SortTrayPerc(5,j); break;
	default: warn("Invalid tray:"@iTray);  break;
	}
}

function int SortTrayPerc( int iTray, int j )
{
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	switch(iTray)
	{
	case 1: for(j=j;j>1;j--) if( ShouldExchange( U2Player.WeaponOrder1[j-1], U2Player.WeaponOrder1[j] )) ExchangeTray( 1, j-1, j ); else return j; return 1;
	case 2: for(j=j;j>1;j--) if( ShouldExchange( U2Player.WeaponOrder2[j-1], U2Player.WeaponOrder2[j] )) ExchangeTray( 2, j-1, j ); else return j; return 1;
	case 3: for(j=j;j>1;j--) if( ShouldExchange( U2Player.WeaponOrder3[j-1], U2Player.WeaponOrder3[j] )) ExchangeTray( 3, j-1, j ); else return j; return 1;
	case 4: for(j=j;j>1;j--) if( ShouldExchange( U2Player.WeaponOrder4[j-1], U2Player.WeaponOrder4[j] )) ExchangeTray( 4, j-1, j ); else return j; return 1;
	case 5: for(j=j;j>1;j--) if( ShouldExchange( U2Player.WeaponOrder5[j-1], U2Player.WeaponOrder5[j] )) ExchangeTray( 5, j-1, j ); else return j; return 1;
	default: warn("Invalid tray:"@iTray); return 0;
	}
}

function bool ShouldExchange( U2PlayerController.WeaponInfo A, U2PlayerController.WeaponInfo B )
{
	if( A.Priority < B.Priority )
		return true;
	else if( A.Priority == B.Priority )
	{
		// weapons with same tray/priority sort alphabetically
		if( A.WeaponName < B.WeaponName )
			return true;
	}
	return false;
}

function ExchangeTray( int iTray, int A, int B )
{
	local U2PlayerController.WeaponInfo Temp;
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	if(A==B) return;
	switch(iTray)
	{
	case 1: Temp=U2Player.WeaponOrder1[A]; U2Player.WeaponOrder1[A]=U2Player.WeaponOrder1[B]; U2Player.WeaponOrder1[B]=Temp; break;
	case 2: Temp=U2Player.WeaponOrder2[A]; U2Player.WeaponOrder2[A]=U2Player.WeaponOrder2[B]; U2Player.WeaponOrder2[B]=Temp; break;
	case 3: Temp=U2Player.WeaponOrder3[A]; U2Player.WeaponOrder3[A]=U2Player.WeaponOrder3[B]; U2Player.WeaponOrder3[B]=Temp; break;
	case 4: Temp=U2Player.WeaponOrder4[A]; U2Player.WeaponOrder4[A]=U2Player.WeaponOrder4[B]; U2Player.WeaponOrder4[B]=Temp; break;
	case 5: Temp=U2Player.WeaponOrder5[A]; U2Player.WeaponOrder5[A]=U2Player.WeaponOrder5[B]; U2Player.WeaponOrder5[B]=Temp; break;
	default: warn("Invalid tray:"@iTray); return;
	}
}
	
function Weapon GetWeapon( int iTray, int iElement )
{
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	switch(iTray)
	{
	case 1: if(iElement<U2Player.WeaponOrder1.length) return U2Player.WeaponOrder1[iElement].Weapon; else return None;
	case 2: if(iElement<U2Player.WeaponOrder2.length) return U2Player.WeaponOrder2[iElement].Weapon; else return None;
	case 3: if(iElement<U2Player.WeaponOrder3.length) return U2Player.WeaponOrder3[iElement].Weapon; else return None;
	case 4: if(iElement<U2Player.WeaponOrder4.length) return U2Player.WeaponOrder4[iElement].Weapon; else return None;
	case 5: if(iElement<U2Player.WeaponOrder5.length) return U2Player.WeaponOrder5[iElement].Weapon; else return None;
	default: return None;
	}
}

function SetWeapon( Weapon W, int iTray, int iElement )
{
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	if(iTray<1 || iTray>5){ warn("Cannot add"@W@".  Invalid tray:"@iTray); return; }
	if(iElement<1 || iElement>9){ warn("Cannot add"@W@".  Invalid element:"@iElement); return; }
	switch(iTray)
	{
	case 1: U2Player.WeaponOrder1[iElement].Weapon = W; break;
	case 2: U2Player.WeaponOrder2[iElement].Weapon = W; break;
	case 3: U2Player.WeaponOrder3[iElement].Weapon = W; break;
	case 4: U2Player.WeaponOrder4[iElement].Weapon = W; break;
	case 5: U2Player.WeaponOrder5[iElement].Weapon = W; break;
	}
}

function int GetWeaponPriority( int iTray, int iElement )
{
	local U2PlayerController U2Player;
	U2Player=GetU2Player();
	switch(iTray)
	{
	case 1: if(iElement<U2Player.WeaponOrder1.length) return U2Player.WeaponOrder1[iElement].Priority; else return 0;
	case 2: if(iElement<U2Player.WeaponOrder2.length) return U2Player.WeaponOrder2[iElement].Priority; else return 0;
	case 3: if(iElement<U2Player.WeaponOrder3.length) return U2Player.WeaponOrder3[iElement].Priority; else return 0;
	case 4: if(iElement<U2Player.WeaponOrder4.length) return U2Player.WeaponOrder4[iElement].Priority; else return 0;
	case 5: if(iElement<U2Player.WeaponOrder5.length) return U2Player.WeaponOrder5[iElement].Priority; else return 0;
	default: warn("Invalid tray:"@iTray); return 0;
	}
}


//
// UI Callbacks.
//

// Fix ARL: UI Script Accessors could certainly use the ability to define parameters.

//=============================================================================
function string GetWeaponAmmo(int i,int j)
{
	local int Value;
	local Weapon W;
	W=GetWeapon(i,j);
	if(W!=None && !W.bDeleteMe /*&& W.AmmoType!=None*/)
	{
//		Value=W.AmmoType.AmmoAmount;
		Value=W.GetAmmoAmount();
		if		( Value < 1    )			return "000";
		else if	( Value < 10   )			return "00"$Value;
		else if	( Value < 100  )			return "0"$Value;
		else if	( Value < 1000 )			return string(Value);
		else								return "MAX";
	}//	else if ( W!=None && !W.bDeleteMe )	return "N/A";
		else								return "ERR";
}
//-----------------------------------------------------------------------------
function interface string GetWeaponAmmo_1_1(){ return GetWeaponAmmo(1,1); }
function interface string GetWeaponAmmo_1_2(){ return GetWeaponAmmo(1,2); }
function interface string GetWeaponAmmo_1_3(){ return GetWeaponAmmo(1,3); }
function interface string GetWeaponAmmo_1_4(){ return GetWeaponAmmo(1,4); }
function interface string GetWeaponAmmo_1_5(){ return GetWeaponAmmo(1,5); }
function interface string GetWeaponAmmo_1_6(){ return GetWeaponAmmo(1,6); }
function interface string GetWeaponAmmo_1_7(){ return GetWeaponAmmo(1,7); }
function interface string GetWeaponAmmo_1_8(){ return GetWeaponAmmo(1,8); }
function interface string GetWeaponAmmo_1_9(){ return GetWeaponAmmo(1,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponAmmo_2_1(){ return GetWeaponAmmo(2,1); }
function interface string GetWeaponAmmo_2_2(){ return GetWeaponAmmo(2,2); }
function interface string GetWeaponAmmo_2_3(){ return GetWeaponAmmo(2,3); }
function interface string GetWeaponAmmo_2_4(){ return GetWeaponAmmo(2,4); }
function interface string GetWeaponAmmo_2_5(){ return GetWeaponAmmo(2,5); }
function interface string GetWeaponAmmo_2_6(){ return GetWeaponAmmo(2,6); }
function interface string GetWeaponAmmo_2_7(){ return GetWeaponAmmo(2,7); }
function interface string GetWeaponAmmo_2_8(){ return GetWeaponAmmo(2,8); }
function interface string GetWeaponAmmo_2_9(){ return GetWeaponAmmo(2,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponAmmo_3_1(){ return GetWeaponAmmo(3,1); }
function interface string GetWeaponAmmo_3_2(){ return GetWeaponAmmo(3,2); }
function interface string GetWeaponAmmo_3_3(){ return GetWeaponAmmo(3,3); }
function interface string GetWeaponAmmo_3_4(){ return GetWeaponAmmo(3,4); }
function interface string GetWeaponAmmo_3_5(){ return GetWeaponAmmo(3,5); }
function interface string GetWeaponAmmo_3_6(){ return GetWeaponAmmo(3,6); }
function interface string GetWeaponAmmo_3_7(){ return GetWeaponAmmo(3,7); }
function interface string GetWeaponAmmo_3_8(){ return GetWeaponAmmo(3,8); }
function interface string GetWeaponAmmo_3_9(){ return GetWeaponAmmo(3,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponAmmo_4_1(){ return GetWeaponAmmo(4,1); }
function interface string GetWeaponAmmo_4_2(){ return GetWeaponAmmo(4,2); }
function interface string GetWeaponAmmo_4_3(){ return GetWeaponAmmo(4,3); }
function interface string GetWeaponAmmo_4_4(){ return GetWeaponAmmo(4,4); }
function interface string GetWeaponAmmo_4_5(){ return GetWeaponAmmo(4,5); }
function interface string GetWeaponAmmo_4_6(){ return GetWeaponAmmo(4,6); }
function interface string GetWeaponAmmo_4_7(){ return GetWeaponAmmo(4,7); }
function interface string GetWeaponAmmo_4_8(){ return GetWeaponAmmo(4,8); }
function interface string GetWeaponAmmo_4_9(){ return GetWeaponAmmo(4,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponAmmo_5_1(){ return GetWeaponAmmo(5,1); }
function interface string GetWeaponAmmo_5_2(){ return GetWeaponAmmo(5,2); }
function interface string GetWeaponAmmo_5_3(){ return GetWeaponAmmo(5,3); }
function interface string GetWeaponAmmo_5_4(){ return GetWeaponAmmo(5,4); }
function interface string GetWeaponAmmo_5_5(){ return GetWeaponAmmo(5,5); }
function interface string GetWeaponAmmo_5_6(){ return GetWeaponAmmo(5,6); }
function interface string GetWeaponAmmo_5_7(){ return GetWeaponAmmo(5,7); }
function interface string GetWeaponAmmo_5_8(){ return GetWeaponAmmo(5,8); }
function interface string GetWeaponAmmo_5_9(){ return GetWeaponAmmo(5,9); }
//=============================================================================
function string GetWeaponLabel(int i,int j)
{
	local Weapon W;
	W=GetWeapon(i,j);
	if(W!=None && !W.bDeleteMe && W.ItemID!="")
		return W.ItemID;
	else
		return UnknownID;
}
//-----------------------------------------------------------------------------
function interface string GetWeaponLabel_1_1(){ return GetWeaponLabel(1,1); }
function interface string GetWeaponLabel_1_2(){ return GetWeaponLabel(1,2); }
function interface string GetWeaponLabel_1_3(){ return GetWeaponLabel(1,3); }
function interface string GetWeaponLabel_1_4(){ return GetWeaponLabel(1,4); }
function interface string GetWeaponLabel_1_5(){ return GetWeaponLabel(1,5); }
function interface string GetWeaponLabel_1_6(){ return GetWeaponLabel(1,6); }
function interface string GetWeaponLabel_1_7(){ return GetWeaponLabel(1,7); }
function interface string GetWeaponLabel_1_8(){ return GetWeaponLabel(1,8); }
function interface string GetWeaponLabel_1_9(){ return GetWeaponLabel(1,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponLabel_2_1(){ return GetWeaponLabel(2,1); }
function interface string GetWeaponLabel_2_2(){ return GetWeaponLabel(2,2); }
function interface string GetWeaponLabel_2_3(){ return GetWeaponLabel(2,3); }
function interface string GetWeaponLabel_2_4(){ return GetWeaponLabel(2,4); }
function interface string GetWeaponLabel_2_5(){ return GetWeaponLabel(2,5); }
function interface string GetWeaponLabel_2_6(){ return GetWeaponLabel(2,6); }
function interface string GetWeaponLabel_2_7(){ return GetWeaponLabel(2,7); }
function interface string GetWeaponLabel_2_8(){ return GetWeaponLabel(2,8); }
function interface string GetWeaponLabel_2_9(){ return GetWeaponLabel(2,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponLabel_3_1(){ return GetWeaponLabel(3,1); }
function interface string GetWeaponLabel_3_2(){ return GetWeaponLabel(3,2); }
function interface string GetWeaponLabel_3_3(){ return GetWeaponLabel(3,3); }
function interface string GetWeaponLabel_3_4(){ return GetWeaponLabel(3,4); }
function interface string GetWeaponLabel_3_5(){ return GetWeaponLabel(3,5); }
function interface string GetWeaponLabel_3_6(){ return GetWeaponLabel(3,6); }
function interface string GetWeaponLabel_3_7(){ return GetWeaponLabel(3,7); }
function interface string GetWeaponLabel_3_8(){ return GetWeaponLabel(3,8); }
function interface string GetWeaponLabel_3_9(){ return GetWeaponLabel(3,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponLabel_4_1(){ return GetWeaponLabel(4,1); }
function interface string GetWeaponLabel_4_2(){ return GetWeaponLabel(4,2); }
function interface string GetWeaponLabel_4_3(){ return GetWeaponLabel(4,3); }
function interface string GetWeaponLabel_4_4(){ return GetWeaponLabel(4,4); }
function interface string GetWeaponLabel_4_5(){ return GetWeaponLabel(4,5); }
function interface string GetWeaponLabel_4_6(){ return GetWeaponLabel(4,6); }
function interface string GetWeaponLabel_4_7(){ return GetWeaponLabel(4,7); }
function interface string GetWeaponLabel_4_8(){ return GetWeaponLabel(4,8); }
function interface string GetWeaponLabel_4_9(){ return GetWeaponLabel(4,9); }
//-----------------------------------------------------------------------------
function interface string GetWeaponLabel_5_1(){ return GetWeaponLabel(5,1); }
function interface string GetWeaponLabel_5_2(){ return GetWeaponLabel(5,2); }
function interface string GetWeaponLabel_5_3(){ return GetWeaponLabel(5,3); }
function interface string GetWeaponLabel_5_4(){ return GetWeaponLabel(5,4); }
function interface string GetWeaponLabel_5_5(){ return GetWeaponLabel(5,5); }
function interface string GetWeaponLabel_5_6(){ return GetWeaponLabel(5,6); }
function interface string GetWeaponLabel_5_7(){ return GetWeaponLabel(5,7); }
function interface string GetWeaponLabel_5_8(){ return GetWeaponLabel(5,8); }
function interface string GetWeaponLabel_5_9(){ return GetWeaponLabel(5,9); }
//=============================================================================
function int GetWeaponIcon(int i,int j)
{
	local Weapon W;
	W=GetWeapon(i,j);
	if (W?)	return W.IconIndex;
	else	return 0;
}
//-----------------------------------------------------------------------------
function interface int GetWeaponIcon_1_1(){ return GetWeaponIcon(1,1); }
function interface int GetWeaponIcon_1_2(){ return GetWeaponIcon(1,2); }
function interface int GetWeaponIcon_1_3(){ return GetWeaponIcon(1,3); }
function interface int GetWeaponIcon_1_4(){ return GetWeaponIcon(1,4); }
function interface int GetWeaponIcon_1_5(){ return GetWeaponIcon(1,5); }
function interface int GetWeaponIcon_1_6(){ return GetWeaponIcon(1,6); }
function interface int GetWeaponIcon_1_7(){ return GetWeaponIcon(1,7); }
function interface int GetWeaponIcon_1_8(){ return GetWeaponIcon(1,8); }
function interface int GetWeaponIcon_1_9(){ return GetWeaponIcon(1,9); }
//-----------------------------------------------------------------------------
function interface int GetWeaponIcon_2_1(){ return GetWeaponIcon(2,1); }
function interface int GetWeaponIcon_2_2(){ return GetWeaponIcon(2,2); }
function interface int GetWeaponIcon_2_3(){ return GetWeaponIcon(2,3); }
function interface int GetWeaponIcon_2_4(){ return GetWeaponIcon(2,4); }
function interface int GetWeaponIcon_2_5(){ return GetWeaponIcon(2,5); }
function interface int GetWeaponIcon_2_6(){ return GetWeaponIcon(2,6); }
function interface int GetWeaponIcon_2_7(){ return GetWeaponIcon(2,7); }
function interface int GetWeaponIcon_2_8(){ return GetWeaponIcon(2,8); }
function interface int GetWeaponIcon_2_9(){ return GetWeaponIcon(2,9); }
//-----------------------------------------------------------------------------
function interface int GetWeaponIcon_3_1(){ return GetWeaponIcon(3,1); }
function interface int GetWeaponIcon_3_2(){ return GetWeaponIcon(3,2); }
function interface int GetWeaponIcon_3_3(){ return GetWeaponIcon(3,3); }
function interface int GetWeaponIcon_3_4(){ return GetWeaponIcon(3,4); }
function interface int GetWeaponIcon_3_5(){ return GetWeaponIcon(3,5); }
function interface int GetWeaponIcon_3_6(){ return GetWeaponIcon(3,6); }
function interface int GetWeaponIcon_3_7(){ return GetWeaponIcon(3,7); }
function interface int GetWeaponIcon_3_8(){ return GetWeaponIcon(3,8); }
function interface int GetWeaponIcon_3_9(){ return GetWeaponIcon(3,9); }
//-----------------------------------------------------------------------------
function interface int GetWeaponIcon_4_1(){ return GetWeaponIcon(4,1); }
function interface int GetWeaponIcon_4_2(){ return GetWeaponIcon(4,2); }
function interface int GetWeaponIcon_4_3(){ return GetWeaponIcon(4,3); }
function interface int GetWeaponIcon_4_4(){ return GetWeaponIcon(4,4); }
function interface int GetWeaponIcon_4_5(){ return GetWeaponIcon(4,5); }
function interface int GetWeaponIcon_4_6(){ return GetWeaponIcon(4,6); }
function interface int GetWeaponIcon_4_7(){ return GetWeaponIcon(4,7); }
function interface int GetWeaponIcon_4_8(){ return GetWeaponIcon(4,8); }
function interface int GetWeaponIcon_4_9(){ return GetWeaponIcon(4,9); }
//-----------------------------------------------------------------------------
function interface int GetWeaponIcon_5_1(){ return GetWeaponIcon(5,1); }
function interface int GetWeaponIcon_5_2(){ return GetWeaponIcon(5,2); }
function interface int GetWeaponIcon_5_3(){ return GetWeaponIcon(5,3); }
function interface int GetWeaponIcon_5_4(){ return GetWeaponIcon(5,4); }
function interface int GetWeaponIcon_5_5(){ return GetWeaponIcon(5,5); }
function interface int GetWeaponIcon_5_6(){ return GetWeaponIcon(5,6); }
function interface int GetWeaponIcon_5_7(){ return GetWeaponIcon(5,7); }
function interface int GetWeaponIcon_5_8(){ return GetWeaponIcon(5,8); }
function interface int GetWeaponIcon_5_9(){ return GetWeaponIcon(5,9); }


defaultproperties
{
	TimeShownMax=3.0
}

