//=============================================================================
// IconNode.uc
//=============================================================================

class IconNode extends BreakableItem
	native
	placeable;

//-----------------------------------------------------------------------------

const MaxHolders = 4;

//-----------------------------------------------------------------------------

struct native HolderInfoT
{
	var() class<ArtifactHolder> Class;
	var() bool bHasArtifact;
};

//-----------------------------------------------------------------------------

var() int TeamIndex;							// Team who base this belongs to
var() HolderInfoT HolderInfo[MaxHolders];		// for properly initializing artifact holders
var ArtifactHolder Holders[MaxHolders];			// references to the artifact holders
var vector HolderLocOffsets[MaxHolders];		// 3d offsets used to place the holders
var rotator HolderRotOffsets[MaxHolders];		// orientations to use for placing holders

var() float TouchingHeight;						// Half height of registration pad
var() float TouchingRadius;						// Half width of registration pad
var CollisionProxy TouchingCollisionProxy;		// for running through and registering artifacts

var array<StaticMesh> ExtraStaticMeshes;		// list of meshes attached to this actor
var array<vector> ExtraMeshLocations;			// relative locations of these meshes

var sound ArtifactStolenSound;
var sound ArtifactDeniedSound;
var sound IconNodeShutdownSound;
var sound IconNodeActivateSound;

var bool bShutdown;								// unable to store artifacts

//-----------------------------------------------------------------------------

cpptext
{
	virtual void PostBeginPlay();
	void SetupExtraMeshes();
}

//-----------------------------------------------------------------------------

event PreBeginPlay()
{
	Super.PreBeginPlay();
	TouchingCollisionProxy = CreateCollisionProxy( TouchingRadius, TouchingHeight, Location );
	SetTeam( GetTeam() );		// init team skins
}

//-----------------------------------------------------------------------------

event PostBeginPlay()
{
	Super.PostBeginPlay();
	CreateHolders();
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	CleanupCollisionProxy( TouchingCollisionProxy );
	CleanupHolders();
	Super.Destroyed();
}

//-----------------------------------------------------------------------------
// Artifact pick / drop off area (touch cylinder)

function CollisionProxy CreateCollisionProxy( float Radius, float Height, vector Location )
{
	local CollisionProxy CP;
	CP = Spawn( class'CollisionProxy' );
	CP.CP_SetCollision( true, false, false );
	CP.CP_SetLocation( Location );
	CP.CP_SetCollisionSize( Radius, Height );
	CP.CP_SetTouchTarget( Self );
	CP.SetBase( Self );
	return CP;
}

//-----------------------------------------------------------------------------

function CleanupCollisionProxy( CollisionProxy CP )
{
	if( CP != None )
	{
		CP.Destroy();
		CP = None;
	}
}

//-----------------------------------------------------------------------------
// holders for artifacts

function CreateHolders()
{
	local int i, NewId;
	local vector X,Y,Z;
	local ArtifactHolder AH;

	if( GetTeam() == 255 )
		Warn( "IconNode:" @ Self @ "has not been assigned to a team");

	GetAxes(Rotation,X,Y,Z);

	for( i=0; i < ArrayCount(Holders); i++ )
	{
		if( HolderInfo[i].Class != None )
			AH = Spawn( HolderInfo[i].Class );

		if( AH != None )
		{
			AH.IconNode = Self;
			AH.SetTeam( GetTeam() );
			AH.SetRotation( Rotation + HolderRotOffsets[i] );
			AH.SetLocation( Location + HolderLocOffsets[i].X * X
									 + HolderLocOffsets[i].Y * Y
									 + HolderLocOffsets[i].Z * Z );
			if( HolderInfo[i].bHasArtifact )
			{
				NewId = XMPGame( Level.Game ).GetArtifactID( AH );
				AH.Store( NewId );
				//AH.DMN("contains Artifact #:" @ AH.ArtifactID );
			}
			Holders[i] = AH;
		}
	}
}

//-----------------------------------------------------------------------------

function CleanupHolders()
{
	local int i;
	for( i=0; i < ArrayCount(Holders); i++ )
	{
		Holders[i].Destroy();
		Holders[i] = None;
	}
}

//-----------------------------------------------------------------------------

simulated function int GetTeam() { return TeamIndex; }
function SetTeam( int NewTeam )
{
	TeamIndex = NewTeam;
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
}

//-----------------------------------------------------------------------------
// Gameplay / Scoring
//-----------------------------------------------------------------------------

event Touch( Actor Other )
{
	local U2Pawn P;
	P = U2Pawn(Other);
	if( class'Pawn'.static.ValidPawn(P) )
	{
		if( Other.GetTeam() != GetTeam() )
			TryToStealArtifact( P );
		else
		{
			if( XMPGame(Level.Game).IsTeamFeatureOnline( Class.Name, GetTeam() ) )
				TryToStoreArtifactFrom( P );
			else
				PlaySound( ArtifactDeniedSound );
		}
	}
}

//-----------------------------------------------------------------------------

function bool CarryingArtifact( Pawn P )
{
	local Inventory Inv;
	for( Inv=P.Inventory; Inv!=None; Inv=Inv.Inventory )
		if( Artifact(Inv) != None )
			return true;
	return false;
}

//-----------------------------------------------------------------------------

function TryToStealArtifact( Pawn Stealer )
{
	local int i, ArtifactID;
	local Controller C;

	if( CarryingArtifact( Stealer ) )
	{
		//DM("Can't steal an artifact while already carrying one");
		return;
	}

	for( i=0; i < ArrayCount(Holders); i++ )
	{
		if( Holders[i].IsArtifactStored() )
		{
			ArtifactID = Holders[i].ArtifactStolen( Stealer );
			XMPGame(Level.Game).NotifyArtifactStolen( ArtifactID, Stealer );
			for( C=Level.ControllerList; C!=None; C=C.nextController )
				if( C.IsRealPlayer() && C.GetTeam() == GetTeam() )
				{
                    Level.Game.BroadcastLocalizedTeam( C, class'ArtifactStolenMessage');
					break;
				}
			PlaySound( ArtifactStolenSound );
			return;
		}
	}

	//DM("No Artifacts left to steal");
	PlaySound( ArtifactDeniedSound );
}

//-----------------------------------------------------------------------------

function TryToStoreArtifactFrom( Pawn Other )
{
	local Artifact A;
	local Inventory Inv;

	for( Inv=Other.Inventory; Inv!=None; Inv=Inv.Inventory )
	{
		A = Artifact(Inv);
		if( A != None )
		{
			if( StoreArtifact( Other, A.ID, A.Class ) )
			{
				Other.DeleteInventory(A);
				A.Destroy();
			}
			else
				Warn("Unable to find a holder to store artifact #" @ A.ID @ "carrier:" @ Other );
		}
	}
}

//-----------------------------------------------------------------------------

function bool SentHome( int ArtifactId )
{
	local int i;
	local Controller C;

	for( i = 0; i < ArrayCount(Holders); i++ )
		if( Holders[i].ArtifactID == ArtifactId )
		{
			// occasionally the artifact is already stored, store it anyway rather than break the game
			if( Holders[i].IsArtifactStored() )
				Warn("Artifact #" @ ArtifactID @ "is already stored in the icon node" );

			Holders[i].Store( ArtifactID );
			for( C=Level.ControllerList; C!=None; C=C.nextController )
				if( C.IsRealPlayer() && C.GetTeam() == GetTeam() )
				{
                    Level.Game.BroadcastLocalizedTeam( C, class'ArtifactReturnedMessage');
					break;
				}
			return true;
		}

	Warn("Unable to find an artifact holder for artifact ID:" @ ArtifactId );
	for( i = 0; i < ArrayCount(Holders); i++ )
		DMTN("Holder:" @ i @ "=" @ Holders[i] @ "ID:" @ Holders[i].ArtifactID );
	return false;
}

//-----------------------------------------------------------------------------

function bool StoreArtifact( Pawn Other, int ArtifactID, class<Artifact> ArtifactClass )
{
	local int i;

	for( i = 0; i < ArrayCount(Holders); i++ )
	{
		if( Holders[i].CanBeStored( ArtifactClass, ArtifactID ) )
		{
			Holders[i].Store( ArtifactID );
			XMPGame( Level.Game ).NotifyArtifactStored( ArtifactID, Other );
			Level.Game.BroadcastLocalizedTeam( Other.Controller, class'ArtifactStoredMessage');
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------

function bool Contains( int ArtifactID )
{
	local int i;
	for( i = 0; i < ArrayCount(Holders); i++ )
		if( Holders[i].ArtifactID == ArtifactID )
			return true;
	return false;
}

//-----------------------------------------------------------------------------

event TakeDamage( int Damage, Pawn EventInstigator, vector HitLocation, vector Momentum, class<DamageType> DamageType)
{
}

//-----------------------------------------------------------------------------

function NotifyActivate()
{
	PlaySound( IconNodeActivateSound );
	bShutdown = false;
}

//-----------------------------------------------------------------------------

function NotifyShutdown()
{
	PlaySound( IconNodeShutdownSound );
	bShutdown = true;
}

//-----------------------------------------------------------------------------

interface function float   GetUsePercent( Controller User ) { return 0; }
interface function bool        HasUseBar( Controller User ) { return false; }
interface function string GetDescription( Controller User ) { return ""; }
interface function int   GetUseIconIndex( Controller User ) { return 0; }

//-----------------------------------------------------------------------------

function DumpArtifacts()
{
	local int i;
	DMTN("Team:" @ GetTeam() );
	for( i = 0; i < ArrayCount(Holders); i++ )
		DM("Holder:" @ Holders[i] @ "ArtifactId:" @ Holders[i].ArtifactId @ "IsArtifactStored?:" @ Holders[i].IsArtifactStored() );
}

//-----------------------------------------------------------------------------

defaultproperties
{
	Description="Icon Node"
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'XMPWorldItemsM.Base.ArtifactMachineBase'
	bStatic=false
	bBlockKarma=true
	bBlockNonZeroExtentTraces=true
	TeamIndex=255
	TouchingHeight=40
	TouchingRadius=75

	AlternateSkins(0)=Shader'XMPWorldItemsT.Base.Poly_Pulse_Red'
	AlternateSkins(1)=Shader'XMPWorldItemsT.Base.Poly_Pulse_Blue'
	RepSkinIndex=1

	HolderLocOffsets(0)=(X=243,Y=171,Z=85)
	HolderLocOffsets(1)=(X=-243,Y=171,Z=85)
	HolderLocOffsets(2)=(X=-243,Y=-171,Z=85)
	HolderLocOffsets(3)=(X=243,Y=-171,Z=85)

	HolderRotOffsets(0)=(Yaw=-11000)
	HolderRotOffsets(1)=(Yaw=11000)
	HolderRotOffsets(2)=(Yaw=21768)
	HolderRotOffsets(3)=(Yaw=43768)

	ExtraStaticMeshes[0]=StaticMesh'XMPWorldItemsM.Base.ArtifactMachineTop'
	ExtraStaticMeshes[1]=StaticMesh'XMPWorldItemsM.Base.ArtifactMachineUpper'

	IconNodeShutdownSound=sound'U2A.Artifacts.IconNodeShutdown'
	IconNodeActivateSound=sound'U2A.Artifacts.IconNodeActivate'
	ArtifactStolenSound=sound'U2A.Artifacts.ArtifactStolen'
	ArtifactDeniedSound=sound'U2A.Artifacts.ArtifactStoreFailed'
	AmbientSound=sound'U2A.Artifacts.IconNodeAmbient'
}
