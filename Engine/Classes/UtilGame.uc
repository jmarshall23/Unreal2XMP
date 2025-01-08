//=============================================================================
// UtilGame.uc
// $Author: Mfox $
// $Date: 1/05/03 8:45p $
// $Revision: 38 $
//=============================================================================
//NEW: file

class UtilGame extends Actor // never instantiated and extending Actor makes things easier
	abstract;

//=============================================================================
// Game-level utility functions, e.g. inventory, actor searching, AI. 
//=============================================================================

static final function DumpInventory( Pawn P, optional string ContextString )
{
	local Inventory Inv;
	
	P.DMTNS( "DumpInventory" @ ContextString $ ":" );
	for( Inv = P.Inventory; Inv != None; Inv = Inv.Inventory )
		Inv.Dump( "  ", "" );
}

//-----------------------------------------------------------------------------
// Spawn and add to inventory if not already present. If an item of the same
// class is present, return that one.

static final function Inventory GiveInventoryClass( Pawn TargetPawn, class<Inventory> InvClass )
{
	local Inventory Inv, InvNew;

	//TargetPawn.DMTNS( "GiveInventoryClass " $ InvClass );
	//class'UtilGame'.static.DumpInventory( TargetPawn, "GiveInventoryClass BEGIN" );
	if( InvClass != None )
	{
		Inv = TargetPawn.FindInventoryType( InvClass );
		if( Inv == None || Inv.bMergesCopies )
		{
			InvNew = TargetPawn.Spawn( InvClass );
			if( InvNew != None )
				InvNew.GiveTo( TargetPawn );
		}
	}
	//class'UtilGame'.static.DumpInventory( TargetPawn, "GiveInventoryClass END" );
	if( Inv != None )
		return Inv;
	else
		return InvNew;
}

//-----------------------------------------------------------------------------
// Spawn and add to inventory if not already present.

static final function Inventory GiveInventoryString( Pawn TargetPawn, coerce string InventoryString )
{
	return GiveInventoryClass( TargetPawn, class<Inventory>(DynamicLoadObject( InventoryString, class'Class' )) );
}

//-----------------------------------------------------------------------------
// Add to inventory if not already present.

static final function bool GiveInventory( Pawn TargetPawn, Inventory Inv )
{
	local Inventory MatchingInv;

	if( Inv != None )
	{
		MatchingInv = TargetPawn.FindInventoryType( Inv.Class );
		if( MatchingInv == None || Inv.bMergesCopies )
		{
			Inv.GiveTo( TargetPawn );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Return inventory item from holder's inventory matching class 'InventoryType'

static final function Inventory GetInventoryItem( Pawn InventoryHolder, class<Inventory> InventoryType )
{
	local Inventory CurrentInventoryItem;
	local Inventory FoundInventoryItem;

	if( InventoryHolder == None )
		return None;

	for( CurrentInventoryItem = InventoryHolder.Inventory;
			( ( None != CurrentInventoryItem ) && ( None == FoundInventoryItem ) );
			CurrentInventoryItem = CurrentInventoryItem.Inventory )
	{
		if( ClassIsChildOf( CurrentInventoryItem.Class, InventoryType ) )
		{
			FoundInventoryItem = CurrentInventoryItem;
		}
	}
	
	return FoundInventoryItem;
}

//-----------------------------------------------------------------------------

static function SetAmmoTypes( Weapon W, Pawn Other )
{
	local int ii;
	
	for( ii=0; ii<ArrayCount( W.AmmoTypes ); ii++ )
	{
		if ( W.AmmoName[ ii ] != None )
		{
			W.AmmoTypes[ ii ] = Ammunition(Other.FindInventoryType( W.AmmoName[ ii ] ));
			if ( W.AmmoTypes[ ii ] == None )
			{		
				W.AmmoTypes[ ii ] = W.Spawn( W.AmmoName[ ii ] );	// Create ammo type required		
				Other.AddInventory( W.AmmoTypes[ ii ] );			// and add to player's inventory
			}
		}
	}
}

//-----------------------------------------------------------------------------

static function AddAllAmmoPercent( Pawn Other, float AddedAmmoPct )
{
	local Inventory Inv;

	if( !class'Pawn'.static.ValidPawn( Other ) )
		return;

	for( Inv=Other.Inventory; Inv!=None; Inv=Inv.Inventory )
		Inv.Resupply( AddedAmmoPct );
}

//-----------------------------------------------------------------------------

static function float ScaleAmmoResupplyRate( Pawn Other, float InitialRate )
{
	local Weapon W;
	local Inventory Inv;
	local int DepletedAmmoTypes, TotalAmmoTypes;

	// count the number of ammo types that need ammo
	for( Inv=Other.Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		W = Weapon(Inv);
		if( W? && W.AmmoType? && W.AmmoType.AmmoUsed > 0 )
		{
			TotalAmmoTypes++;
			if( W.ResupplyStatus() < 1.0 )
				DepletedAmmoTypes++;
		}
	}

	if( TotalAmmoTypes > 0 )
		return InitialRate * float(TotalAmmoTypes) / float(DepletedAmmoTypes);
	else
		return InitialRate;
}

//-----------------------------------------------------------------------------

static final function MakeShake( Actor Context, vector ShakeLocation, float ShakeRadius, float ShakeMagnitude, optional float ShakeDuration )
{
	local Controller C;
	local PlayerController Player;
	local float Dist,Pct;

	if( Context==None || ShakeRadius<=0 || ShakeMagnitude<=0 )
		return;

	for( C=Context.Level.ControllerList; C!=None; C=C.nextController )
	{
		Player = PlayerController(C);
		//NEW (mb) - don't shake a flying / ghosted player (leave shake in for matinee cutscenes)
		if( Player!=None &&
			(Player.Pawn==None || Player.Pawn.Physics != PHYS_Flying) )
		/*OLD
		if( Player!=None )
		*/
		{
			Dist = VSize(ShakeLocation-Player.Location);
			if( Dist<ShakeRadius )
			{
				Pct = 1.0 - (Dist / ShakeRadius);
				Player.ShakeView( ShakeMagnitude * Pct, ShakeDuration );
			}
		}
	}
}

//=============================================================================
//@ Aiming
//=============================================================================

static final function vector GetRotatedFireStart( Pawn SourcePawn, vector SourceLocation, rotator TargetRotation, vector FireOffset )
{
	local vector X, Y, Z;
	local vector ReturnedFireStart;
	
	GetAxes( TargetRotation, X, Y, Z );
	
	ReturnedFireStart = SourceLocation + SourcePawn.EyePosition() + FireOffset.X * X + FireOffset.Y * Y + FireOffset.Z * Z;
	
	// prone NPCs are getting a fire start that is in the ground?
	ReturnedFireStart.Z = FMax( SourceLocation.Z - 0.5*SourcePawn.CollisionHeight, ReturnedFireStart.Z );
	
	//SourcePawn.DMTNS( "GetRotatedFireStart returning " $ ReturnedFireStart );
	//SourcePawn.DMTNS( "  TargetRotation: " $ TargetRotation );
	//SourcePawn.DMTNS( "  EyePosition:    " $ TargetPawn.EyePosition() );
	return ReturnedFireStart;
}

//=============================================================================
//@ Searching
//=============================================================================

//-----------------------------------------------------------------------------
// Search for closest actor to InstanceActor which matches given ClassName 
// (bIsA=false) or IsA(ClassName) (bIsA=true). Returns None if no such actor
// found. Matches with InstanceActor are NOT filtered out.

static final function Actor GetClosestOfClass( Actor InstanceActor, name ClassName, bool bIsA )
{
	local Actor A;
	local float MinDist;
	local float Distance;
	local Actor Closest;

	// try to match closest actor with same class (not including super classes) as given name
	Closest = None;
	MinDist = 999999;

	foreach InstanceActor.AllActors( class'Actor', A )
	{
		if( A.Class.Name == ClassName || (bIsA && A.IsA(ClassName)) )
		{
			Distance = VSize( InstanceActor.Location - A.Location );
			if( Distance < MinDist )
			{
				Closest = A;
				MinDist = Distance;
			}
		}
	}

	return Closest;
}

//-----------------------------------------------------------------------------
// NOTE: if this is called with a target name that ends in a digit, we look for
// a matching actor in the level. But, this isn't safe if the actor was spawned
// in-game since there is no guarantee that the same name will be used each 
// time. Will normally return None for these cases, use bAllowUnSafe=true to
// allow such actors to be found, e.g. for testing purposes.

static final function Actor GetNamedActor( Actor InstanceActor, name N, optional bool bTryClassMatch, optional bool bAllowUnSafe )
{
	local Actor A;

	if( InstanceActor != None )
	{
		if( !bTryClassMatch || class'Util'.static.EndsInDigit( string(N) ) )
		{
			// first try to find an actor with the given name (can be at most one)
			foreach InstanceActor.AllActors( class'Actor', A )
			{
				if( A.Name == N )
					return A;
			}
		}

		if( bTryClassMatch )
		{
			A = GetClosestOfClass( InstanceActor, N, false );		// try exact match
			if( A == None )
				A = GetClosestOfClass( InstanceActor, N, true );	// try IsA match
	
			return A;
		}
	}

	return None;
}

//-----------------------------------------------------------------------------

static final function Actor GetNamedOrHitActor( Controller C, optional name TargetName, optional bool bTraceVisibleNonColliding, optional bool bTryClassMatch )
{
	if( TargetName != '' )
	{
		return GetNamedActor( C, TargetName, bTryClassMatch, true );
	}
	else
	{
		return class'Util'.static.GetHitActor( C, bTraceVisibleNonColliding );
	}
}

//-----------------------------------------------------------------------------

static final function bool FindActorByName( Actor SearchingActor, Name ActorName, out Actor FoundActor )
{
	local Actor CurrentActor;
	local bool bFoundActor;
	
	foreach SearchingActor.AllActors( class'Actor', CurrentActor )
	{
		if( CurrentActor.Name == ActorName )
		{
			FoundActor = CurrentActor;
			bFoundActor = true;
			break;
		}
	}

	return bFoundActor;
}

//=============================================================================
//@ Visibility
//=============================================================================

//-----------------------------------------------------------------------------
// Allows you to check if an object is in your view.
//-----------------------------------------------------------------------------
// ViewVec:	A vector facing "forward"						(relative to your location.)
// DirVec:	A vector pointing to the object in question.	(relative to your location.)
// FOVCos:	Cosine of how many degrees between ViewVec and DirVec to be seen.
//-----------------------------------------------------------------------------
// REQUIRE: FOVCos > 0
// NOTE: While normalization is not required for ViewVec or DirVec, it helps
// if both vectors are about the same size.
//-----------------------------------------------------------------------------

static final function bool IsInViewCos( vector ViewVec, vector DirVec, float FOVCos )
{
	local float CosAngle;		//cosine of angle from object's LOS to WP
    
    CosAngle = Normal( ViewVec ) dot  Normal( DirVec );

	//The first test makes sure the target is within the firer's front 180o view.
	//The second test might look backwards, but it isn't.  Since cos(0) == 1,
	//as the angle gets smaller, CosAngle *increases*, so an angle less than
	//the max will have a larger cosine value.
	
	return (0 <= CosAngle && FOVCos < CosAngle);
}

//-----------------------------------------------------------------------------
// Returns true if Target is in within given FOVCos wrt SeeingActor.
//-----------------------------------------------------------------------------

static final function bool ActorLookingAt( Actor SeeingActor, Actor Target, float FOVCos )
{
	//2002.12.19 (mdf) warning fix
	if( Target == None || SeeingActor == None )
		return false;

	return IsInViewCos( vector(SeeingActor.Rotation), Target.Location - SeeingActor.Location, FOVCos );
}

//-----------------------------------------------------------------------------
// Allow various classes of actors with teams to set their skins appropriately
//-----------------------------------------------------------------------------

static final function SetTeamSkin( Actor TargetActor, int NewTeam )
{
	if( NewTeam == 255 )
	{
		// neutral team - clear team skin
		if( TargetActor.Level.NetMode != NM_DedicatedServer )
		{
			// access skins array directly
			if( TargetActor.Skins.Length <= TargetActor.RepSkinIndex )
				TargetActor.Skins.Length = TargetActor.RepSkinIndex + 1;
			TargetActor.Skins[TargetActor.RepSkinIndex] = None;
		}

		if( TargetActor.Level.NetMode == NM_DedicatedServer ||
			TargetActor.Level.NetMode == NM_ListenServer )
		{
			// replicate to client
			TargetActor.RepSkin = None;
		}
	}
	else
	{
		// team skin - assign to actor
		if( NewTeam < TargetActor.AlternateSkins.Length )
		{
			if( TargetActor.Level.NetMode != NM_DedicatedServer )
			{
				// access skins array directly
				if( TargetActor.Skins.Length <= TargetActor.RepSkinIndex )
					TargetActor.Skins.Length = TargetActor.RepSkinIndex + 1;
				TargetActor.Skins[TargetActor.RepSkinIndex] = TargetActor.AlternateSkins[NewTeam];
			}

			if( TargetActor.Level.NetMode == NM_DedicatedServer ||
				TargetActor.Level.NetMode == NM_ListenServer )
			{
				// replicate to client
				TargetActor.RepSkin	= TargetActor.AlternateSkins[NewTeam];
			}
		}
	}
}