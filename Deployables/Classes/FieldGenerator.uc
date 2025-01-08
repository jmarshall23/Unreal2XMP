/*-----------------------------------------------------------------------------
When spawned (via a FieldGeneratorDeployable), checks for other nearby FGs to
which it can link. If primary-activated, can join with up to 2 other FGs, if
alt-activated can join with 1 other FG. If a link is possible, spawns a 
ForceWall to provide collision and the visual effect associated with the 
force field.

If picked up, increases the inventory count.

In theory, the health of FGs is preserved by maintaining a sorted list of FG
health in the FieldGeneratorDeployable but I haven't tested whether this works
in networked games.
*/

class FieldGenerator extends DeployedUnit
	placeable;

/*-----------------------------------------------------------------------------
Networking comments.

FG:		FieldGenerator (2 of these anchor the ends of each force field)
FW:		ForceWall (dynamic blocking volume used to provide collision, display force field)

Issues:

	+The replication relevancy code only does location to location traces to
	 verify visibility which is why ForceWalls (and vehicles) are set to use
	 bAlwaysRelevant. The FGs themselves use "standard" relevancy rules and 
	 will be created/destroyed on clients accordingly.
	 
	+We want FWs to exist on the server and on the client to ensure that
	 client movement is correct (if these afre present only on the client,
	 they won't block Pawns, if these are present only on the server then
	 client prediction will be "off" and Pawns might "vibrate" as their
	 location is continually corrected on the server).
	 
	+When "using" a field generator or force field, these requests need to be
	 routed to the server, handled there, then the necessary information replicated
	 to the client so the force field can be adjusted if needed (e.g. turn collision
	 on/off, destroy force field).
*/

struct TFGLink
{
	var FieldGenerator		LinkFG;
	var ForceWall			LinkFW;
};

// properties which control force wall behavior
var float LinkRadius;
var int	NumPriLinks;
var int	NumAltLinks;
var float CosMinAngle;
var float TraceTolerance;
var Sound LinkSound;
var Sound UnlinkSound;

var int AvailableLinks;			// number of available links (=FGLinks.Length)
var int NumLinks;				// number of active links
var	array<TFGLink> FGLinks;		// list of force wall "links" associated with this field generator
var bool bDidShutDownDestroy;	// recursion lockout

//-----------------------------------------------------------------------------

replication
{
	reliable if( bNetDirty && ROLE == ROLE_Authority )
		AvailableLinks; // data replicated to client
}

//-----------------------------------------------------------------------------

function Initialize( bool AltActivate )
{
	Super.Initialize( AltActivate );

	if( !AltActivate )
		AvailableLinks = NumPriLinks;	
	else
		AvailableLinks = NumAltLinks;

	FGLinks.Length = AvailableLinks;
	NumLinks = 0;
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	//DMTN( "Destroyed" );
	
	// make sure force field generator cleaned up if field generators destroyed (e.g. with killactor)
	if( !bDidShutDownDestroy )
		ShutDownDeployedUnit( true );
	
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

function interface string GetDescription( Controller User )
{
	return default.Description $ " (" $ Health $ ":" $ AvailableLinks $ ")";
}

//-----------------------------------------------------------------------------
// If this FG is linked to F, returns the link index, returns -1 otherwise.

function int GetLinkIndex( FieldGenerator F )
{
	local int i;

	for( i=0; i<AvailableLinks; i++ )
	{
		if( FGLinks[i].LinkFG == F )
			return i;
	}
	
	return -1;
}

//-----------------------------------------------------------------------------
// What's the first free link slot?

function int GetFirstFreeSlot()
{
	local int i;
	local int Retval;

	Retval = -1;
	for( i=0; i<AvailableLinks; i++ )
	{
		if( FGLinks[i].LinkFG == None )
		{
			Retval = i;
			break;
		}
	}
	return Retval;
}

//-----------------------------------------------------------------------------
// Handles the unlinking of the passed in slot and destroying the VerticalBeam
// if necessary.

function Unlink( int UnlinkSlot, bool bDestroyWall )
{
	// Unlinks the given slot
	FGLinks[UnlinkSlot].LinkFG = None;
	if( bDestroyWall && FGLinks[UnlinkSlot].LinkFW != None && !FGLinks[UnlinkSlot].LinkFW.bDeleteMe )
		FGLinks[UnlinkSlot].LinkFW.Destroy();
		
	PlaySound( UnlinkSound, SLOT_None );
	FGLInks[UnlinkSlot].LinkFW = None;
	AmbientSound = default.AmbientSound;
	NumLinks--;
}

//-----------------------------------------------------------------------------
// The FG is shutdown. All links are severed, the VerticalBeam is destroyed and,
// if given, a pickup version of the FG is spawned.

function ShutDownDeployedUnit( bool bDestroyed, optional Pawn P )
{
	local int i;

	//DMTN( "ShutDownDeployedUnit" );
	// Disconnect all links
	for( i=0; i<AvailableLinks; i++ )
	{
		if( FGLinks[i].LinkFG != None )
		{
			FGLinks[i].LinkFG.Unlink( FGLinks[i].LinkFG.GetLinkIndex( Self ), false );
			Unlink( i, true );
		}
	}

	bDidShutDownDestroy = true;
	Super.ShutDownDeployedUnit( bDestroyed, P );
}

//-----------------------------------------------------------------------------

function DisableUnit()
{
	local int i;

	// Disable all links
	for( i=0; i<AvailableLinks; i++ )
	{
		if( FGLinks[i].LinkFW != None && FGLinks[i].LinkFW.IsEnabled() )
			FGLinks[i].LinkFW.DisableUnit();
	}

	Super.DisableUnit();
}

//-----------------------------------------------------------------------------

function TakeDamage( int Damage, Pawn InstigatedBy, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	local int i;
	
	//DMTN( "TakeDamage  Links: " $ AvailableLinks );
	
	// damage must have been done to FG directly (ForceWall damage goes through ForceWallTakeDamage)
	Super.TakeDamage( Damage, InstigatedBy, HitLocation, Momentum, DamageType );
	
	if( !bDeleteMe )
	{
		// ping all associated forcewalls so they show damage effect
		for( i=0; i<AvailableLinks; i++ )
		{
			if( FGLinks[i].LinkFW != None  )
			{
				//DMTN( "  calling TookHit on " $ FGLinks[i].LinkFW );
				FGLinks[i].LinkFW.TookHit();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Damage from force wall.

function ForceWallTakeDamage( int Damage, Pawn InstigatedBy )
{
	//DMTN( "ForceWallTakeDamage" );
	// damage done to force wall -- call Super directly so we won't call TookHit above
	Super.TakeDamage( Damage, InstigatedBy, vect(0,0,0), vect(0,0,0), class'DamageTypePhysical' );
}

/*???
//-----------------------------------------------------------------------------

function Trigger( Actor Other, Pawn EventInstigator )
{
	OnUse( Other );
}
*/

//-----------------------------------------------------------------------------

function AddLink( FieldGenerator OtherFG, ForceWall FW, int LinkSlot )
{
	FGLinks[ LinkSlot ].LinkFG = OtherFG;
	FGLinks[ LinkSlot ].LinkFW = FW;
	PlaySound( LinkSound, SLOT_None );
	AmbientSound = class'ForceWall'.default.AmbientSound;
	NumLinks++;
}

//-----------------------------------------------------------------------------
// Spawn actor(s) associated with the force wall collision/display.

function ForceWall CreateForceWall( FieldGenerator StartFG, FieldGenerator EndFG )
{
	local ForceWall FW;
	
	FW = Spawn( class'ForceWall', , , (StartFG.Location + EndFG.Location)/2 );
	FW.Initialize( StartFG, EndFG, GetTeam() );
	
	return FW;
}

//-----------------------------------------------------------------------------
// Set's the passed in link, sets up creation of the ForceWallProxy, displays 
// effects, and plays sounds.

function AddLinks( FieldGenerator F, int LinkSlot )
{
	local ForceWall FW;
	
	// Create force field
	FW = CreateForceWall( Self, F );
	AddLink( F, FW, LinkSlot );
	F.AddLink( Self, FW, F.GetFirstFreeSlot() );
}

//-----------------------------------------------------------------------------
// This tests to see if the FG I am linked to has already created a 
// ForceWallProxy linking to me.

function int ForceWallIndex( FieldGenerator F )
{
	local int i;
	local int Retval;

	Retval = -1;
	for( i=0; i<AvailableLinks; i++ )
	{
		if( FGLinks[i].LinkFG == F )
		{
			if( FGLinks[i].LinkFW != None )
			{
				Retval = i;
				break;
			}
		}
	}

	return Retval;
}

//-----------------------------------------------------------------------------

function bool AnglesOK( FieldGenerator SourceFG, FieldGenerator TargetFG )
{
	local vector ExistingVector, PossibleVector;
	local bool bAngleOK;
	local int i;
			
	PossibleVector = TargetFG.Location - SourceFG.Location;
	PossibleVector.Z = 0;
	PossibleVector = Normal( PossibleVector );

	bAngleOK = true;
	for( i=0; i<SourceFG.AvailableLinks; i++ )
	{
		if( SourceFG.FGLinks[ i ].LinkFG != None )
		{
			ExistingVector = SourceFG.FGLinks[ i ].LinkFG.Location - SourceFG.Location;
			ExistingVector.Z = 0;
			ExistingVector = Normal( ExistingVector );
			//DM( "CosAngle: " $ ExistingVector dot PossibleVector );
			//DM( "Angle:    " $ Acos(ExistingVector dot PossibleVector) );
			if( (ExistingVector dot PossibleVector) > CosMinAngle )
			{
				bAngleOK = false;
				break;
			}
		}
	}
	
	//DMTN( "AnglesOK returning " $ bAngleOK );
	return bAngleOK;
}

//-----------------------------------------------------------------------------
// NOTE: we could possibly just try spawning the ForceWall and return true if
// that succeeds?

function bool TraceCheck( FieldGenerator TraceToFG )
{
	local vector HitLocation, HitNormal, Extent, StartLocation, EndLocation, EdgeVector;
	local Actor HitActor;

	Extent.X = CollisionRadius;
	Extent.Y = CollisionRadius;
	Extent.Z = class'ForceWall'.default.FieldHeight - TraceTolerance;

	EdgeVector = TraceToFG.Location - Location;
	EdgeVector.Z = 0;
	EdgeVector = Normal( EdgeVector );

	StartLocation = Location + CollisionRadius * EdgeVector;
	StartLocation.Z += class'ForceWall'.default.BottomOffset;
	EndLocation = TraceToFG.Location - TraceToFG.CollisionRadius * EdgeVector;
	EndLocation.Z += class'ForceWall'.default.BottomOffset;

	//HitActor = Trace( HitLocation, HitNormal, EndLocation, StartLocation, false, Extent, , TRACE_World | TRACE_Volumes | TRACE_StopAtFirstHit );
	HitActor = Trace( HitLocation, HitNormal, EndLocation, StartLocation, false, Extent );
	
	return( HitActor == None || 
			HitActor == TraceToFG || 
			(TraceToFG.AvailableLinks >= 1 && TraceToFG.FGLinks[0].LinkFW? && HitActor == TraceToFG.FGLinks[0].LinkFW) ||
			(TraceToFG.AvailableLinks >= 2 && TraceToFG.FGLinks[1].LinkFW? && HitActor == TraceToFG.FGLinks[1].LinkFW) );
		     //( VSize( HitLocation - TraceToFG.Location ) >= VSize( TraceToFG.Location - Location ) ) || 
			 //( FieldGenerator(HitActor) != None) );
}

//-----------------------------------------------------------------------------

function bool CanLinkTo( FieldGenerator FG )
{
	//DMTN( "CanLinkTo called for " $ FG );
	
	if( FG == Self )
	{
		//DMT( " Self" );
		return false;
	}
	
	if( !FG.bActive )
	{
		//DMT( " !bActive" );
		return false;
	}

	if( !FG.bEnabled )
	{
		//DMT( " !bEnabled" );
		return false;
	}

	if( FG.GetTeam() != GetTeam() )
	{
		//DMT( " different team" );
		return false;
	}

	// Check if FG has valid links before calling TestConnection
	if( FG.NumLinks >= FG.AvailableLinks )
	{
		//DMT( " free slots <=0: " $ FG.NumLinks );
		return false;
	}

	// Am I already linked to this FG?
	if( GetLinkIndex( FG ) != -1 )
	{
		//DMT( " already linked to this FG" );
		return false;
	}

	if( !TraceCheck( FG ) )
	{
		//DMT( " TraceCheck failed" );
		return false;
	}
	
	return true;
}

/*-----------------------------------------------------------------------------
Uses the ActorTypes list to search for FGs in range. Performs a few tests on
the FGs found and calls FindValidLink() to determine validity of the FG. If it
is found to be valid, ScanArea returns an FG, otherwise it returns None.
*/

function CheckForLinks()
{
	local int i, j;
	local Pawn P;
	local FieldGenerator FG;
	local array<FieldGenerator> SortedFGs;
	local int StartCandidate;
	local float FGDistance;

	// build a list of candidate FGs in the area sorted by increasing distance
	//foreach VisibleCollidingActors( class'FieldGenerator', FG, LinkRadius ) // FGs getting blocked by forcewalls?
	for( P=Level.PawnList; P!=None; P=P.NextPawn )
	{
		FG = FieldGenerator( P );
		//DMTN( "CheckForLinks considering " $ FG );
		if( FG == None )
			continue;

		FGDistance = VSize( FG.Location - Location );
		//DMTN( "  FGDistance: " $ FGDistance );
		if( FGDistance > LinkRadius )
			continue;
		
		if( CanLinkTo( FG ) )
		{
			// insert into array with closest first
			i = 0;
			while( i < SortedFGs.Length && VSize( SortedFGs[ i ].Location - Location ) < FGDistance )
				i++;

			// insert at i after moving everything else down
			SortedFGs.Length = SortedFGs.Length + 1;
			for( j=SortedFGs.Length-2; j>=i; j-- )
				SortedFGs[ j+1 ] = SortedFGs[ j ];
			SortedFGs[ i ] = FG;			
		}
	}

	//DMTN( "CheckForLinks found: " $ SortedFGs.Length $ " candidate FGS for " $ AvailableLinks $ " links" );
	
	// link to the closest candidate FGs which don't result in "bad" angles
	StartCandidate = 0;
	for( i=0; i<AvailableLinks; i++ )
	{
		if( FGLinks[ i ].LinkFG == None )
		{
			for( j=StartCandidate; j<SortedFGs.Length; j++ )
			{
				// make sure that adding this link won't create any "bad" angles (angles which are too acute)
				if( AnglesOK( Self, SortedFGs[ j ] ) && AnglesOK( SortedFGs[ j ], Self ) )
				{
					// add it
					AddLinks( SortedFGs[ j ], i );
					StartCandidate = j+1; // if checking other link slot, start with this one
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------

function CheckForEncroachingPawns()
{
	local Pawn P;
	
	for( P=Level.PawnList; P!=None; P=P.NextPawn )
	{
		if( VSize( P.Location - Location ) < (LinkRadius + 50.0) )
		{
			if( class'Util'.static.DoActorCylindersIntersect( Self, P ) )
				DamageEncroacher( P, Pawn(Owner) );
		}
	}
}

//-----------------------------------------------------------------------------

function NotifyDeployed()
{	
	//DMTN( "NotifyDeployed  Owner: " $ Owner );
	Super.NotifyDeployed();
	
	// temp hack so summoned field generators will link up
	if( AvailableLinks <= 0 )
	{
		SetTeam( 1 );
		Initialize( false );
	}
	
	CheckForLinks();

	//DumpFGs();
}

//-----------------------------------------------------------------------------

function DumpFGs()
{
	local Pawn P;
	local FieldGenerator FG;
	local int ii;
	
	DMT( "" );
	DMT( "==> DumpFGs" );
	for( P=Level.PawnList; P != None; P=P.NextPawn )
	{
		FG = FieldGenerator(P);
		if( FG != None && !FG.bDeleteMe )
		{
			FG.DMT( "  " $ FG $ "(up to " $ FG.AvailableLinks $ " links)" );
			for( ii=0; ii<FG.AvailableLinks; ii++ )
			{
				if( FG.FGLinks[ii].LinkFG != None )
				{
					FG.DMT( "    " $ FG.FGLinks[ii].LinkFG );
				}
			}
		}
	}	
	DMT( "" );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	// display
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPM.Items.FieldGenerator'
	AlternateSkins(0)=Shader'XMPT.Items.Metl_fieldgen_SD_001_Red'
	AlternateSkins(1)=Shader'XMPT.Items.Metl_fieldgen_SD_001_Blue'
	RepSkinIndex=0

	DestroyedAlertSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorDestroyedAlert'
	LinkSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorLink'
	UnlinkSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorUnlink'
	ShutdownSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorShutdown'
	DeploySound=Sound'U2XMPA.FieldGenerator.FieldGeneratorActivate'
	DisabledSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorDisabled'
	EnabledSound=Sound'U2XMPA.FieldGenerator.FieldGeneratorEnabled'
	ExplosionClass=class'ExplosionFieldGenerator'

	PickupMessage="You picked up a Field Generator."

	// physical
	CollisionHeight=70.000000
	CollisionRadius=16.00000
	DrawScale=0.110000
	PrePivot=(X=-10.000000,Y=-5.000000,Z=-20.000000)
	Health=300
	bUseCylinderCollision=true
	
	// AI
	ControllerClass=class'StationaryPawnControllerDefensive'

	InventoryType=class'FieldGeneratorDeployable'
	
	// configuration
	LinkRadius=1000
	CosMinAngle=0.707
	Description="Field Generator"
	TraceTolerance=16.000000
	NumPriLinks=2
	NumAltLinks=1
	bEnabled=true
}
