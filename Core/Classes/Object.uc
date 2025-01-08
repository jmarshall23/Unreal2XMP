//=============================================================================
// Object: The base class all objects.
// This is a built-in Unreal class and it shouldn't be modified.
//=============================================================================
class Object
	native
	noexport;

//=============================================================================
// UObject variables.

// Internal variables.
var native private const int ObjectInternal[6];
var native const object Outer;
var native const int ObjectFlags;
var(Object) native const editconst name Name;
var native const editconst class Class;

//=============================================================================
// Unreal base structures.

// Object flags.
const RF_Transactional	= 0x00000001; // Supports editor undo/redo.
const RF_Public         = 0x00000004; // Can be referenced by external package files.
const RF_Transient      = 0x00004000; // Can't be saved or loaded.
const RF_NotForClient	= 0x00100000; // Don't load for game client.
const RF_NotForServer	= 0x00200000; // Don't load for game server.
const RF_NotForEdit		= 0x00400000; // Don't load for editor.

const NoState = '';
const NoLabel = '';

// A globally unique identifier.
struct Guid
{
	var int A, B, C, D;
};

// A point or direction vector in 3d space.
struct Vector
{
	var() config float X, Y, Z;
};

// A plane definition in 3d space.
struct Plane extends Vector
{
	var() config float W;
};

// An orthogonal rotation in 3d space.
struct Rotator
{
	var() config int Pitch, Yaw, Roll;
};

// An arbitrary coordinate system in 3d space.
struct Coords
{
	var() config vector Origin, XAxis, YAxis, ZAxis;
};

// Quaternion
struct Quat
{
	var() config float X, Y, Z, W;
};

// Used to generate random values between Min and Max
struct Range
{
	var() config float A;
	var() config float B;
};

// Vector of Ranges
struct RangeVector
{
	var() config range X;
	var() config range Y;
	var() config range Z;
};

// A scale and sheering.
struct Scale
{
	var() config vector Scale;
	var() config float SheerRate;
	var() config enum ESheerAxis
	{
		SHEER_None,
		SHEER_XY,
		SHEER_XZ,
		SHEER_YX,
		SHEER_YZ,
		SHEER_ZX,
		SHEER_ZY,
	} SheerAxis;
};

// Camera orientations for Matinee
enum ECamOrientation
{
	CAMORIENT_None,
	CAMORIENT_LookAtActor,
	CAMORIENT_FacePath,
	CAMORIENT_Interpolate,
	CAMORIENT_Dolly,
};

// Generic axis enum.
enum EAxis
{
	AXIS_X,
	AXIS_Y,
	AXIS_Z
};

// A color.
struct Color
{
	var() config byte B, G, R, A;
};

// A bounding box.
struct Box
{
	var vector Min, Max;
	var byte IsValid;
};

// A bounding box sphere together.
struct BoundingVolume extends Box
{
	var plane Sphere;
};

// a 4x4 matrix
struct Matrix
{
	var() Plane XPlane;
	var() Plane YPlane;
	var() Plane ZPlane;
	var() Plane WPlane;
};

// A interpolated function
struct InterpCurvePoint
{
	var() float InVal;
	var() float OutVal;
};

struct InterpCurve
{
	var() array<InterpCurvePoint>	Points;
};

struct CompressedPosition
{
	var vector Location;
	var rotator Rotation;
	var vector Velocity;
};

enum EAlignment
{
	ALMT_None,
	ALMT_Left,
	ALMT_Right,
	ALMT_Top,
	ALMT_Bottom,
	ALMT_Center,
	ALMT_HardLeft,
	ALMT_HardRight,
	ALMT_HardTop,
	ALMT_HardBottom,
	ALMT_HardCenter,
};

struct Point
{
	var() public float X;
	var() public float Y;
};

struct Dimension
{
	var() public float Width;
	var() public float Height;
};

struct Rectangle
{
	var() public float X;
	var() public float Y;
	var() public float Width;
	var() public float Height;
};

struct Alignment
{
	var() public EAlignment XAxis;
	var() public EAlignment YAxis;
};

struct ComponentHandle
{
	var const native int buffer;
};

struct ParticleHandle
{
	var const native int buffer;
};

//=============================================================================
// Constants.

const MaxInt = 0x7fffffff;
const MaxFlt							= 3.402823466E38;
const Pi     = 3.1415926535897932;
const DegreesToRadians					= 0.0174532925199432;	// PI / 180.0
const RadiansToDegrees					= 57.295779513082321;	// 180.0 / PI
const DegreesToRotationUnits			= 182.044; 				// 65536 / 360
const RotationUnitsToDegrees			= 0.00549; 				// 360 / 65536

//=============================================================================
// Basic native operators and functions.

// Handle validity checks.
static final postoperator bool ?(ComponentHandle A){ return A.buffer!=0; }
static final preoperator  bool !(ComponentHandle A){ return A.buffer==0; }
static final postoperator bool ?(ParticleHandle A){ return A.buffer!=0; }
static final preoperator  bool !(ParticleHandle A){ return A.buffer==0; }

// Bool operators.
native(129) static final preoperator  bool  !  ( bool A );
native(242) static final operator(24) bool  == ( bool A, bool B );
native(243) static final operator(26) bool  != ( bool A, bool B );
native(130) static final operator(30) bool  && ( bool A, skip bool B );
native(131) static final operator(30) bool  ^^ ( bool A, bool B );
native(132) static final operator(32) bool  || ( bool A, skip bool B );

// Additional bool operators.
native static final postoperator bool ? ( bool    B );//{ return  B; } 
//native static final preoperator  bool ! ( bool    B );//{ return !B; } 
native static final postoperator bool ? ( name    N );//{ return N != ''; } 
native static final preoperator  bool ! ( name    N );//{ return N == ''; } 
native static final postoperator bool ? ( int     I );//{ return I != 0; } 
native static final preoperator  bool ! ( int     I );//{ return I == 0; } 
native static final postoperator bool ? ( float   F );//{ return F != 0; } 
native static final preoperator  bool ! ( float   F );//{ return F == 0; } 
native static final postoperator bool ? ( Object  O );//{ return O != none; } 
native static final preoperator  bool ! ( Object  O );//{ return O == none; } 
native static final postoperator bool ? ( string  S );//{ return S != ""; } 
native static final preoperator  bool ! ( string  S );//{ return S == ""; } 
native static final postoperator bool ? ( vector  V );//{ return V != vect(0,0,0); } 
native static final preoperator  bool ! ( vector  V );//{ return V == vect(0,0,0); } 
native static final postoperator bool ? ( rotator R );//{ return R != rot(0,0,0); } 
native static final preoperator  bool ! ( rotator R );//{ return R == rot(0,0,0); }

// Byte operators.
native(133) static final operator(34) byte *= ( out byte A, byte B );
native(134) static final operator(34) byte /= ( out byte A, byte B );
native(135) static final operator(34) byte += ( out byte A, byte B );
native(136) static final operator(34) byte -= ( out byte A, byte B );
native(137) static final preoperator  byte ++ ( out byte A );
native(138) static final preoperator  byte -- ( out byte A );
native(139) static final postoperator byte ++ ( out byte A );
native(140) static final postoperator byte -- ( out byte A );

// Integer operators.
native(141) static final preoperator  int  ~  ( int A );
native(143) static final preoperator  int  -  ( int A );
native(144) static final operator(16) int  *  ( int A, int B );
native(145) static final operator(16) int  /  ( int A, int B );
native(146) static final operator(20) int  +  ( int A, int B );
native(147) static final operator(20) int  -  ( int A, int B );
native(148) static final operator(22) int  << ( int A, int B );
native(149) static final operator(22) int  >> ( int A, int B );
native(196) static final operator(22) int  >>>( int A, int B );
native(150) static final operator(24) bool <  ( int A, int B );
native(151) static final operator(24) bool >  ( int A, int B );
native(152) static final operator(24) bool <= ( int A, int B );
native(153) static final operator(24) bool >= ( int A, int B );
native(154) static final operator(24) bool == ( int A, int B );
native(155) static final operator(26) bool != ( int A, int B );
native(156) static final operator(28) int  &  ( int A, int B );
native(157) static final operator(28) int  ^  ( int A, int B );
native(158) static final operator(28) int  |  ( int A, int B );
native(159) static final operator(34) int  *= ( out int A, float B );
native(160) static final operator(34) int  /= ( out int A, float B );
native(161) static final operator(34) int  += ( out int A, int B );
native(162) static final operator(34) int  -= ( out int A, int B );
native(163) static final preoperator  int  ++ ( out int A );
native(164) static final preoperator  int  -- ( out int A );
native(165) static final postoperator int  ++ ( out int A );
native(166) static final postoperator int  -- ( out int A );

// Integer functions.
native(167) static final Function     int  Rand  ( int Max ); //NOTE (mdf): returns values in 0..Max-1
native(249) static final function     int  Min   ( int A, int B );
native(250) static final function     int  Max   ( int A, int B );
native(251) static final function     int  Clamp ( int V, int A, int B );

// Float operators.
native(169) static final preoperator  float -  ( float A );
native(170) static final operator(12) float ** ( float A, float B );
native(171) static final operator(16) float *  ( float A, float B );
native(172) static final operator(16) float /  ( float A, float B );
native(173) static final operator(18) float %  ( float A, float B );
native(174) static final operator(20) float +  ( float A, float B );
native(175) static final operator(20) float -  ( float A, float B );
native(176) static final operator(24) bool  <  ( float A, float B );
native(177) static final operator(24) bool  >  ( float A, float B );
native(178) static final operator(24) bool  <= ( float A, float B );
native(179) static final operator(24) bool  >= ( float A, float B );
native(180) static final operator(24) bool  == ( float A, float B );
native(210) static final operator(24) bool  ~= ( float A, float B );
native(181) static final operator(26) bool  != ( float A, float B );
native(182) static final operator(34) float *= ( out float A, float B );
native(183) static final operator(34) float /= ( out float A, float B );
native(184) static final operator(34) float += ( out float A, float B );
native(185) static final operator(34) float -= ( out float A, float B );

// Float functions.
native(186) static final function     float Abs   ( float A );
native(187) static final function     float Sin   ( float A );
native      static final function	  float Asin  ( float A );
native(188) static final function     float Cos   ( float A );
native      static final function     float Acos  ( float A );
native(189) static final function     float Tan   ( float A );
native(190) static final function     float Atan  ( float A, float B ); 
native(191) static final function     float Exp   ( float A );
native(192) static final function     float Loge  ( float A );
native(193) static final function     float Sqrt  ( float A );
native(194) static final function     float Square( float A );
native(195) static final function     float FRand (); //NOTE (mdf): returns values in 0.0..1.0
native(244) static final function     float FMin  ( float A, float B );
native(245) static final function     float FMax  ( float A, float B );
native(246) static final function     float FClamp( float V, float A, float B );
native(247) static final function     float Lerp  ( float Alpha, float A, float B );
native(248) static final function     float Smerp ( float Alpha, float A, float B );
            static final function     float Round ( float A ){ return int(A+0.5); }


// Vector operators.
native(211) static final preoperator  vector -     ( vector A );
native(212) static final operator(16) vector *     ( vector A, float B );
native(213) static final operator(16) vector *     ( float A, vector B );
native(296) static final operator(16) vector *     ( vector A, vector B );
native(214) static final operator(16) vector /     ( vector A, float B );
native(215) static final operator(20) vector +     ( vector A, vector B );
native(216) static final operator(20) vector -     ( vector A, vector B );
native(275) static final operator(22) vector <<    ( vector A, rotator B );
native(276) static final operator(22) vector >>    ( vector A, rotator B );
native(217) static final operator(24) bool   ==    ( vector A, vector B );
native(218) static final operator(26) bool   !=    ( vector A, vector B );
native(219) static final operator(16) float  Dot   ( vector A, vector B );
native(220) static final operator(16) vector Cross ( vector A, vector B );
native(221) static final operator(34) vector *=    ( out vector A, float B );
native(297) static final operator(34) vector *=    ( out vector A, vector B );
native(222) static final operator(34) vector /=    ( out vector A, float B );
native(223) static final operator(34) vector +=    ( out vector A, vector B );
native(224) static final operator(34) vector -=    ( out vector A, vector B );

// Vector functions.
native(225) static final function float  VSize  ( vector A );
native(341) static final function float  VSizeSq ( vector A );
native(342)	static final function float  VSize2D ( vector A );
native(343)	static final function float  VSizeSq2D( vector A );
native static final function vector RotateAngleAxis( vector V, vector Axis, INT Angle ); // Axis is assumed to be normalized
native(226) static final function vector Normal ( vector A );
native(227) static final function        Invert ( out vector X, out vector Y, out vector Z );
native(252) static final function vector VRand  ( );
native(300) static final function vector MirrorVectorByNormal( vector Vect, vector Normal );
native(253) static final function vector RandomSpreadVector( float SpreadDegrees );

// Rotator operators and functions.
native(142) static final operator(24) bool ==     ( rotator A, rotator B );
native(203) static final operator(26) bool !=     ( rotator A, rotator B );
native(287) static final operator(16) rotator *   ( rotator A, float    B );
native(288) static final operator(16) rotator *   ( float    A, rotator B );
native(289) static final operator(16) rotator /   ( rotator A, float    B );
native(290) static final operator(34) rotator *=  ( out rotator A, float B  );
native(291) static final operator(34) rotator /=  ( out rotator A, float B  );
native(316) static final operator(20) rotator +   ( rotator A, rotator B );
native(317) static final operator(20) rotator -   ( rotator A, rotator B );
native(318) static final operator(34) rotator +=  ( out rotator A, rotator B );
native(319) static final operator(34) rotator -=  ( out rotator A, rotator B );
native(229) static final function GetAxes         ( rotator A, out vector X, out vector Y, out vector Z );
native(230) static final function GetUnAxes       ( rotator A, out vector X, out vector Y, out vector Z );
native(320) static final function rotator RotRand ( optional bool bRoll );
native      static final function rotator OrthoRotation( vector X, vector Y, vector Z );
native      static final function rotator Normalize( rotator Rot );
native		static final operator(24) bool ClockwiseFrom( int A, int B );

// Range operators.
native(321) static final preoperator  range -     ( range A );
native(322) static final operator(16) range *     ( range A, float B );
native(323) static final operator(16) range *     ( float A, range B );
native(324) static final operator(16) range *     ( range A, range B );
native(340) static final operator(16) range /     ( float A, range B );
native(325) static final operator(16) range /     ( range A, float B );
native(326) static final operator(20) range +     ( range A, range B );
native(327) static final operator(20) range -     ( range A, range B );
native(328) static final operator(24) bool  ==    ( range A, range B );
native(329) static final operator(26) bool  !=    ( range A, range B );
native(330) static final operator(34) range *=    ( out range A, float B );
native(331) static final operator(34) range *=    ( out range A, range B );
native(332) static final operator(34) range /=    ( out range A, float B );
native(333) static final operator(34) range +=    ( out range A, range B );
native(334) static final operator(34) range -=    ( out range A, range B );
native(335) static final operator(10) float @=    ( out float A, range B );

// Range functions.
native(336) static final function float RSize     ( range A );
native(337) static final function float GetRand   ( range A );
native(338) static final function float RMin      ( range A );
native(339) static final function float RMax      ( range A );

// String operators.
native(112) static final operator(40) string $  ( coerce string A, coerce string B );
native(168) static final operator(40) string @  ( coerce string A, coerce string B );
native(115) static final operator(24) bool   <  ( string A, string B );
native(116) static final operator(24) bool   >  ( string A, string B );
native(120) static final operator(24) bool   <= ( string A, string B );
native(121) static final operator(24) bool   >= ( string A, string B );
native(122) static final operator(24) bool   == ( string A, string B );
native(123) static final operator(26) bool   != ( string A, string B );
native(124) static final operator(24) bool   ~= ( string A, string B );

// String functions.
native(125) static final function int    Len    ( coerce string S );
native(126) static final function int    InStr  ( coerce string S, coerce string t );
native(127) static final function string Mid    ( coerce string S, int i, optional int j );
native(128) static final function string Left   ( coerce string S, int i );
native(234) static final function string Right  ( coerce string S, int i );
native(235) static final function string Caps   ( coerce string S );
native		static final function string Locs   ( coerce string S );
native(236) static final function string Chr    ( int i );
native(237) static final function int    Asc    ( string S );
native      static final function bool   Split  ( string S, string InS, out string LeftS, out string RightS, optional bool bRight );
native      static final function int    StrArray( string S, string Delim, out array<string> Ar, optional bool bUnique );

native static final function float LineLineIntersection( vector P1, vector V1, vector P2, vector P3 );

// Object operators.
native(114) static final operator(24) bool == ( Object A, Object B );
native(119) static final operator(26) bool != ( Object A, Object B );

// Name operators.
native(254) static final operator(24) bool == ( name A, name B );
native(255) static final operator(26) bool != ( name A, name B );

// Return name corresponding to from name table. If bAdd is true will add to name table if not found.
native static final function name StringToName( string S, optional bool bAdd );

// InterpCurve operator
native		static final function float InterpCurveEval( InterpCurve curve, float input );
native		static final function InterpCurveGetOutputRange( InterpCurve curve, out float min, out float max );
native		static final function InterpCurveGetInputDomain( InterpCurve curve, out float min, out float max );

// Quaternion functions
native		static final function Quat QuatProduct( Quat A, Quat B );
native		static final function Quat QuatInvert( Quat A );
native		static final function vector QuatRotateVector( Quat A, vector B );
native		static final function Quat QuatFindBetween( Vector A, Vector B );
native		static final function Quat QuatFromAxisAndAngle( Vector Axis, Float Angle );
native		static final function Quat QuatFromRotator( rotator A );
native		static final function rotator QuatToRotator( Quat A );

//=============================================================================
// General functions.

// Logging.
native(231) final static function Log( coerce string S, optional name Tag );
native(232) final static function Warn( coerce string S );
native static function string Localize( string SectionName, string KeyName, string PackageName );
native final static function int FileSize( string Filename );

// Goto state and label.
native(113) final function GotoState( optional name NewState, optional name Label );
native(281) final function bool IsInState( name TestState );
native(284) final function name GetStateName();

// Objects.
native(258) static final function bool ClassIsChildOf( class TestClass, class ParentClass );
native(303) final function bool IsA( name ClassName );

// Probe messages.
native(117) final function Enable( name ProbeFunc );
native(118) final function Disable( name ProbeFunc );

// Properties.
native final function string GetPropertyText( string PropName );
native final function SetPropertyText( string PropName, string PropValue, optional bool bIgnoreConst ); // NEW (mdf)
//OLD native final function SetPropertyText( string PropName, string PropValue );
native static final function name GetEnum( object E, int i );
native static final function object DynamicLoadObject( string ObjectName, class ObjectClass, optional bool MayFail );
native static final function object FindObject( string ObjectName, class ObjectClass );

/* 
Timer Notes (mdf):

When cleaning up objects, be sure to remove any timers from the object before
it is destroyed or the timer manager could end up trying to call a NotifyFunc 
off an invalid object (=crash). A good way to handle this for Actors is to call 
RemoveAllTimers in the Actor's Destroyed event. For Objects, one approach is
to add a Cleanup function to the Object and include a call to RemoveAllTimers
in there then call Cleanup as needed from outside of the object.

When a timer is set up, it is forever associated with the specified function
(the function associated with the given name within the current context for the
object which is setting up the timer). i.e. if the object's state changes, the
timer will *not* call a different function if one exists within the object's
new context.

It is possible to "override" a timer indirectly -- just have the global timer
function call a 2nd function (which *will* respect the state scoping rules).

Timers can't be notified more frequently than the framerate (e.g. if the 
framerate is consistently < 10FPS, a timers aren't going to be getting 
notified more frequently than about every 0.1 seconds.
*/
native simulated final function AddTimer( name NotifyFunc, float Frequency, optional bool bRepeat, optional float RandomSpread, optional bool bForceUpdate );
native simulated final function AddTimerS( string NotifyFunc, float Frequency, optional bool bRepeat, optional float RandomSpread, optional bool bForceUpdate );
native simulated final function RemoveTimer( name NotifyFunc );
native simulated final function RemoveTimerS( string NotifyFunc );
native simulated final function RemoveAllTimers();

native simulated final function float TimeRemaining( name NotifyFunc ); // returns -1 if timer not found, Max( 0, time remaining) otherwise
native simulated final function float TimeRemainingS( string NotifyFunc ); // returns -1 if timer not found, Max( 0, time remaining) otherwise

// Configuration.
native(536) final function SaveConfig();
native static final function StaticSaveConfig();
native static final function ResetConfig();

// Deallocation.
native(1032) final function Delete();	// Should be called on all Objects *before* their Outer is Destroyed.

// Given a seed, it returns the next seed in the pseudo-random sequence. Has a 2**22 period.
// This is the easiest LCG I could find that had a reliable period and good distribution :)
// Usage:
/*
//to seed our random number generator
Seed = Rand();
log("Random Number 0: "$Seed);
ProgressSeed(Seed);
log("Random Number 1: "$Seed);
ProgressSeed(Seed);
MAXSEED = 2**22;
//must divide the integer by the period (2^22) to get a 0->1 float value
RandomFloat = (Seed/MAXSEED);
log("Random Float: "$RandomFloat);
*/
const MAXSEED = 4194304.0;//2**22;
native static final function ProgressSeed( out int Seed );

native(1033) static final function float RandRange( float Min, float Max ); // Return a random number within Min..Max-1.

native(1034) static final function float Blend( float A, float B, float Pct );
native(1035) static final function float Ablend( float A, float B, float Result );
native(1036) static final function float BlendR( Range R, float Pct );
native(1037) static final function float AblendR( Range R, float Result );

native(1038) static final function bool FDecision( float Odds ); // NEW (mdf) returns true if 0.0 < FRand() <= Odds

//(e.g. use EnumStr( enum'ERenderStyle', Style, 4 ) to return "Translucent")
native static final function string EnumStr( Object Type, byte Val, optional int Pre );

//=============================================================================
// Engine notification functions.

//
// Called immediately after New()ed.
//
event Constructed();

//
// Called immediately when entering a state, while within
// the GotoState call that caused the state change.
//
event BeginState();

//
// Called immediately before going out of the current state,
// while within the GotoState call that caused the state change.
// 
event EndState();

defaultproperties
{
}
