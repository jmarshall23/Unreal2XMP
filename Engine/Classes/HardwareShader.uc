class HardwareShader extends RenderedMaterial
	native
	noexport;

// DS_SHADER

enum SConstant
{
	EVC_Unused, // This constant isn't used by the shader and does not need to be updated
	EVC_MaterialDefined, // The constant is defined by by the Value member of FVSConstantsInfo
	EVC_ViewProjMatrix, // The constant, and the three constants after it will contain the view projection matrix
	EVC_WorldViewProjMatrix,
	EVC_WorldMatrix,
	EVC_InvViewMatrix, // The constant, and the three constants after it will contain the inverse view matrix
	EVC_ViewMatrix, // The constant, and the three constants after it will contain the view matrix
	EVC_InvWorldMatrix,
	EVC_Time, // Time, cycled over 120 seconds
	EVC_CosTime,
	EVC_SinTime,
	EVC_TanTime,
	EVC_Eye,
	EVC_XYCircle,
	EVC_NearestLightPos1,
	EVC_InvLightRadius1,
	EVC_NearestLightPos2,
	EVC_InvLightRadius2,
	EVC_LightColor1,
	EVC_LightColor2,
	EVC_Max
};

/**
 * Used to specify constants for a vertex shader. 
 */ 
struct SConstantsInfo
{
	var() SConstant Type;
	var() Plane Value;
};


//-----------------------------------------------------------------------------
// Vertex Shader Data

var(HardwareShader) SConstantsInfo VSConstants[200];

var(HardwareShader) string VertexShaderText[4];

/** 
 * Indexed array of where the streams will show up
 * if StreamMapping[0] == NORMAL then v0 will contain the vertex normal
 */
var(HardwareShader) array<INT> StreamMapping;



//-----------------------------------------------------------------------------
// Pixel Shader Data

var(HardwareShader) SConstantsInfo PSConstants[32];

var(HardwareShader) string PixelShaderText[4];

// Textures to be used by each stage/pass
// 2D array, PASSES x STAGES
var(HardwareShader) Texture Textures[32];

// Native pointers
var transient const int PixelShader[4];
var transient const int VertexShader[4];

var() int NumPasses;

var(HardwareShader) int AlphaBlending[4];
var(HardwareShader) int AlphaTest[4];
var(HardwareShader) byte AlphaRef[4];
var(HardwareShader) int ZTest[4];
var(HardwareShader) int ZWrite[4];
var(HardwareShader) byte SrcBlend[16];
var(HardwareShader) byte DestBlend[16];

defaultproperties
{
	NumPasses=1;
}