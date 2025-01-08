//=============================================================================
// HitActorDebugInfoManager.uc
// $Author: Mfox $
// $Date: 12/12/02 12:44p $
// $Revision: 53 $
//=============================================================================

class HitActorDebugInfoManager extends DebugInfoManager
	config(User);

/*-----------------------------------------------------------------------------
Handy class for showing on-screen information for the Actor currently under the
player's crosshair, or for a "locked" actor. See tracetoggle, tracelock etc.
exec commands.
*/

var config bool bShowLevel;
var config bool bShowAnimInfo;
var private Actor ActorToShow;
//!!MERGE var private Actor.CheckResult LevelHit;

//-----------------------------------------------------------------------------

function Cleanup()
{
	ActorToShow = None;
	Super.Cleanup();
}

//-----------------------------------------------------------------------------

function bool SetLevelHit()
{
	return false;
//!!MERGE 
/*
	local vector TraceStart;

	if( PCOwner.Pawn != None )
	{
		TraceStart		= PCOwner.Pawn.Location; 
		TraceStart.Z   += PCOwner.Pawn.BaseEyeHeight;
		return !class'Util'.static.RSLC( LevelHit, PCOwner.Pawn, TraceStart, vector(PCOwner.Rotation),,, PCOwner.TRACE_Level );
	}
	else
	{
		TraceStart		= PCOwner.Location; 
		return !class'Util'.static.RSLC( LevelHit, PCOwner, TraceStart, vector(PCOwner.Rotation),,, PCOwner.TRACE_Level );
	}
*/
}

//-----------------------------------------------------------------------------

function string GetBaseInfo()
{	
	local string StrTemp;

	StrTemp = class'Util'.static.GetFloatString(PCOwner.Level.TimeSeconds);
	if( GetActorToShow() != None || bConsoleEcho || bLogfileEcho )
	{
		StrTemp = StrTemp $ " (";
		if( GetActorToShow() != None )
			StrTemp = StrTemp $ "LOCKED";
		if( bConsoleEcho )
			StrTemp = StrTemp $ ">Con";
		if( bLogfileEcho )
			StrTemp = StrTemp $ ">Log";
		StrTemp = StrTemp $ ") ";
	}
	else
	{
		StrTemp = StrTemp $ " ";
	}

	return StrTemp;
}

//-----------------------------------------------------------------------------

function DoSetInfoStringsLevel()
{
//!!MERGE 
/*
	local Material Tex;
	local Material.CheckResult MaterialHit;
		
	MaterialHit.Next = LevelHit.Next;
	MaterialHit.Actor = LevelHit.Actor;
	MaterialHit.Location = LevelHit.Location;
	MaterialHit.Normal = LevelHit.Normal;
	MaterialHit.Primitive = LevelHit.Primitive;
	MaterialHit.Time = LevelHit.Time;
	MaterialHit.Item = LevelHit.Item;
	MaterialHit.Material = LevelHit.Material;
	
	if( PCOwner.Pawn != None )
		InfoStrings[InfoStrings.Length] = GetBaseInfo() $ LevelHit.Actor.Name $ " Dist=" $ int(VSize( LevelHit.Location - PCOwner.Pawn.Location));
	else
		InfoStrings[InfoStrings.Length] = GetBaseInfo() $ LevelHit.Actor.Name $ " Dist=" $ int(VSize( LevelHit.Location - PCOwner.Location));
	InfoStrings[InfoStrings.Length]	= " Location    " $ class'Util'.static.GetVectorString(LevelHit.Location);
	InfoStrings[InfoStrings.Length]	= " Normal      " $ class'Util'.static.GetVectorString(LevelHit.Normal);

	Tex = class'Material'.static.GetHitTexture( MaterialHit );
	if( Tex != None )
		InfoStrings[InfoStrings.Length]= " Texture     " $ Tex $ "(" $ EnumStr( enum'ETextureType', Tex.TextureType ) $ ")" ;
	else
		InfoStrings[InfoStrings.Length]= " Texture     " $ "Default";
*/
}

//-----------------------------------------------------------------------------

function ShowWeaponInfo( Pawn P )
{
	local int AmmoAmount, ClipRounds;
	local Inventory Inv;
	local Weapon W;
	local float Rating, Style;
	local int NumWeapons;
	local bool bFireEnabled, bAltFireEnabled;
	local byte bUseAltFire;

	W = P.Weapon;
	InfoStrings[InfoStrings.Length]	= "";
	if( W == None )
	{
		InfoStrings[InfoStrings.Length]	= " Weapon      " $ "None";
	}
	else
	{
		if( W.AmmoType != None )
			AmmoAmount = W.AmmoType.AmmoAmount;
		else
			AmmoAmount = -1;

		if( W.AmmoType != None )
			ClipRounds = W.AmmoType.ClipRoundsRemaining;
		else
			ClipRounds = -1;

		InfoStrings[InfoStrings.Length] = " Weapon      " $ W.Name $ " state=" $ W.GetStateName() $ " pt=" $ byte(W.bPointing) $ " rf=" $ byte(W.AmmoType.bRapidFire) $ " Ammo=" $ ClipRounds $ "(" $ AmmoAmount $ ")";
	}

	NumWeapons = -99;
		
	InfoStrings[InfoStrings.Length-1] = InfoStrings[InfoStrings.Length-1] $ "  NumWeapons: " $ NumWeapons;
	
	// show info on all weapons in inventory
	for( Inv=P.Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		W = Weapon(Inv);
		if( W != None )
		{
			if( W.AmmoType != None )
				AmmoAmount = W.AmmoType.AmmoAmount;
			else
				AmmoAmount = -1;

			if( P.Controller != None )
			{
				//!!MERGE
				//Rating = W.RateSelf( P.Controller.Enemy, bUseAltFire );
				//Style = W.SuggestAttackStyle( P.Controller.Enemy );
				Rating = -999.999;
				Style = -999.999;
			}

			//!!MERGE
			//bFireEnabled	= ( U2Weapon(W) == None || U2Weapon(W).bFireEnabledForOwner);
			//bAltFireEnabled = ( U2Weapon(W) == None || U2Weapon(W).bAltFireEnabledForOwner);
			bFireEnabled	= true;
			bAltFireEnabled = true;

			InfoStrings[InfoStrings.Length] = 
				"   " $ 
				class'Util'.static.PadString( W.Name, 32 ) $ 
				" Ammo=" $ class'Util'.static.PadString( AmmoAmount, 4 ) $
				" F/A=" $ byte(bFireEnabled) $ "/" $ byte(bAltFireEnabled) $
				" Rating= " $ class'Util'.static.PadString( class'Util'.static.GetFloatString(Rating), 7, true ) $
				" Alt=" $ bUseAltFire $ 
				" Style= " $ class'Util'.static.PadString( class'Util'.static.GetFloatString(Style), 7, true ) $ 
				" Priority= " $ class'Util'.static.PadString( class'Util'.static.GetFloatString(W.SwitchPriority()), 7, true );
		}
	}
}

//-----------------------------------------------------------------------------

function ShowInventoryInfo( Pawn P )
{
	local Inventory Inv;
	local bool bInventoryFound;
	
	// show info on non-weapon/ammo inventory
	InfoStrings[InfoStrings.Length]	= "";
	InfoStrings[InfoStrings.Length]	= " Inventory (except Weapons/Ammo)";
	for( Inv=P.Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		if( Weapon(Inv) == None && Ammunition(Inv) == None )
		{
			InfoStrings[InfoStrings.Length]	= "   " $ class'Util'.static.PadString( Inv.Name, 32 );
			bInventoryFound = true;
		}
	}

	if( !bInventoryFound )
		InfoStrings[InfoStrings.Length]	= "   NA";
}

//-----------------------------------------------------------------------------

function ShowPawnInfo( Pawn P )
{
	local Controller C;
	local Pawn Enemy;
	local Actor Target;
	local U2Pawn U2P;
	local float EnemyDist, EnemyCCDist, TargetDist, TargetCCDist;
	local string SkillStr;
	
	if( P == None )
		return;

	InfoStrings[InfoStrings.Length] = "";
	
	// quick access casts
	C = P.Controller;
	U2P = U2Pawn(P);

	// show skill/controller/physics/DebugFlags
	SkillStr = "Skill: NA";

	//SkillStr = SkillStr $ " Difficulty: " $ P.Level.Game.GetDifficulty() $ " MaxDifficulty: " $ P.Level.Game.GetMaxDifficulty();
	//SkillStr = SkillStr $ " DebugFlags=" $ GetDebugFlags() $ " bTrackStateChanges=" $ GetTrackStateChanges();
	
	// show controller/physics
	if( C != None )
		InfoStrings[InfoStrings.Length]	= " Controller  " $ C $ "  " $ SkillStr;
	else
		InfoStrings[InfoStrings.Length]	= " Controller  " $ C $ "  " $ SkillStr;

	// show controller state information
	if( C != None )
	{
		InfoStrings[InfoStrings.Length]	= " State       " $ C.GetStateName();
	}
	else
		InfoStrings[InfoStrings.Length]	= " State       " $ "NA";

	if( C != None )
		InfoStrings[InfoStrings.Length]	= " EyeHeight   " $ class'Util'.static.GetFloatString(P.EyeHeight) $ "(BEH:" $ class'Util'.static.GetFloatString(P.BaseEyeHeight) $ ") bFire=" $ C.bFire $ " bAltFire=" $ C.bAltFire $ "  TimerRate=" $ class'Util'.static.GetFloatString(C.TimerRate) $ " TimerCounter=" $ class'Util'.static.GetFloatString(C.TimerCounter);
	else
		InfoStrings[InfoStrings.Length]	= " EyeHeight   " $ class'Util'.static.GetFloatString(P.EyeHeight) $ "(BEH:" $ class'Util'.static.GetFloatString(P.BaseEyeHeight) $ ") bFire=NA bAltFire=NA";

	//if( U2P != None )
		//InfoStrings[InfoStrings.Length]	= " Health      " $ P.Health $ "/" $ P.default.Health $ " (hurt=" $ byte(U2P.GetHurt()) $ " alert=" $ byte(U2P.GetAlert()) $ " Stance=" $ U2P.GetStance() $ ") PS=" $ class'Util'.static.GetFloatString(U2P.GetPowerSuit()) $ "/" $ class'Util'.static.GetFloatString(U2P.GetPowerSuitMax());
	//else
		InfoStrings[InfoStrings.Length]	= " Health      " $ P.Health $ "/" $ P.default.Health;
	
	if( P.PlayerReplicationInfo != None )
		InfoStrings[InfoStrings.Length]	= " Name        " $ P.PlayerReplicationInfo.PlayerName $ " (PRI=" $ P.PlayerReplicationInfo $ " Team=" $ P.GetTeam() $ " TI=" $ P.PlayerReplicationInfo.Team $ ")";
	else
		InfoStrings[InfoStrings.Length]	= " Name        " $ "NA (Team=" $ P.GetTeam() $ ")";

	if( C != None )
	{
		Enemy = C.Enemy;
		if( Enemy != None )
		{
			EnemyDist	= int(VSize( Enemy.Location - P.Location));
			EnemyCCDist	= int(class'Util'.static.GetDistanceBetweenActors( Enemy, P ));
		}
		
		Target = C.Target;
		if( Target != None && Target != Enemy )
		{
			TargetDist	= int(VSize( Target.Location - P.Location));
			TargetCCDist	= int(class'Util'.static.GetDistanceBetweenActors( Target, P ));
		}

		if( Target == None || Target == Enemy )
		   InfoStrings[InfoStrings.Length]= " Enemy       " $ Enemy $ " Dist=" $ class'Util'.static.GetFloatString(EnemyDist) $ " CCDist=" $ class'Util'.static.GetFloatString(EnemyCCDist) $ " Target:" $ C.Target;
		else
		   InfoStrings[InfoStrings.Length]= " Enemy       " $ Enemy $ " Dist=" $ class'Util'.static.GetFloatString(EnemyDist) $ " CCDist=" $ class'Util'.static.GetFloatString(EnemyCCDist) $ " (Target " $ Target $ " Dist=" $ class'Util'.static.GetFloatString(TargetDist) $ " CCDist=" $ class'Util'.static.GetFloatString(TargetCCDist) $ ") Target:" $ C.Target;
	}

	ShowWeaponInfo( P );
	ShowInventoryInfo( P );
}

//-----------------------------------------------------------------------------

function ShowAnimInfo( Actor A )
{
	local int ii;
	local name Anim;
	local float Frame, Rate;
	
	for( ii=0; ii<10; ii++ )
	{
		A.GetAnimParams( ii, Anim, Frame, Rate );
		InfoStrings[InfoStrings.Length] = ii $ "  Anim: " $ class'Util'.static.PadString(Anim,20) $ " Frame: " $ Frame $ " Rate: " $ Rate;
		
		if( ii == 2 )
			InfoStrings[InfoStrings.Length] = "";
	}
}
	
//-----------------------------------------------------------------------------

function DoSetInfoStrings( Actor A )
{
	local int NumFrames;
	local int CurFrame;
	local float Distance, CCDistance;
	local string TexStr;
	//!!MERGE local Material.CheckResult CR;
	local name Anim;
	local float Frame, Rate;
	
	// if A is a Controller, show info for its Pawn
	if( Controller(A) != None && Controller(A).Pawn != None )
		A = Controller(A).Pawn;
		
	if( PCOwner.Pawn != None )
	{
		Distance	= VSize( A.Location - PCOwner.Pawn.Location);
		CCDistance	= class'Util'.static.GetDistanceBetweenActors( A, PCOwner.Pawn );
	}
	else
	{
		Distance	= VSize( A.Location - PCOwner.Location);
		CCDistance	= class'Util'.static.GetDistanceBetweenActors( A, PCOwner );
	}

	InfoStrings[InfoStrings.Length] = GetBaseInfo() $ A.Name $ " RxH=" $ int(A.CollisionRadius) $ "x" $ int(A.CollisionHeight) $ " Dist=" $ class'Util'.static.GetFloatString(Distance) $ " CCDist=" $ class'Util'.static.GetFloatString(CCDistance) $ " Accel=" $ class'Util'.static.GetFloatString(VSize(A.Acceleration)) $ " (" $ class'Util'.static.GetVectorString(A.Acceleration) $ ") Yaw: " $ class'Util'.static.GetFloatString( (A.Rotation.Yaw & 0xFFFF) * RotationUnitsToDegrees );

	NumFrames = -1;
	//!!MERGE
	/*
	if( A.Mesh != None )
	{
		NumFrames = A.GetAnimFrames(A.AnimSequence);
	}
	else
	{
		NumFrames = -1;
	}
	if( A.AnimFrame < 0.0 )
	{
		CurFrame = int( A.AnimFrame*NumFrames - 0.5 ); // tweening?
	}
	else
	{																
		CurFrame = int( A.AnimFrame*NumFrames + 1.5 ); // map 0..N-1 to 1..N
	}
	*/
	
	A.GetAnimParams( 0, Anim, Frame, Rate );
	CurFrame = int( Frame + 0.5 );

		
	//!!MERGE
	/*
	CR.Actor = A;
	MERGE CR.Location = A.Location;
	Tex = class'Material'.static.GetHitTexture( CR );
	if( Tex != None )
		TexStr = Tex $ "(" $ EnumStr( enum'ETextureType', Tex.TextureType ) $ ")" ;
	else*/
		TexStr = "NA";
	
	InfoStrings[InfoStrings.Length]		  = " Location    " $ class'Util'.static.PadString(class'Util'.static.GetVectorString(A.Location),40) $ "  Velocity=" $ class'Util'.static.GetFloatString(VSize(A.Velocity)) $ " (" $ class'Util'.static.GetVectorString(A.Velocity) $ ")";
	InfoStrings[InfoStrings.Length]		  = " State       " $ A.GetStateName() $ " (" $ class'Util'.static.GetPhysicsString(A) $ ")" ;
	InfoStrings[InfoStrings.Length]		  = " Event/Tag   " $ A.Event $ "/" $ A.Tag  $ "  Base: " $ A.Base;
	InfoStrings[InfoStrings.Length]		  = " Mesh        " $ A.Mesh $ " StaticMesh: " $ A.StaticMesh $ " Texture: " $ TexStr $ " (DS=" $ class'Util'.static.GetFloatString(A.DrawScale) $ " DT=" $ EnumStr( enum'EDrawType', A.DrawType) $ ")" $ " Rotation: " $ A.Rotation;
	InfoStrings[InfoStrings.Length]		  = " Anim        " $ Anim $ " (IsAnimating=" $ byte(A.IsAnimating()) $ " loop=" $ byte(A.SimAnim.bAnimLoop) $ " frame=" $ class'Util'.static.GetFloatString(CurFrame) $ " @ " $ class'Util'.static.GetFloatString(Rate) $ ")";

	ShowPawnInfo( Pawn(A) );
	
	//always show for now?
	//if( bShowAnimInfo )
	if( true )
	{
		InfoStrings[InfoStrings.Length] = "";
		ShowAnimInfo( A );
	}
}

//-----------------------------------------------------------------------------

function SetInfoStrings()
{
	local Actor HitActor;

	ClearInfoStrings();

	if( GetActorToShow() == None )
	{
		HitActor = class'Util'.static.GetHitActor( PCOwner, true );
	
		if( HitActor != None )
		{
			if( LevelInfo(HitActor) == None && TerrainInfo(HitActor) == None )
			{
				DoSetInfoStrings( HitActor );
			}
			else if( bShowLevel ) 
			{
				if( SetLevelHit() )
				{
					DoSetInfoStringsLevel();
				}
				else
				{
					InfoStrings[0] = " Error (level)";
				}
			}
		}
	}
	else
	{
		if( LevelInfo(GetActorToShow()) == None )
		{
		 	DoSetInfoStrings( GetActorToShow() );
		}
		else
		{
		 	DoSetInfoStringsLevel();
		}
	}
}

//-----------------------------------------------------------------------------

function SetActorToShow( Actor A )
{
     ActorToShow = A;
	 if( LevelInfo(A) != None )
	 {
		if( !SetLevelHit() )
	  	{
			log( "Error tracing to level!" );
			ActorToShow = None;
	  	}
	}
	else if( Pawn(A) != None && Pawn(A).Controller != None )
	{
		// lock controller instead so trace follows Controller to new Pawn if old Pawn dies
		ActorToShow = Pawn(A).Controller;
	}
}

//-----------------------------------------------------------------------------

function Actor GetActorToShow()
{
	if( ActorToShow != None && ActorToShow.bDeleteMe )
		SetActorToShow( None );
	return ActorToShow;
}

//-----------------------------------------------------------------------------

function SetShowLevel( bool bVal )
{
	bShowLevel = bVal;
}

//-----------------------------------------------------------------------------

function bool GetShowLevel()
{
	return bShowLevel;
}

//-----------------------------------------------------------------------------

function bool GetShowAnimInfo()
{
	return bShowAnimInfo;
}

//-----------------------------------------------------------------------------

function SetShowAnimInfo( bool bVal )
{
	bShowAnimInfo = bVal;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	XOffset=4
	YOffset=256
	bConsoleEcho=false
	bLogfileEcho=false
	bShowLevel=false
}
