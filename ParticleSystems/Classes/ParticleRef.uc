//=============================================================================
// $Author: Mfox $
// $Date: 4/30/02 12:27p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleRef.uc
// Author:	Aaron R Leiby
// Date:	18 October 2000
//------------------------------------------------------------------------------
// Description:	Used to place references to other particle systems in a level.
// This way you can have a single map that houses all your particle systems,
// and place them in other maps via this.  But since they'll always be getting
// loaded automatically from the original map, then if you change the original
// template particle system, all the references will get automatically updated.
//------------------------------------------------------------------------------
// How to use this class:
// + Place in level.
// + Set Reference string.
// + Set the type appropriately.
//------------------------------------------------------------------------------

class ParticleRef extends Actor
	placeable
	native;

var() string Reference;
var() class<ParticleGenerator> Type;
var() bool bPreLoad;
var() bool bReset;

var() ParticleGenerator Ref;

//------------------------------------------------------------------------------
simulated event PreBeginPlay()
{
	if( bPreLoad ) SpawnRef();
	Super.PreBeginPlay();
}

//------------------------------------------------------------------------------
event SpawnRef()
{
	if( Ref==None && Reference!="" && Type!=None )
	{
		Ref = class'ParticleGenerator'.static.DynamicCreateNew( Self, Type, Reference, Location );
		Ref.SetRotation( Rotation );
//		Ref.SetBase( Self );
	}
}

//------------------------------------------------------------------------------
simulated event Destroyed()
{
	if( Ref!=None )
	{
		Ref.ParticleDestroy();
		Ref = None;
	}

	Super.Destroyed();
}


defaultproperties
{
	bHidden=true
	DrawType=DT_Custom
}

