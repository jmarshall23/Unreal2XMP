/*=============================================================================
	Engine.h: Unreal engine public header file.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#ifndef _INC_ENGINE
#define _INC_ENGINE

/*----------------------------------------------------------------------------
	API.
----------------------------------------------------------------------------*/

#ifndef ENGINE_API
	#define ENGINE_API DLL_IMPORT
#endif

/*-----------------------------------------------------------------------------
	Dependencies.
-----------------------------------------------------------------------------*/

#include "Core.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

ENGINE_API extern class FMemStack			GEngineMem;
ENGINE_API extern class FMemCache			GCache;
ENGINE_API extern class FStats				GStats;
ENGINE_API extern class FEngineStats		GEngineStats;
ENGINE_API extern class FMatineeTools		GMatineeTools;
ENGINE_API extern class UGlobalTempObjects* GGlobalTempObjects;
ENGINE_API extern class FStatGraph*			GStatGraph;
ENGINE_API extern class FTempLineBatcher*	GTempLineBatcher;
ENGINE_API extern FLOAT				GRealDeltaTime;				/* Real DeltaTime between last tick and this one */
ENGINE_API extern FLOAT				GGameDeltaTime;				/* Game-level DeltaTime between last tick and this one (handles SLOMO, etc) */

#define DECLARE_STATIC_UOBJECT( ObjectClass, ObjectName, ExtraJunk ) \
			static ObjectClass* ObjectName = NULL;	\
			if( !ObjectName )	\
			{ \
				ObjectName = ConstructObject<ObjectClass>(ObjectClass::StaticClass()); \
				GGlobalTempObjects->AddGlobalObject((UObject**)&ObjectName); \
				ExtraJunk; \
			}

/*-----------------------------------------------------------------------------
	Engine compiler specific includes.
-----------------------------------------------------------------------------*/

#if __GNUG__ || __MWERKS__ || __LINUX__
	#include "UnEngineGnuG.h"
#endif

/*-----------------------------------------------------------------------------
	Size of the world.
-----------------------------------------------------------------------------*/

#define WORLD_MAX		524288.0	/* Maximum size of the world */
#define HALF_WORLD_MAX	262144.0	/* Half the maximum size of the world */
#define HALF_WORLD_MAX1	262143.0	/* Half the maximum size of the world - 1*/
#define MIN_ORTHOZOOM	250.0		/* Limit of 2D viewport zoom in */
#define MAX_ORTHOZOOM	16000000.0	/* Limit of 2D viewport zoom out */

// Karma Includes
#if !defined(__LINUX__)
#ifndef WITH_KARMA
#  define WITH_KARMA
#endif
#endif

#ifdef WITH_KARMA
#if SUPPORTS_PRAGMA_PACK
#pragma pack (push,8)
#endif
    #include "MePrecision.h"
    #include "Mst.h"
    #include "McdTriangleList.h"
    #include "MeAssetDB.h"
    #include "MeAssetFactory.h"
#if SUPPORTS_PRAGMA_PACK
#pragma pack (pop)
#endif

    // These need to be public for UnrealEd etc.
    const MeReal K_ME2UScale = (MeReal)50;
    const MeReal K_U2MEScale = (MeReal)0.02;

	const MeReal K_Rad2U = (MeReal)10430.2192;
	const MeReal K_U2Rad = (MeReal)0.000095875262;
#endif

/*-----------------------------------------------------------------------------
	Editor callback codes.
-----------------------------------------------------------------------------*/
enum EUnrealEdCallbacks
{
	EDC_None						= 0,
	EDC_Browse						= 1,
	EDC_UseCurrent					= 2,
	EDC_CurTexChange				= 10,
	EDC_CurStaticMeshChange			= 11,
	EDC_SelPolyChange				= 20,
	EDC_SelChange					= 21,
	EDC_RtClickTexture				= 23,
	EDC_RtClickStaticMesh			= 24,
	EDC_RtClickPoly					= 26,
	EDC_RtClickActor				= 27,
	EDC_RtClickWindow				= 28,
	EDC_RtClickWindowCanAdd			= 29,
	EDC_MapChange					= 42,
	EDC_ViewportUpdateWindowFrame	= 43,
	EDC_SurfProps					= 44,
	EDC_SaveMap						= 45,
	EDC_SaveMapAs					= 46,
	EDC_LoadMap						= 47,
	EDC_PlayMap						= 48,
	EDC_CamModeChange				= 49,
	EDC_RedrawAllViewports			= 50,
	EDC_ViewportsDisableRealtime	= 51,
	EDC_ActorPropertiesChange		= 52,
	EDC_RtClickTerrainLayer			= 53,
	EDC_RtClickActorStaticMesh		= 54,
	EDC_RefreshEditor				= 55,
	EDC_DisplayLoadErrors			= 56,
	EDC_RtClickMatScene				= 57,
	EDC_RtClickAnimSeq              = 58,
	EDC_RedrawCurrentViewport		= 59,
	EDC_MaterialTreeClick			= 60,
	EDC_RtClickMatAction			= 61,
};

// Used with the EDC_RefreshEditor callback to selectively refresh parts of the editor.
enum ERefreshEditor
{
	ERefreshEditor_Misc					= 1,	// All other things besides browsers
	ERefreshEditor_ActorBrowser			= 2,
	ERefreshEditor_GroupBrowser			= 4,
	ERefreshEditor_MeshBrowser			= 8,
	ERefreshEditor_AnimationBrowser		= 16,
	ERefreshEditor_MusicBrowser			= 32,
	ERefreshEditor_ParticleBrowser		= 64,
	ERefreshEditor_PrefabBrowser		= 128,
	ERefreshEditor_SoundBrowser			= 256,
	ERefreshEditor_StaticMeshBrowser	= 512,
	ERefreshEditor_TextureBrowser		= 1024,
	ERefreshEditor_Matinee				= 2048,
	ERefreshEditor_Terrain				= 4096,
};

#define ERefreshEditor_AllBrowsers		ERefreshEditor_GroupBrowser | ERefreshEditor_MeshBrowser | ERefreshEditor_AnimationBrowser | ERefreshEditor_MusicBrowser | ERefreshEditor_PrefabBrowser | ERefreshEditor_SoundBrowser | ERefreshEditor_StaticMeshBrowser

// Camera orientations for Matinee (this is mirrored in Object.uc - keep all definitions in sync!)
enum ECamOrientation
{
	CAMORIENT_None,
	CAMORIENT_LookAtActor,
	CAMORIENT_FacePath,
	CAMORIENT_Interpolate,
	CAMORIENT_Dolly,
};

// Struct for storing Matinee camera orientations (this is mirrored in MatObject.uc and SceneManager.uc - keep all definitions in sync!)
class UMatSubAction;
struct ENGINE_API FOrientation
{
	// Exposed in editor
	INT	CamOrientation;
	AActor* LookAt;
	FLOAT EaseInTime;
	INT bReversePitch;
	INT bReverseYaw;
	INT bReverseRoll;

	// Work varibles
	UMatSubAction* MA;		// Used entirely for comparison purposes (this is the subaction this orientation belongs to)
	FLOAT PctInStart, PctInEnd, PctInDuration;
	FRotator StartingRotation;		// The original rotation we were at when the change started

	FOrientation()
		: CamOrientation(CAMORIENT_FacePath), LookAt(NULL), EaseInTime(0), MA(NULL), bReverseYaw(0), bReversePitch(0), bReverseRoll(0)
	{}

	FOrientation& operator=( const FOrientation Other )
	{
		CamOrientation = Other.CamOrientation;
		LookAt = Other.LookAt;
		EaseInTime = Other.EaseInTime;
		StartingRotation = Other.StartingRotation;
		PctInStart = Other.PctInStart;
		PctInEnd = Other.PctInEnd;
		PctInDuration = Other.PctInDuration;
		MA = Other.MA;
		bReverseYaw = Other.bReverseYaw;
		bReversePitch = Other.bReversePitch;
		bReverseRoll = Other.bReverseRoll;

		return *this;
	}
	UBOOL operator!=( const FOrientation& Other ) const
	{
		return (MA != Other.MA);
	}
};

// Ease In and Ease Out Interpolation is done by calculating the 
// acceleration which will cover exactly half the distance in half 
// the specified time. That acceleration is applied for the first 
// half, then the inverse is applied for the second half to slow 
// down so the data point stops as it reaches the target point.

class FInterpolator
{
public:
	FInterpolator()
	{
		bDone = 1;
		_value = 1.f;
	}
	~FInterpolator()
	{}
	UBOOL Start( FLOAT InTime )
	{
		bDone = 0;
		if(InTime <= 0)
		{
			bDone = 1;
			return 0;
		}
			_value = 0.f;
		_speed = 0.0f;
		_acceleration = 1.f/(InTime*InTime/4.f);
		_remainingTime = _totalTime = InTime;
		return true;
	}
	void Tick(float deltaTime)
	{
		if( IsDone() )	return;

		_remainingTime -= deltaTime;
		if(_remainingTime < _totalTime/2.f)
		{
			// Deceleration
			_speed -= _acceleration * deltaTime;
		}
		else
		{
			// Acceleration
			_speed += _acceleration * deltaTime;
		}
		_value += _speed*deltaTime;
		bDone = (_remainingTime < 0.f);
	}
	float GetValue()
	{
		return _value;
	}
	UBOOL IsDone()
	{
		return bDone;
	}

	UBOOL bDone;
	float _value;
	float _remainingTime;
	float _totalTime;
	float _speed;
	float _acceleration;
};

/*-----------------------------------------------------------------------------
	FRotatorF.
-----------------------------------------------------------------------------*/

// A floating point version of FRotator.  Used by Matinee so precision doesn't get lost in camera rotation calcs.
class ENGINE_API FRotatorF
{
public:
	// Variables.
	FLOAT Pitch; // Looking up and down (0=Straight Ahead, +Up, -Down).
	FLOAT Yaw;   // Rotating around (running in circles), 0=East, +North, -South.
	FLOAT Roll;  // Rotation about axis of screen, 0=Straight, +Clockwise, -CCW.

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FRotatorF& R )
	{
		return Ar << R.Pitch << R.Yaw << R.Roll;
	}

	// Constructors.
	FRotatorF() {}
	FRotatorF( FLOAT InPitch, FLOAT InYaw, FLOAT InRoll )
	:	Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}
	FRotatorF( FRotator InRotator )
		:	Pitch(InRotator.Pitch), Yaw(InRotator.Yaw), Roll(InRotator.Roll) {}

	FRotator Rotator()
	{
		return FRotator( (INT)Pitch, (INT)Yaw, (INT)Roll );
	}

	FRotatorF operator*( FLOAT In ) const
	{
		return FRotatorF( Pitch*In, Yaw*In, Roll*In );
	}
	FRotatorF operator+( FRotatorF In ) const
	{
		return FRotatorF( Pitch+In.Pitch, Yaw+In.Yaw, Roll+In.Roll );
	}
	FRotatorF operator-( FRotatorF In ) const
	{
		return FRotatorF( Pitch-In.Pitch, Yaw-In.Yaw, Roll-In.Roll );
	}

	FRotatorF operator*=( FLOAT In )
	{
		Pitch *= In;		Yaw *= In;		Roll *= In;
		return *this;
	}
	FRotatorF operator+=( FRotatorF In )
	{
		Pitch += In.Pitch;		Yaw += In.Yaw;		Roll += In.Roll;
		return *this;
	}
	FRotatorF operator-=( FRotatorF In )
	{
		Pitch -= In.Pitch;		Yaw -= In.Yaw;		Roll -= In.Roll;
		return *this;
	}

	inline FVector Vector()
	{
		return (GMath.UnitCoords / Rotator()).XAxis;
	}
};

enum EInputKey
{
    IK_None                 =0,
    IK_LeftMouse            =1,
    IK_RightMouse           =2,
    IK_Cancel               =3,
    IK_MiddleMouse          =4,
    IK_Unknown05            =5,
    IK_Unknown06            =6,
    IK_Unknown07            =7,
    IK_Backspace            =8,
    IK_Tab                  =9,
    IK_Unknown0A            =10,
    IK_Unknown0B            =11,
    IK_Unknown0C            =12,
    IK_Enter                =13,
    IK_Unknown0E            =14,
    IK_Unknown0F            =15,
    IK_Shift                =16,
    IK_Ctrl                 =17,
    IK_Alt                  =18,
    IK_Pause                =19,
    IK_CapsLock             =20,
    IK_Unknown15            =21,
    IK_Unknown16            =22,
    IK_Unknown17            =23,
    IK_Unknown18            =24,
    IK_Unknown19            =25,
    IK_Unknown1A            =26,
    IK_Escape               =27,
    IK_Unknown1C            =28,
    IK_Unknown1D            =29,
    IK_Unknown1E            =30,
    IK_Unknown1F            =31,
    IK_Space                =32,
    IK_PageUp               =33,
    IK_PageDown             =34,
    IK_End                  =35,
    IK_Home                 =36,
    IK_Left                 =37,
    IK_Up                   =38,
    IK_Right                =39,
    IK_Down                 =40,
    IK_Select               =41,
    IK_Print                =42,
    IK_Execute              =43,
    IK_PrintScrn            =44,
    IK_Insert               =45,
    IK_Delete               =46,
    IK_Help                 =47,
    IK_0                    =48,
    IK_1                    =49,
    IK_2                    =50,
    IK_3                    =51,
    IK_4                    =52,
    IK_5                    =53,
    IK_6                    =54,
    IK_7                    =55,
    IK_8                    =56,
    IK_9                    =57,
    IK_Unknown3A            =58,
    IK_Unknown3B            =59,
    IK_Unknown3C            =60,
    IK_Unknown3D            =61,
    IK_Unknown3E            =62,
    IK_Unknown3F            =63,
    IK_Unknown40            =64,
    IK_A                    =65,
    IK_B                    =66,
    IK_C                    =67,
    IK_D                    =68,
    IK_E                    =69,
    IK_F                    =70,
    IK_G                    =71,
    IK_H                    =72,
    IK_I                    =73,
    IK_J                    =74,
    IK_K                    =75,
    IK_L                    =76,
    IK_M                    =77,
    IK_N                    =78,
    IK_O                    =79,
    IK_P                    =80,
    IK_Q                    =81,
    IK_R                    =82,
    IK_S                    =83,
    IK_T                    =84,
    IK_U                    =85,
    IK_V                    =86,
    IK_W                    =87,
    IK_X                    =88,
    IK_Y                    =89,
    IK_Z                    =90,
    IK_Unknown5B            =91,
    IK_Unknown5C            =92,
    IK_Unknown5D            =93,
    IK_Unknown5E            =94,
    IK_Unknown5F            =95,
    IK_NumPad0              =96,
    IK_NumPad1              =97,
    IK_NumPad2              =98,
    IK_NumPad3              =99,
    IK_NumPad4              =100,
    IK_NumPad5              =101,
    IK_NumPad6              =102,
    IK_NumPad7              =103,
    IK_NumPad8              =104,
    IK_NumPad9              =105,
    IK_GreyStar             =106,
    IK_GreyPlus             =107,
    IK_Separator            =108,
    IK_GreyMinus            =109,
    IK_NumPadPeriod         =110,
    IK_GreySlash            =111,
    IK_F1                   =112,
    IK_F2                   =113,
    IK_F3                   =114,
    IK_F4                   =115,
    IK_F5                   =116,
    IK_F6                   =117,
    IK_F7                   =118,
    IK_F8                   =119,
    IK_F9                   =120,
    IK_F10                  =121,
    IK_F11                  =122,
    IK_F12                  =123,
    IK_F13                  =124,
    IK_F14                  =125,
    IK_F15                  =126,
    IK_F16                  =127,
    IK_F17                  =128,
    IK_F18                  =129,
    IK_F19                  =130,
    IK_F20                  =131,
    IK_F21                  =132,
    IK_F22                  =133,
    IK_F23                  =134,
    IK_F24                  =135,
    IK_Unknown88            =136,
    IK_Unknown89            =137,
    IK_Unknown8A            =138,
    IK_Unknown8B            =139,
    IK_Unknown8C            =140,
    IK_Unknown8D            =141,
    IK_Unknown8E            =142,
    IK_Unknown8F            =143,
    IK_NumLock              =144,
    IK_ScrollLock           =145,
    IK_Unknown92            =146,
    IK_Unknown93            =147,
    IK_Unknown94            =148,
    IK_Unknown95            =149,
    IK_Unknown96            =150,
    IK_Unknown97            =151,
    IK_Unknown98            =152,
    IK_Unknown99            =153,
    IK_Unknown9A            =154,
    IK_Unknown9B            =155,
    IK_Unknown9C            =156,
    IK_Unknown9D            =157,
    IK_Unknown9E            =158,
    IK_Unknown9F            =159,
    IK_LShift               =160,
    IK_RShift               =161,
    IK_LControl             =162,
    IK_RControl             =163,
    IK_UnknownA4            =164,
    IK_UnknownA5            =165,
    IK_UnknownA6            =166,
    IK_UnknownA7            =167,
    IK_UnknownA8            =168,
    IK_UnknownA9            =169,
    IK_UnknownAA            =170,
    IK_UnknownAB            =171,
    IK_UnknownAC            =172,
    IK_UnknownAD            =173,
    IK_UnknownAE            =174,
    IK_UnknownAF            =175,
    IK_UnknownB0            =176,
    IK_UnknownB1            =177,
    IK_UnknownB2            =178,
    IK_UnknownB3            =179,
    IK_UnknownB4            =180,
    IK_UnknownB5            =181,
    IK_UnknownB6            =182,
    IK_UnknownB7            =183,
    IK_UnknownB8            =184,
    IK_Unicode              =185,
    IK_Semicolon            =186,
    IK_Equals               =187,
    IK_Comma                =188,
    IK_Minus                =189,
    IK_Period               =190,
    IK_Slash                =191,
    IK_Tilde                =192,
    IK_Mouse4               =193,
    IK_Mouse5               =194,
    IK_Mouse6               =195,
    IK_Mouse7               =196,
    IK_Mouse8               =197,
    IK_UnknownC6            =198,
    IK_UnknownC7            =199,
    IK_Joy1                 =200,
    IK_Joy2                 =201,
    IK_Joy3                 =202,
    IK_Joy4                 =203,
    IK_Joy5                 =204,
    IK_Joy6                 =205,
    IK_Joy7                 =206,
    IK_Joy8                 =207,
    IK_Joy9                 =208,
    IK_Joy10                =209,
    IK_Joy11                =210,
    IK_Joy12                =211,
    IK_Joy13                =212,
    IK_Joy14                =213,
    IK_Joy15                =214,
    IK_Joy16                =215,
    IK_UnknownD8            =216,
    IK_UnknownD9            =217,
    IK_UnknownDA            =218,
    IK_LeftBracket          =219,
    IK_Backslash            =220,
    IK_RightBracket         =221,
    IK_SingleQuote          =222,
    IK_UnknownDF            =223,
    IK_UnknownE0            =224,
    IK_UnknownE1            =225,
    IK_UnknownE2            =226,
    IK_UnknownE3            =227,
    IK_MouseX               =228,
    IK_MouseY               =229,
    IK_MouseZ               =230,
    IK_MouseW               =231,
    IK_JoyU                 =232,
    IK_JoyV                 =233,
    IK_JoySlider1           =234,
    IK_JoySlider2           =235,
    IK_MouseWheelUp         =236,
    IK_MouseWheelDown       =237,
    IK_Unknown10E           =238,
    UK_Unknown10F           =239,
    IK_JoyX                 =240,
    IK_JoyY                 =241,
    IK_JoyZ                 =242,
    IK_JoyR                 =243,
    IK_UnknownF4            =244,
    IK_UnknownF5            =245,
    IK_Attn                 =246,
    IK_CrSel                =247,
    IK_ExSel                =248,
    IK_ErEof                =249,
    IK_Play                 =250,
    IK_Zoom                 =251,
    IK_NoName               =252,
    IK_PA1                  =253,
    IK_OEMClear             =254,
    IK_MAX                  =255,
};
enum EInputAction
{
    IST_None                =0,
    IST_Press               =1,
    IST_Hold                =2,
    IST_Release             =3,
    IST_Axis                =4,
    IST_MAX                 =5,
};

typedef class FDynamicActor* FActorRenderDataPtr;
typedef class FDynamicLight* FLightRenderDataPtr;

typedef struct FProjectorRenderInfo* FProjectorRenderInfoPtr;
typedef struct FStaticProjectorInfo* FStaticMeshProjectorRenderInfoPtr;

struct FBatchReference
{
	INT	BatchIndex,
		ElementIndex;
};

/*-----------------------------------------------------------------------------
	Engine public includes.
-----------------------------------------------------------------------------*/

#include "UnRebuildTools.h"		// Tools used by UnrealEd for rebuilding the level.
#include "Bezier.h"				// Class for computing bezier curves
#include "UnStats.h"			// Performance stat gathering.
#include "UnObj.h"				// Standard object definitions.
#include "UnPrim.h"				// Primitive class.
#include "UnConvexVolume.h"		// Convex volume primitive.
#include "UnRenderResource.h"	// Render resource objects.
#include "UnRenDev.h"			// Rendering interface definition.
#include "UnRenderUtil.h"		// Rendering utilities.
#include "UnMaterial.h"			// Materials
#include "UnTex.h"				// Texture and palette.
#include "UnModel.h"			// Model class.
#include "UnAnim.h"				// Animation.
#include "UnComponents.h"		// Forward declarations of object components of actors
#include "UnVolume.h"			// Volume brushes.
#include "UnVoiceChat.h"		// Voice chat declarations.
#include "KTypes.h"             // Public Karma integration types.
#include "EngineClasses.h"		// All actor classes.
#include "UnPhysic.h"			// Physics constants
#include "UnReach.h"			// Reach specs.
#include "UnURL.h"				// Uniform resource locators.
#include "UnLevel.h"			// Level object.
#include "UnIn.h"				// Input system.
#include "UnPlayer.h"			// Player class.
#include "UnPackageCheckInfo.h"	// Package MD5 info for validation
#include "UnEngine.h"			// Unreal engine.
#include "UnGame.h"				// Unreal game engine.
#include "UnCanvas.h"			// Canvas interface.
#include "UnCamera.h"			// Viewport subsystem.
#include "UnCameraEffects.h"	// Camera effects.
#include "UnRender.h"			// High-level rendering definitions.
#include "UnProjector.h"		// Projected textures and Decals.
#include "UnMesh.h"				// Mesh base.
#include "UnLodMesh.h"			// LOD mesh base class.
#include "UnSkeletalMesh.h"		// Skeletal animated mesh.
#include "UnVertMesh.h"			// Vertex animated mesh.
#include "UnActor.h"			// Actor inlines.
#include "UnAudio.h"			// Audio code.
#include "UnTerrain.h"			// Terrain objects.
#include "UnTerrainTools.h"		// Terrain tools.
#include "UnStaticMesh.h"		// Static T&L meshes.
#include "UnMatineeTools.h"		// Matinee tools.
#include "UnFluidSurface.h"		// Fluid Surface
#include "UnStatGraph.h"		// Stat drawing utility.
#include "UnCDKey.h"			// CD key validation.
#include "UMovie.h"				// Added by Demiurge Studios (Movie)

/*-----------------------------------------------------------------------------
	Hit proxies.
-----------------------------------------------------------------------------*/

// Hit an axis indicator on a gizmo
struct HGizmoAxis : public HHitProxy
{
	DECLARE_HIT_PROXY(HGizmoAxis,HHitProxy)
	AActor* Actor;
	INT Axis;
	HGizmoAxis( AActor* InActor, INT InAxis ) : Actor(InActor), Axis(InAxis) {}
	virtual AActor* GetActor()
	{
		return Actor;
	}

};

// Hit an actor vertex.
struct HActorVertex : public HHitProxy
{
	DECLARE_HIT_PROXY(HActorVertex,HHitProxy)
	AActor* Actor;
	FVector Location;
	HActorVertex( AActor* InActor, FVector InLocation ) : Actor(InActor), Location(InLocation) {}
	virtual AActor* GetActor()
	{
		return Actor;
	}
};

// Hit a bezier control point
struct HBezierControlPoint : public HHitProxy
{
	DECLARE_HIT_PROXY(HBezierControlPoint,HHitProxy)
	UMatAction* MA;
	UBOOL bStart;		// Is this the starting(=0) or ending(=1) control point?
	HBezierControlPoint( UMatAction* InMA, UBOOL InStart ) : MA(InMA), bStart(InStart) {}

	UBOOL operator==( const HBezierControlPoint& BCP ) const
	{
		return (MA==BCP.MA && bStart==BCP.bStart);
	}
};

/*-----------------------------------------------------------------------------
	Terrain editing brush types.
-----------------------------------------------------------------------------*/

enum ETerrainBrush
{
	TB_None				= -1,
	TB_VertexEdit		= 0,	// Select/Drag Vertices on heightmap
	TB_Paint			= 1,	// Paint on selected layer
	TB_Smooth			= 2,	// Does a filter on the selected vertices
	TB_Noise			= 3,	// Adds random noise into the selected vertices
	TB_Flatten			= 4,	// Flattens the selected vertices to the height of the vertex which was initially clicked
	TB_TexturePan		= 5,	// Pans the texture on selected layer
	TB_TextureRotate	= 6,	// Rotates the texture on selected layer
	TB_TextureScale		= 7,	// Scales the texture on selected layer
	TB_Select			= 8,	// Selects areas of the terrain for copying, generating new terrain, etc
	TB_Visibility		= 9,	// Toggles terrain sectors on/off
	TB_Color			= 10,	// Paints color into the RGB channels of layers
	TB_EdgeTurn			= 11,	// Turns edges of terrain triangulation
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
#endif

