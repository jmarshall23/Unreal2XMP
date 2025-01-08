/*-----------------------------------------------------------------------------
Provides the collision and controls the visual effect for a force field (these
are always associated with 2 field generators, one on either side of the force
field).

Once faces/collision has been set up, checks for any Pawns which are encroached
by the blocking volume and encroaches these. Also does this any time the
collision is re-enabled after having been disabled (e.g. when the wall is 
used).
*/

class ForceWall extends DynamicBlockingVolume;

//-----------------------------------------------------------------------------

enum FWTransition
{
	FWT_None,
	FWT_Init,
	FWT_Disable,
	FWT_Enable,
	FWT_Hit,
};

//-----------------------------------------------------------------------------

// main configurable force wall properties
var localized string Description;
var float UsableDistance;
var int BeamCount;
var float FieldHeight, BottomOffset;
var Color DefaultColorNA, DefaultColorRed, DefaultColorBlue;
var Color DisabledColor, HitColor;
var float TransitionTime_Init;
var float TransitionTime_Disable;
var float TransitionTime_Enable;
var float TransitionTime_Hit;

var float DamageTransferPct;
var float EnergyCostPerSec;

// internal use
var Color StartColor, CurrentColor, DesiredColor;
var float ElapsedTransitionTime, TransitionTime;
var FieldGenerator StartFG, EndFG; // used on server only (no guarantee these will be set on client, e.g. because actors might not be relevant)
var vector StartFGLocation, EndFGLocation;
var float WallRadius, WallHeight;
var int TeamIndex;

var bool bBatchedDamageHandlerEnabled;
var float DamageAccumulator;
var Pawn LastInstigatedBy;
var	bool bEnabled;

var ProxyPawn			MyProxyPawn;
var	PulseLineGenerator	MyBeams;

var FWTransition PendingTransitionType;
//var bool bTransitionActive;
var int ServerNumTransitions, ClientNumTransitions;
var bool bServerCollision, bClientCollision;

//-----------------------------------------------------------------------------

replication
{
	// data to replicate to client
 	reliable if( bNetDirty && Role == ROLE_Authority )
		StartFGLocation, EndFGLocation, TeamIndex, PendingTransitionType, ServerNumTransitions, bServerCollision;
}

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	Super.PreBeginPlay();
	XMPGame(Level.Game).RegisterConsumer( Self, EnergyCostPerSec );
	//DMTN( "PreBeginPlay  StartFGLocation: " $ class'Util'.static.GetVectorString( StartFGLocation ) $ " EndFGLocation: " $ class'Util'.static.GetVectorString( EndFGLocation ) );
}

//-----------------------------------------------------------------------------

simulated event PostBeginPlay()
{
	//DMTN( "In PostBeginPlay  Role: " $ EnumStr( enum'ENetRole', Role ) );
	Super.PostBeginPlay();
	if( Role < ROLE_Authority )
		ReplicationReadyCheck();
}

//-----------------------------------------------------------------------------

simulated event Destroyed()
{
	if( MyBeams? )
	{
		MyBeams.Destroy();
		MyBeams= None;
	}
	
	if( Role == ROLE_Authority )
	{
		if( MyProxyPawn? )
		{
			MyProxyPawn.Destroy();
			MyProxyPawn = None;
		}
		
		XMPGame(Level.Game).UnRegisterConsumer( Self );
	}
			
	RemoveAllTimers(); // in case timer(s) somehow still active
	
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

simulated event PostNetReceive()
{
	// something was replicated to client ForceWall
	Super.PostNetReceive();

	//DMTN( "PostNetReceive  PendingTransitionType: " $ EnumStr( enum'FWTransition', PendingTransitionType ) );
	//if( PendingTransitionType != FWT_None )
	//if( !bTransitionActive && ServerNumTransitions != ClientNumTransitions )
	if( ServerNumTransitions != ClientNumTransitions )
	{
		StartTransition( PendingTransitionType );
		//PendingTransitionType = FWT_None;
		ClientNumTransitions = ServerNumTransitions;
		//bTransitionActive = true;
	}

	if(	bServerCollision != bClientCollision )
	{
		if( bServerCollision )
			ClientEnableCollision();
		else
			ClientDisableCollision();
		
		bClientCollision = bServerCollision;
	}
}

//-----------------------------------------------------------------------------

simulated function int GetTeam() { return TeamIndex; }

//-----------------------------------------------------------------------------

simulated function Color GetDefaultColor()
{
	if( TeamIndex == 0 )
		return DefaultColorRed;
	else if( TeamIndex == 1 )
		return DefaultColorBlue;
	else
		return DefaultColorNA;
}
	
//-----------------------------------------------------------------------------

function SetTeam( int NewTeamIndex )
{
	TeamIndex = NewTeamIndex;
	SetupTransition( FWT_Init );
}

//-----------------------------------------------------------------------------

function bool SameTeam( Pawn Other )
{
	return (TeamIndex == Other.GetTeam() );
}

//-----------------------------------------------------------------------------

function interface string GetDescription( Controller User )
{
	return default.Description; 
}

//-----------------------------------------------------------------------------

function bool IsEnabled() 
{ 
	return bEnabled; 
}

//-----------------------------------------------------------------------------

function SetEnabled( bool bVal ) 
{ 
	bEnabled = bVal; 
}

//-------------------------------------------------------------------------

simulated function string GetColorString( Color C )
{
	return "R=" $ C.R $ " G=" $ C.G $ " B=" $ C.B;
}

//-------------------------------------------------------------------------

simulated function Color BlendColors( Color C1, Color C2, float Percentage )
{
	local Color BlendedColor;

	BlendedColor.R = Blend( C1.R, C2.R, Percentage );
	BlendedColor.G = Blend( C1.G, C2.G, Percentage );
	BlendedColor.B = Blend( C1.B, C2.B, Percentage );
	BlendedColor.A = Blend( C1.A, C2.A, Percentage );

	return BlendedColor;
}

//-------------------------------------------------------------------------

simulated function SetColor( Color NewColor )
{
	if( MyBeams? )
	{
		MyBeams.BeamColor = NewColor;
		//MyBeams.BeamColor = MakeColor( 0,0,0,255 );
		MyBeams.SpriteJointColor = NewColor;
		//DMTN( "SetColor  Role: " $ EnumStr( enum'ENetRole', Role ) );
		//DMTN( "  NewColor:  " $ GetColorString( NewColor ) );
	}
	
	CurrentColor = NewColor;
}

//-------------------------------------------------------------------------

simulated function UpdateTransition( float DeltaTime )
{
	local float Percentage;

	//DMTN( "UpdateTransition" );
	
	ElapsedTransitionTime += DeltaTime;
	if( ElapsedTransitionTime >= TransitionTime )
	//if( CurrentColor == DesiredColor )	
	{
		EndTransition();
	}
	else
	{
		Percentage = ElapsedTransitionTime / TransitionTime;
		SetColor( BlendColors( StartColor, DesiredColor, Percentage ) );
	}
}

//-------------------------------------------------------------------------
// Called on server to set up a transition on the client. Simply sets the
// transition type (which is replicated to the client) and enables the
// client's tick function (to check for the arrival of the replicated
// transition type). Note: whenever the client detects a new transition
// type it sets it back to FWT_None so its possible to determine when
// this has been set again (and replicated).

simulated function StartTransition( FWTransition TransitionType )
{
	//DMTN( "StartTransition: " $ EnumStr( enum'FWTransition', TransitionType ) );
	//DMTN( "  IN  StartColor: " $ GetColorString( StartColor ) $ " DesiredColor: " $ GetColorString( DesiredColor ) $ " CurrentColor: " $ GetColorString( CurrentColor ) );

	ElapsedTransitionTime = 0.0;
	switch( TransitionType )
	{
	case FWT_Init:
		StartColor = DisabledColor;
		DesiredColor = GetDefaultColor();
		TransitionTime = TransitionTime_Init;
		break;
	case FWT_Disable:
		StartColor = CurrentColor;
		DesiredColor = DisabledColor;
		TransitionTime = TransitionTime_Disable;
		break;
	case FWT_Enable:
		StartColor = CurrentColor;
		DesiredColor = GetDefaultColor();
		TransitionTime = TransitionTime_Enable;
		break;
	case FWT_Hit:
		StartColor = HitColor;
		TransitionTime = TransitionTime_Hit;
		break;
	}
	
	if( StartColor != CurrentColor )
		SetColor( StartColor );

	//DMTN( "  OUT  StartColor: " $ GetColorString( StartColor ) $ " DesiredColor: " $ GetColorString( DesiredColor ) $ " CurrentColor: " $ GetColorString( CurrentColor ) );
	//DMTN( "  StartColor: " $ GetColorString( StartColor ) $ " DesiredColor: " $ GetColorString( DesiredColor ) $ " CurrentColor: " $ GetColorString( CurrentColor ) $ " over " $ TransitionTime $ " secs." );

	Enable( 'Tick' );
}

//-------------------------------------------------------------------------
// Called on server to set up a transition on the client. Simply sets the
// transition type (which is replicated to the client). Once this is
// replicated to the client, PostNetReceive should be called, telling the
// client to start the transition and enable the client tick function to
// process it.

function SetupTransition( FWTransition TransitionType )
{
	//DMTN( "SetupTransition: " $ EnumStr( enum'FWTransition', TransitionType ) $ " Role: " $ EnumStr( enum'ENetRole', Role ));
	//DMTN( "  IN  StartColor: " $ GetColorString( StartColor ) $ " DesiredColor: " $ GetColorString( DesiredColor ) $ " CurrentColor: " $ GetColorString( CurrentColor ) );
	if( Level.NetMode != NM_DedicatedServer )
	{
		//DMTN( "  calling StartTransition immediately" );
		StartTransition( TransitionType );
		PendingTransitionType = FWT_None;
	}
	else
	{
		//DMTN( "  setting up PendingTransitionType" );
		PendingTransitionType = TransitionType;
		ServerNumTransitions++; // force replication in case PendingTransitionType reset to same value?
	}
		
	//DMTN( "  final PendingTransitionType: " $ EnumStr( enum'FWTransition', PendingTransitionType ) );
}

//-------------------------------------------------------------------------

simulated function EndTransition()
{
	//DMTN( "EndTransition" );
	//bTransitionActive = false;
	Disable( 'Tick' );
	SetColor( DesiredColor );
}

//-------------------------------------------------------------------------

function EnableUnit()
{
	if( !bDeleteMe )
	{
		EnableCollision();
		SetupTransition( FWT_Enable );
		PlaySound( class'FieldGenerator'.default.EnabledSound, SLOT_None );
		SetEnabled( true );
	}
}

//-------------------------------------------------------------------------

function DisableUnit()
{
	if( !bDeleteMe )
	{
		DisableCollision();
		SetupTransition( FWT_Disable );
		PlaySound( class'FieldGenerator'.default.DisabledSound, SLOT_None );
		AddTimer( 'EnableUnit', class'FieldGenerator'.default.DisableTime, false );
		SetEnabled( false );
	}
}

//-----------------------------------------------------------------------------

function ForceWallKilled()
{
	//DMTN( "ForceWallKilled" );
	//DM( "*** ForceWallKilled - StartFG = " $ StartFG $ " EndFG = " $ EndFG );
	// Destroy the FieldGenerators that own this ForceWallProxy
	if( StartFG? )
	{
		StartFG.ShutdownDeployedUnit( true );
		StartFG.Explode();
		StartFG.Destroy();
	}
	if( EndFG? )
	{
		EndFG.ShutdownDeployedUnit( true );
		EndFG.Explode();
		EndFG.Destroy();
	}
}

//-----------------------------------------------------------------------------

function OnUse( Actor Other )
{
	//DMTN( "OnUse  Other: " $ Other );
	if( StartFG.IsUsable( Other ) )
		DisableUnit();
}

//-----------------------------------------------------------------------------

function bool CanHit( Controller AttackingController )
{
	return ( !SameTeam( AttackingController.Pawn ) );
}

//-------------------------------------------------------------------------

simulated event Tick( float DeltaTime )
{
	Super.Tick( DeltaTime );
	if( !bDeleteMe )
		UpdateTransition( DeltaTime );
}

//-----------------------------------------------------------------------------

function TookHit()
{
	if( !bDeleteMe )
		SetupTransition( FWT_Hit );
}

//-----------------------------------------------------------------------------

function BatchedDamageHandler()
{
	local int FGDamage;

	//DMTN( "BatchedDamageHandler  DamageAccumulator: " $ DamageAccumulator );
		
	bBatchedDamageHandlerEnabled = false;
	if( DamageAccumulator >= 1.0 )
	{
		FGDamage = FMax( 1, Round( DamageAccumulator*DamageTransferPct ) );

		// Deal a fraction of the damage to the connecting FieldGenerators
		// NOTE: StartFG and EndFG should always be set at this point on the server
		StartFG.ForceWallTakeDamage( FGDamage, LastInstigatedBy );
		EndFG.ForceWallTakeDamage( FGDamage, LastInstigatedBy );
			
		TookHit();
		DamageAccumulator = 0.0;
	}
}

//-----------------------------------------------------------------------------

function TakeDamage( int Damage, Pawn DamageInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType )
{
	//DMTN( "TakeDamage  Damage: " $ Damage $ " DamageType: " $ DamageType $ " Role: " $ EnumStr( enum'ENetRole', Role ) );

	if( !bDeleteMe )
	{	
		// "batch" damage during the current tick and process it asap in timer afterwards
		//(mdf): I'm not sure why this was done for the forcewalls but its not a bad idea if generalized
		DamageAccumulator += Damage;
		LastInstigatedBy = DamageInstigator; //track last Pawn to do damage for scoring purposes?

		if( !bBatchedDamageHandlerEnabled )
		{
			AddTimer( 'BatchedDamageHandler', 0.001, false );
			bBatchedDamageHandlerEnabled = true;
		}
	}
}

//-----------------------------------------------------------------------------

function DamageEncroacher( Pawn P )
{
	class'DeployedUnit'.static.DamageEncroacher( P, Pawn(Owner) );
}

//-----------------------------------------------------------------------------

function bool CheckIntersect( vector TestLocation, Pawn P )
{
	return class'Util'.static.DoCylindersIntersect( 
				TestLocation, 
				WallRadius, 
				WallHeight, 
				P.Location, 
				P.CollisionRadius, 
				P.CollisionHeight );
}

//-----------------------------------------------------------------------------

function CheckForEncroachingPawns()
{
	local Pawn P;
	local vector WallVector, TestLocation;
	local float BlockerDistance;
	local bool bEncroach;
	local float WallLen, TotalLen;
	
	//DMTN( "CheckForEncroachingPawns  StartFG: " $ StartFG $ "  EndFG: " $ EndFG );
	
	for( P=Level.PawnList; P!=None; P=P.NextPawn )
	{
		//DMTN( "  checking " $ P );
		if( P == StartFG || P == EndFG || P == MyProxyPawn )
			continue;
			
		// quick check whether anywhere near force field
		bEncroach = false;
		BlockerDistance = VSize( P.Location - Location );
		if( BlockerDistance < (0.5*class'FieldGenerator'.default.LinkRadius + P.CollisionRadius) )
		{
			// check every N units along wall from start to end location
			WallVector = EndFGLocation - StartFGLocation;
			WallLen = VSize( WallVector );
			WallVector = Normal( WallVector );
			
			TotalLen = 0.0;
			TestLocation = StartFGLocation;
			while( TotalLen < WallLen )
			{
				//AddCylinder( TestLocation, WallRadius, WallHeight, ColorGreen() );
				if( CheckIntersect( TestLocation, P ) )
				{
					bEncroach = true;
					break;
				}
				TestLocation += 2*WallRadius*WallVector;
				TotalLen += 2*WallRadius;
			}				
		
			if( bEncroach )
			{
				//DMTN( "  wall encroaches " $ P );
				//AddActor( P, ColorBlue() );
				DamageEncroacher( P );
			}
		}
	}
}

//-----------------------------------------------------------------------------

event bool EncroachingOn( Actor Other )
{
	DamageEncroacher( Pawn(Other) );
	return false;
}

//-------------------------------------------------------------------------

function EnableCollision()
{
	SetCollision( true, true, true );
	bServerCollision = true;
	
	if( ROLE == ROLE_Authority )
		CheckForEncroachingPawns();
}

//-------------------------------------------------------------------------

function DisableCollision()
{
	SetCollision( false, false, false );
	bServerCollision = false;
}

//-------------------------------------------------------------------------

simulated function ClientEnableCollision()
{
	SetCollision( true, true, true );
}

//-------------------------------------------------------------------------

simulated function ClientDisableCollision()
{
	SetCollision( false, false, false );
}

//-----------------------------------------------------------------------------

simulated function CreateVisualEffect()
{
	local float BeamSpacing;
	local float BeamVertIncrement;
	local int	i;

	//DMTN( "CreateVisualEffect  Role: " $ EnumStr( enum'ENetRole', Role ) );

	if( Level.NetMode == NM_DedicatedServer )
		return;
		
	BeamSpacing = (2*FieldHeight) / (BeamCount-1);
	MyBeams = Spawn( class'ForceWallBeam' );
	for( i=0; i<BeamCount; i++ )
	{
		BeamVertIncrement = -1*FieldHeight + BottomOffset + i*BeamSpacing;
		MyBeams.AddConnection( StartFGLocation + vect(0,0,1)*BeamVertIncrement, EndFGLocation + vect(0,0,1)*BeamVertIncrement );
	}
	MyBeams.Trigger( Self, None );
	
	//DMTN( "  calling SetColor DefaultColor: " $ GetColorString( GetDefaultColor() ) );
	SetColor( GetDefaultColor() );
}

//-----------------------------------------------------------------------------

simulated function CreateCollision()
{
	local vector StartLocation, EndLocation, WallVector;
	local vector StartLocation3D, EndLocation3D;
	local vector WallWidthVector;
	local vector PA1, PA2, PA3, PA4, PB1, PB2, PB3, PB4;
	local array<Face> Faces;

	//DMTN( "CreateCollision  Role: " $ EnumStr( enum'ENetRole', Role ) );

	WallVector = Normal( EndFGLocation - StartFGLocation );

	// build wall partially overlapping with endpoints
	StartLocation = StartFGLocation + vect(0,0,1)*BottomOffset + WallRadius*WallVector;
	EndLocation = EndFGLocation + vect(0,0,1)*BottomOffset - 0.5*WallRadius*WallVector;

	// now adjust the size & update collision hash
	WallWidthVector = WallVector cross vect(0,0,1);
	StartLocation3D = StartLocation - WallWidthVector * WallRadius + vect(0,0,1)*WallHeight;
	EndLocation3D	= EndLocation + WallWidthVector * WallRadius - vect(0,0,1)*WallHeight;

	// set up faces for blocking volume
	PA1 = StartLocation3D;
	PA2 = EndLocation3D		- WallWidthVector * 2.0*WallRadius + 2.0*vect(0,0,1)*WallHeight;
	PA3 = EndLocation3D		- WallWidthVector * 2.0*WallRadius;
	PA4 = StartLocation3D	- 2.0*vect(0,0,1)*WallHeight;
	PB1 = EndLocation3D		+ 2.0*vect(0,0,1)*WallHeight;
	PB2 = StartLocation3D	+ WallWidthVector * 2.0*WallRadius;
	PB3 = StartLocation3D	+ WallWidthVector * 2.0*WallRadius - 2.0*vect(0,0,1)*WallHeight;
	PB4 = EndLocation3D;
	
	Faces.Length = 6;
	Faces[ 0 ].Points.Length = 4;
	Faces[ 0 ].Points[ 0 ] = PA1;
	Faces[ 0 ].Points[ 1 ] = PA2;
	Faces[ 0 ].Points[ 2 ] = PA3;
	Faces[ 0 ].Points[ 3 ] = PA4;

	Faces[ 1 ].Points.Length = 4;
	Faces[ 1 ].Points[ 0 ] = PB1;
	Faces[ 1 ].Points[ 1 ] = PB2;
	Faces[ 1 ].Points[ 2 ] = PB3;
	Faces[ 1 ].Points[ 3 ] = PB4;

	Faces[ 2 ].Points.Length = 4;
	Faces[ 2 ].Points[ 0 ] = PA1;
	Faces[ 2 ].Points[ 1 ] = PA4;
	Faces[ 2 ].Points[ 2 ] = PB3;
	Faces[ 2 ].Points[ 3 ] = PB2;

	Faces[ 3 ].Points.Length = 4;
	Faces[ 3 ].Points[ 0 ] = PB1;
	Faces[ 3 ].Points[ 1 ] = PB4;
	Faces[ 3 ].Points[ 2 ] = PA3;
	Faces[ 3 ].Points[ 3 ] = PA2;

	Faces[ 4 ].Points.Length = 4;
	Faces[ 4 ].Points[ 0 ] = PA1;
	Faces[ 4 ].Points[ 1 ] = PB2;
	Faces[ 4 ].Points[ 2 ] = PB1;
	Faces[ 4 ].Points[ 3 ] = PA2;

	Faces[ 5 ].Points.Length = 4;
	Faces[ 5 ].Points[ 0 ] = PA3;
	Faces[ 5 ].Points[ 1 ] = PB4;
	Faces[ 5 ].Points[ 2 ] = PB3;
	Faces[ 5 ].Points[ 3 ] = PA4;
	
	SetFaces( Faces );
	
	EnableCollision();

	/*
	// debug
	//SetFaces( Faces );
	//DrawFaces( Faces, ColorRed() );
	//AddCylinder( StartLocation3D, 8, 8, ColorGreen() );
	//AddCylinder( EndLocation3D, 8, 8, ColorRed() );
	//AddArrow( StartLocation3D, EndLocation3D, ColorRed() );
	*/
}

//-----------------------------------------------------------------------------

function Initialize( FieldGenerator _StartFG, FieldGenerator _EndFG, int _TeamIndex )
{	
	StartFG			= _StartFG;
	EndFG			= _EndFG;
	StartFGLocation	= StartFG.Location;
	EndFGLocation	= EndFG.Location;
	SetTeam( _TeamIndex );
	
	SetEnabled( true );

	CreateCollision();	
	
	if( Role == ROLE_Authority )
	{
		// create proxy Pawn for AI to aim at
		MyProxyPawn = Spawn( class'ProxyPawn' );
		MyProxyPawn.Initialize( 2.0*WallRadius, WallHeight );
		MyProxyPawn.SetTeam( TeamIndex );
	}
	
	if( Level.NetMode != NM_DedicatedServer )
		CreateVisualEffect();
}

//-----------------------------------------------------------------------------

simulated function SetupClient()
{
	//DMTN( "SetupClient" );
	CreateVisualEffect();
	CreateCollision();
}

//-----------------------------------------------------------------------------

simulated function bool ReplicationReady()
{
	return ( StartFGLocation != default.StartFGLocation && EndFGLocation != default.EndFGLocation );
}

//-----------------------------------------------------------------------------

simulated function ReplicationReadyCheck()
{
	//DMTNS( "ReplicationReadyCheck  Role: " $ EnumStr( enum'ENetRole', Role ) );
		
	// hack: assume replication complete once changes from defaults
	if( ReplicationReady() )
		SetupClient();
	else
		AddTimer( 'ReplicationReadyCheck', 0.2, false );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	Texture=Texture'DeveloperT.Special.TypeMask_Plasma' //NOTE: used for impact effects
	Description="Disable Force Wall"
	bUsable=true
	UsableDistance=70.000000
	bProjTarget=true
	bBlockZeroExtentTraces=true
	SoundRadius=60
	SoundVolume=255
	AmbientSound=Sound'U2XMPA.FieldGenerator.ForceWallAmbient'
	bCanBeBaseForPawns=false
	TeamIndex=-1
	StartFGLocation=(X=-99999999,Y=-99999999,Z=-99999999)
	EndFGLocation=(X=-99999999,Y=-99999999,Z=-99999999)
	FieldHeight=60.0
	BottomOffset=10.0
	BeamCount=6
	WallRadius=8
	WallHeight=70
	
	DefaultColorNA=(R=255,G=255,B=0,A=255)  // uninitialized
	DefaultColorRed=(R=255,G=0,B=0,A=255)
	DefaultColorBlue=(R=0,G=0,B=255,A=255)
	DisabledColor=(R=0,G=255,B=0,A=255)
	HitColor=(R=255,G=255,B=255,A=255)
	
	TransitionTime_Init=0.5
	TransitionTime_Disable=0.5
	TransitionTime_Enable=0.5
	TransitionTime_Hit=0.5
	DamageTransferPct=0.333333
	EnergyCostPerSec=0.0010
	
	//networking
	RemoteRole=ROLE_SimulatedProxy //mainly so simulated functions work on client?
	bAlwaysRelevant=true //like vehicles, these are too large to work with usual relevancy rules
	bNetNotify=true //enable PostNetReceive events
}