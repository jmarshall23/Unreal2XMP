//=============================================================================
// UseProxy.uc
// $Author$
// $Date$
// $Revision$
//=============================================================================

class UseProxy extends Actor
	placeable;

//-----------------------------------------------------------------------------

var() Actor UseMaster;

//-----------------------------------------------------------------------------

replication
{
	reliable if ( bNetDirty && Role==ROLE_Authority )
		UseMaster;
}

//-----------------------------------------------------------------------------

event PostBeginPlay()
{
	Super.PostBeginPlay();
	if( UseMaster? )
	{
		UseMaster.NetDependents.Insert(0,1);
		UseMaster.NetDependents[0] = Self;
	}
	else
		Warn(Self @ "was not given a use master - must be fixed in the editor");
}

//-----------------------------------------------------------------------------

event Destroyed()
{
	local int i;
	if( UseMaster? )
	{
		for( i=UseMaster.NetDependents.Length-1; i >= 0; i-- )
			if( UseMaster.NetDependents[i] == Self )
				UseMaster.NetDependents.Remove(i,1);
	}
	Super.Destroyed();
}

//-----------------------------------------------------------------------------

function OnUse( Actor Other )
{
	if( UseMaster != None )
		UseMaster.OnUse( Other );
}

//-----------------------------------------------------------------------------

function OnUnuse( Actor Other )
{
	if( UseMaster != None )
		UseMaster.OnUnuse( Other );
}

//-----------------------------------------------------------------------------

function SetTeam( int NewTeam )
{
	class'UtilGame'.static.SetTeamSkin( Self, NewTeam );
}

//-----------------------------------------------------------------------------

interface function bool   HasUseBar		 ( Controller User ) { if( UseMaster? ) return UseMaster.HasUseBar(User);		else { DMN("Use Proxy has no UseMaster"); return Super.HasUseBar(User); } }
interface function float  GetUsePercent  ( Controller User ) { if( UseMaster? ) return UseMaster.GetUsePercent(User);	else { return Super.GetUsePercent(User);  } }
interface function string GetDescription ( Controller User ) { if( UseMaster? ) return UseMaster.GetDescription(User);	else { return Super.GetDescription(User); } }
interface function int    GetUseIconIndex( Controller User ) { if( UseMaster? ) return UseMaster.GetUseIconIndex(User); else { return Super.GetUseIconIndex(User); } }

//-----------------------------------------------------------------------------

defaultproperties
{
	DrawType=DT_StaticMesh
	StaticMesh=StaticMesh'TexPropSphere'
	bStaticLighting=true
	bShadowCast=true
	bUsable=true
	bCollideActors=true
	bBlockActors=true
	bProjTarget=true
	bBlockNonZeroExtentTraces=true
}
