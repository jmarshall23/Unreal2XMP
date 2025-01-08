//=============================================================================
// $Author: Aleiby $
// $Date: 2/25/02 8:19p $
// $Revision: 2 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	LineManager.uc
// Author:	Aaron R Leiby
// Date:	22 August 2000
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//
// + From UnrealScript:
//
//     class'Singleton'.static.GetInstance( Level, class'LineManager', Name ).AddLinePS( ... );
//     -- Name is optional.  Use it if you want a unique set of lines for your own use.  Any name will do.
//
// + From C++:
//
//     USingleton::StaticGetInstance( GetLevel(), "ParticleSystems.LineManager", Name )->AddLinePS( ... );
//
// + From C++ (in non-ParticleSystems dependant packages):
//
//     struct LMParms
//     {
//         FVector	Start
//         FVector	End
//         FVvector	Color		
//         UBOOL	bDirected	
//         FLOAT	ArrowOffset	
//         FLOAT	ArrowWidth	
//         FLOAT	ArrowHeight	
//         FVector	ArrowColor	
//     };
//
//     LMParms Parms;
//     Parms.Start = ...;	// Fill in all parameters.
//     UObject* LM=USingleton::StaticGetInstance( GetLevel(), "ParticleSystems.LineManager", Name );
//     LM->UObject::ProcessEvent( LM->FindFunction( FName(TEXT("AddLinePS"),FNAME_Intrinsic)), Parms );
//
// + From C++ (in UnrealEd):
//
//     UBOOL GIsScriptableSaved=1;
//     Exchange(GIsScriptable,GIsScriptableSaved);
//     LMParms Parms;
//     Parms.Start = ...;	// Fill in all parameters.
//     UObject* LM=USingleton::StaticGetInstance( GetLevel(), "ParticleSystems.LineManager", Name );
//     LM->UObject::ProcessEvent( LM->FindFunction( FName(TEXT("AddLinePS"),FNAME_Intrinsic)), Parms );
//     Exchange(GIsScriptable,GIsScriptableSaved);
//
// + class'Singleton'.static.GetInstance( Level, class'LineManager', Name ).Clean();
//   -- This will clear all existing lines for this instance.
//
// + Use bHidden to toggle lines on/off.
// + Set bClip to false if you want lines to show through geometry -- this may not currently work in all renderers.
//------------------------------------------------------------------------------

class LineManager extends ParticleGenerator
	native;

//------------------------------------------------------------------------------
native event AddLinePS
	(				vector				Start
	,				vector				End
	,	optional	actor.color			Color			// RGBA of line.
	,	optional	bool				bDirected		// Should we draw an arrowhead?
	,	optional	float				ArrowOffset		// Distance of the base of the arrowhead from the StartPoint.
	,	optional	float				ArrowWidth		// Width of base of arrowhead.
	,	optional	float				ArrowHeight		// Height of arrowhead -- from base of arrowhead.
	,	optional	actor.color			ArrowColor		// RGBA of arrowhead.
	);

function DefineDependants()
{
	local class Dependant;
	Dependant = class'ParticleSystems.LineManagerTemplate';
}


defaultproperties
{
	CullDistance=0.000000
}

