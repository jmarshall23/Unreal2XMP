//=============================================================================
// $Author: Mfox $
// $Date: 4/30/02 3:39p $
// $Revision: 4 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	FT_Light.uc
// Author:	Aaron R Leiby
// Date:	25 January 2001
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class: 
//------------------------------------------------------------------------------

class FT_Light extends Light;

//var() float DelayTime;
var() vector Offset;
//var() float FadeTime;

var float Timer;

//------------------------------------------------------------------------------
event Tick( float DeltaTime )
{
//	local vector DesiredLocation;
//	local vector Delta;

//	Super.Tick( DeltaTime );

	if( Timer > 0 )
	{
		Timer -= DeltaTime;
		if( Timer <= 0 )
		{
			Timer = 0;
			TurnOff();
			return;
		}
	}
/*
	if( FadeTime>0 )
	{
		FadeTime -= DeltaTime;
		LightBrightness = default.LightBrightness * (FadeTime / default.FadeTime);
		if( FadeTime<=0 )
			LightType = LT_None;
		LightTag++;
	}
*/
	if( LightType != LT_None )
	{
//		DeltaTime = FMin( DeltaTime, DelayTime );
//		DesiredLocation = Owner.Location + (Offset >> Owner.Rotation);
//		Delta = (DesiredLocation - Location) * (DeltaTime / DelayTime);
//		SetLocation( Location + Delta );
		// mdf/ARL-warning fix
		if( Owner != None )
			SetLocation( Owner.Location + (Offset >> Owner.Rotation) );
	}
}

//------------------------------------------------------------------------------
function Trigger( actor Other, pawn Instigator )
{
	TurnOn();
	Timer = 0.2;
}

//------------------------------------------------------------------------------
function TurnOn()
{
	LightType = LT_Steady;
//	LightType = default.LightType;
//	LightBrightness = default.LightBrightness;
//	FadeTime = 0;
//	LightTag++;
}

//------------------------------------------------------------------------------
function TurnOff()
{
	LightType = LT_None;
//	FadeTime = default.FadeTime;
//	LightTag++;
}

defaultproperties
{
//	DelayTime=1.000000
//	FadeTime=1.000000
	RemoteRole=ROLE_None
	bStatic=false
	bStasis=false
	bNoDelete=false
	bMovable=true
//	bHidden=false
	LightBrightness=255
	LightHue=32
	LightSaturation=80
	LightRadius=12
	bDynamicLight=true
}

