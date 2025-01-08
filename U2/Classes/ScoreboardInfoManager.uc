//=============================================================================
// ScoreboardInfoManager.uc
// $Author: Mfox $
// $Date: 10/04/02 3:24p $
// $Revision: 16 $
//=============================================================================

class ScoreboardInfoManager extends DebugInfoManager
	config(User);

/*-----------------------------------------------------------------------------
A "scoreboard" for showing scores, health, states etc. mainly for debugging
purposes.
*/

const NameFieldLen			= 20;
const HealthFieldLen		=  4;
const PowerSuitFieldLen		=  7;
const WeaponFieldLen		=  2;
const EnemyFieldLen			= 20;
const StateFieldLen			= 24;
const MetaStateFieldLen		=  4;
const OrdersFieldLen		= 10;
const OrdersObjectFieldLen	= 20;
const GoalFieldLen			= 20;
							
const MaxCountedPawns		= 32;	// number of PCs/NPCs which will be processed (for sorting by score, priority etc.)
const MaxDisplayedPawns		= 32;	// number of PCs/NPCs which will be displayed (rest fall off the bottom of the list)

enum ESBType
{
	SB_None,
	SB_Client,
	SB_ServerMP,
	SB_ServerSP,
};

var ESBType SBType;
var string StaticHeaderStr;
var PlayerReplicationInfo PRIArray[ MaxCountedPawns ];

//-----------------------------------------------------------------------------

function Cleanup()
{
	RemoveAllTimers();
	Super.Cleanup();
}

//-----------------------------------------------------------------------------

function SetEnabled( byte bVal )
{
	Super.SetEnabled( bVal );

	if( bVal > 0 )
	{
		if( PCOwner.Level.NetMode != NM_Standalone )
		{
			SBType = SB_Client;
		}
		else	
		{
			// IsA used to break dependency on U2DeathMatch assets
			// NOTE: typos will cause IsA to fail silently.
			// Might be worth it to make a IsASafe() that checks for a valid class name
			if( PCOwner.Level.Game.IsA('U2DeathMatch') )
			{
				SBType = SB_ServerMP;
			}
			else
			{
				SBType = SB_ServerSP;
			}
		}

		SetStaticHeaderStr();
	}

	// !!mdf-tbr: PRIArray not getting correctly filled in (in 
	// GameReplicationInfo.Timer?) so using timer to do this once per second.
	if( PCOwner.Level.NetMode != NM_Standalone )
	{
		if( bVal == 0 )
			RemoveTimer( 'UpdatePRIArrayTimer' );
		else
			AddTimer( 'UpdatePRIArrayTimer', 1.0, true );
	}
}

//-----------------------------------------------------------------------------
// !!mdf-tbr: PRIArray not getting correctly filled in (in 
// GameReplicationInfo.Timer?) so using timer to do this once per second.

function UpdatePRIArrayTimer()
{
	local int ii;
	local PlayerReplicationInfo PRI;

	ii = 0;
	foreach PCOwner.DynamicActors( class'PlayerReplicationInfo', PRI )
		PRIArray[ ii++ ] = PRI;
}

//-----------------------------------------------------------------------------

function bool TeamGame()
{
	return ( PCOwner.Level.Game != None && PCOwner.Level.Game.bTeamGame );
}

//-----------------------------------------------------------------------------

function SetStaticHeaderStr()
{
	StaticHeaderStr = "  " $ class'Util'.static.PadString( "Name", NameFieldLen );

	if( SBType != SB_ServerSP )
		StaticHeaderStr = StaticHeaderStr $ " Scor Sprd Dths";
	 				  								   // 1234
   	StaticHeaderStr = StaticHeaderStr $ " Team";

	if( PCOwner.Level.NetMode == NM_Standalone )
	{																										 
		if( bEnabled > 1 )
		{
			StaticHeaderStr = StaticHeaderStr $ " Hlth PwrSuit We";
			StaticHeaderStr = StaticHeaderStr @ class'Util'.static.PadString( "Enemy", EnemyFieldLen );
			StaticHeaderStr = StaticHeaderStr @ class'Util'.static.PadString( "State/Script", StateFieldLen );
			StaticHeaderStr = StaticHeaderStr @ class'Util'.static.PadString( "Meta", MetaStateFieldLen );
			StaticHeaderStr = StaticHeaderStr @ class'Util'.static.PadString( "Orders", OrdersFieldLen );
			StaticHeaderStr = StaticHeaderStr @ class'Util'.static.PadString( "OrdersObject", OrdersObjectFieldLen );
			StaticHeaderStr = StaticHeaderStr @ class'Util'.static.PadString( "Goal", 0 );
		}
	}
}

//-----------------------------------------------------------------------------

function AppendHeader( coerce string Str )
{
	InfoStrings[ InfoStrings.Length ] = StaticHeaderStr @ Str;
}

/*-----------------------------------------------------------------------------
NOTE: afaict there isn't much that can be done to optimize this without moving
to native code. The overhead for 30 or so NPCs is about 29ms per frame. About
5ms of this is due to the code in this function up to but not including the
"extra debug info" section. The "extra debug info" accounts for about 18ms and
about 11ms of that is due to the code near the bottom which concatenates all
the strings together. The remaining 6ms or so is the rendering overhead. This
component is only used for debugging so I'm not going to worry about this 
overhead for now.
*/

function SetPawnInfo( float BestScore, PlayerReplicationInfo PRI, optional Controller C )
{
	local string NameStr;
	local string PawnStr;
	local string BestScoreStr;
	local string ScoreStr;
	local string DeathsStr;
	local string TeamStr;
	local string HealthStr;
	local string PowerSuitStr;
	local string WeaponStr;
	local string EnemyStr;
	local string StateStr;
	local string MetaStateStr;
	local string OrdersStr;
	local string OrdersObjectStr;
	local string GoalStr;
	local Pawn Enemy;
	
	if( (SBType == SB_ServerSP || TeamGame()) && C.Pawn != None )
		TeamStr = string(C.Pawn.GetTeam());
	else
		TeamStr = "XX";

	if( SBType != SB_ServerSP )
	{
		if( PRI != None )
		{
			NameStr	= PRI.PlayerName;
			BestScoreStr = string(int(PRI.Score - BestScore));
			ScoreStr = string(int(PRI.Score));
			DeathsStr = string(int(PRI.Deaths));
		}
		else if( C != None )
		{
			if( C.Pawn != None )
				NameStr	= Right( C.Pawn.Name, NameFieldLen);
			else
				NameStr	= Right( C.Name, NameFieldLen);
	
			BestScoreStr = "NA";
			ScoreStr = "NA";
			DeathsStr = "NA";
		}
	}
	else if( C != None )
	{
		if( C.Pawn != None )
			NameStr	= Right( C.Pawn.Name, NameFieldLen);
		else
			NameStr	= Right( C.Name, NameFieldLen);
	}

	if( C != None )
	{
		if( C.bIsPlayer )
		{
			if( C == PCOwner )
				PawnStr = "* ";	// player who owns scoreboard
			else if( PlayerController(C) != None )
				PawnStr = "P "; // some other player
			else
				PawnStr = "B "; // some bot
		}
		else
		{
			// NPC
			if( C.bStasis )
				PawnStr = "- ";	// dormant NPC
			//!!MERGE
			//else if( StationaryPawn(C.Pawn) != None )
			//	PawnStr = "S ";	// active StationaryPawn
			else
				PawnStr = "N ";	// active NPC
		}
	}
	else
	{
		PawnStr = "# "; // dead?
	}

	// main info
	if( SBType != SB_ServerSP )
	{
		InfoStrings[InfoStrings.Length] = 
			PawnStr $												// flag bots with *
			class'Util'.static.PadString( NameStr,         NameFieldLen+1 ) $	// name
			class'Util'.static.PadString( ScoreStr,        5 ) $	// score
			class'Util'.static.PadString( BestScoreStr,    5 ) $	// spread
			class'Util'.static.PadString( DeathsStr,       5 );		// deaths
	}
	else
	{
		InfoStrings[InfoStrings.Length] = 
			PawnStr $												// flag bots with *
			class'Util'.static.PadString( NameStr,        NameFieldLen+1 );		// name
	}

	InfoStrings[InfoStrings.Length-1] = InfoStrings[InfoStrings.Length-1] $ class'Util'.static.PadString( TeamStr, 5 );	// team	id

	if( C != None && bEnabled > 1 )
	{
		// extra debug info
		if( C.Pawn != None )
			HealthStr = string(Min( C.Pawn.Health, 9999 ));
		else
			HealthStr = "XXX";

		//!!MERGE
		//if( U2Pawn(C.Pawn) != None )
		//	PowerSuitStr = string(int(U2Pawn(C.Pawn).GetPowerSuit())) $ "/" $ string(int(U2Pawn(C.Pawn).GetPowerSuitMax()));
		//else
			PowerSuitStr = "XXX";

		//!!MERGE
		//if( C.Pawn != None && C.Pawn.Weapon != None )
		//	WeaponStr = C.Pawn.Weapon.GetShortName();
		//else
			WeaponStr = "XX";

		Enemy = C.Enemy;
		if( Enemy != None )
		{
			if( Enemy.PlayerReplicationInfo != None )
				EnemyStr = Enemy.PlayerReplicationInfo.PlayerName;
			else
				EnemyStr = string(Enemy.Name);

			EnemyStr = Right( EnemyStr, EnemyFieldLen );
		}
		else
			EnemyStr = "--";

		//!!MERGE StateStr = Right( C.GetStateInfoString(), StateFieldLen );

		InfoStrings[InfoStrings.Length-1] = 
			InfoStrings[InfoStrings.Length-1] $
			class'Util'.static.PadString( HealthStr,		HealthFieldLen+1 )		$		// health
			class'Util'.static.PadString( PowerSuitStr,		PowerSuitFieldLen+1 )	$		// powersuit info
			class'Util'.static.PadString( WeaponStr,		WeaponFieldLen+1 )		$		// weapon
			class'Util'.static.PadString( EnemyStr,			EnemyFieldLen+1 )		$		// enemy name
			class'Util'.static.PadString( StateStr,			StateFieldLen+1 )		$		// controller state
			class'Util'.static.PadString( MetaStateStr,		MetaStateFieldLen+1 )	$		// behavior controller metastate
			class'Util'.static.PadString( OrdersStr,		OrdersFieldLen+1 )		$		// NPC orders
			class'Util'.static.PadString( OrdersObjectStr,	OrdersObjectFieldLen+1 )$		// NPC order object
			class'Util'.static.PadString( GoalStr,			GoalFieldLen+1 );				// NPC special goal
	}
}

//-----------------------------------------------------------------------------

function SetClientInfoStrings()
{
	local PlayerReplicationInfo PRI;
	local PlayerReplicationInfo SortedPawns[ MaxCountedPawns ];
	local int ii, jj;
	local int PawnIndex, NumPawns, TotalPawns;
	//local U2GameReplicationInfo GRI;

	// !!mdf-tbd: ignore in standalone games (is GRI None in these cases)?
	if( PCOwner.Level.NetMode != NM_Standalone )
	{
		//GRI = U2GameReplicationInfo(PCOwner.GameReplicationInfo);
		//if( GRI != None )
		//{
			// get pawns from GRI
			for( PawnIndex=0; PawnIndex<MaxCountedPawns; PawnIndex++ )
			{
				if( NumPawns < MaxCountedPawns )
				{
					//PRI = GRI.PRIArray[ PawnIndex ];
					PRI = PRIArray[ PawnIndex ];
	
					if( PRI != None )
					{
						// insert into list, highest scores first
						ii=0;
			  			while( ii<NumPawns && PRI.Score < SortedPawns[ii].Score )
			   				ii++;
			
			   			// shift everyone else down
			   			for( jj=NumPawns-1; jj>ii; jj-- )
						{
							if( jj < MaxCountedPawns )
			   					SortedPawns[jj] = SortedPawns[jj-1];
						}

			   			// insert new one
			   			SortedPawns[ii] = PRI;
			   			NumPawns++;
					}
				}

				TotalPawns++;
			}
		//}
	}

	if( NumPawns >= 1 )
	{
		NumPawns = Min( NumPawns, MaxDisplayedPawns );

		AppendHeader( "(shown:" $ NumPawns $ "/" $ TotalPawns $ ")" );

		for( ii=0; ii<NumPawns; ii++ )
			SetPawnInfo( SortedPawns[0].Score, SortedPawns[ii] );
	}
}

//-----------------------------------------------------------------------------

function SetServerInfoStringsMP()
{
	local Controller C;
	local PlayerReplicationInfo PRI;
	local Controller SortedPawns[ MaxCountedPawns ];
	local int ii, jj;
	local int NumPawns, TotalPawns;

	// sort pawns by decreasing score
	for( C=PCOwner.Level.ControllerList; C!=None; C=C.NextController )
	{
		if( NumPawns < MaxCountedPawns )
		{
			PRI = C.PlayerReplicationInfo;
	   		if( (C.bIsPlayer && PRI != None) || bEnabled > 1 )
	   		{
	   			if( PRI == None )
	   			{
	   				// insert into list after any NPCs with a PRI (for debugging help)
	   				ii=0;
	   				while( ii<NumPawns && SortedPawns[ii].PlayerReplicationInfo != None )
	   					ii++;
	   			}
	   			else
	   			{
	   				// insert into list, highest scores first
	   				ii=0;
	   				while( ii<NumPawns && PRI.Score < SortedPawns[ii].PlayerReplicationInfo.Score )
	   					ii++;
	   			}
	
	   			// shift everyone else down
	   			for( jj=NumPawns; jj>ii; jj-- )
				{
					if( jj < MaxCountedPawns )
		   				SortedPawns[jj] = SortedPawns[jj-1];
				}

	   			// insert new one
	   			SortedPawns[ii] = C;
	   			NumPawns++;
   			}
		}

		TotalPawns++;
	}

	if( NumPawns >= 1 )
	{
		NumPawns = Min( NumPawns, MaxDisplayedPawns );

		AppendHeader( "(shown: " $ NumPawns $ "/" $ TotalPawns $ ")" );

		for( ii=0; ii<NumPawns; ii++ )
		{
			C = SortedPawns[ii];
			PRI = C.PlayerReplicationInfo;

			SetPawnInfo( SortedPawns[0].PlayerReplicationInfo.Score, PRI, C );
		}
	}
}

//-----------------------------------------------------------------------------

function SetServerInfoStringsSP()
{
	local Controller C;
	local Controller SortedPawns[ MaxCountedPawns ];
	local int ii, jj;
	local int NumPawns, TotalPawns, ActivePawns;

	for( C=PCOwner.Level.ControllerList; C!=None; C=C.NextController )
	{
		if( C.Pawn == None || C.Pawn.Health <= 0 )
			continue;

   		if( C.bIsPlayer || bEnabled > 1 )
   		{
			ii=0;
			while( ii<NumPawns && SortedPawns[ ii ].bIsPlayer )
				ii++; // skip to first non-player

			//Log( "  first non player is at " $ ii );

			//!!MERGE
			/*
			if( StationaryPawn(C.Pawn) != None )
			{
				// skip to first stationary pawn or bStasis NPC found
				while( ii<NumPawns && StationaryPawn(SortedPawns[ ii ].Pawn) == None && !SortedPawns[ ii ].bStasis )
					ii++;

				//Log( "  first SP / bStasis is at " $ ii );
			}
			*/
			
			if( !C.bIsPlayer && C.bStasis )
			{
				// skip to first bStasis NPC
				//while( ii<NumPawns && !SortedPawns[ ii ].bStasis && C.Pawn != None && (StationaryPawn(C.Pawn) == None || StationaryPawn(SortedPawns[ ii ].Pawn) != None) )
				while( ii<NumPawns && !SortedPawns[ ii ].bStasis && C.Pawn != None )
					ii++;

				//Log( "  first bStasis is at " $ ii );

				//!!MERGE
				/*
				if( StationaryPawn(C.Pawn) != None )
				{
					// skip to first bStasis stationary pawn
					while( ii<NumPawns && StationaryPawn(SortedPawns[ ii ].Pawn) == None )
						ii++;

					//Log( "  first bStasis SP is at " $ ii );
				}
				*/
			}

			if( ii < MaxCountedPawns )
			{
				// insert at ii after shifting everyone else down (possibly off the end of the list)
	   			for( jj=NumPawns; jj>ii; jj-- )
				{
					if( jj < MaxCountedPawns )
		   				SortedPawns[jj] = SortedPawns[jj-1];
				}

	   			// insert new one
	   			SortedPawns[ii] = C;
			}

			if( NumPawns < MaxCountedPawns )
	   			NumPawns++;
   		}

		TotalPawns++;
		if( !C.Pawn.bStasis )
			ActivePawns++;

		//Log( "  current list" );
		//for( jj=0; jj<NumPawns; jj++ )
			//Log( "    " $ jj @ SortedPawns[jj].Pawn );
	}

	if( NumPawns >= 1 )
	{
		NumPawns = Min( NumPawns, MaxDisplayedPawns );

		AppendHeader( "(shown:" $ NumPawns $ "/" $ TotalPawns $ " active:" $ ActivePawns $ ")" );

		for( ii=0; ii<NumPawns; ii++ )
			SetPawnInfo( 0, None, SortedPawns[ii] );
	}
}

//-----------------------------------------------------------------------------

function SetInfoStrings()
{
	switch( SBType )
	{
	case SB_Client:
		SetClientInfoStrings();
		break;
	case SB_ServerMP:
		SetServerInfoStringsMP();
		break;
	case SB_ServerSP:
		SetServerInfoStringsSP();
		break;
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
	XOffset=4
	YOffset=-20
	MaxEnabled=2
}
