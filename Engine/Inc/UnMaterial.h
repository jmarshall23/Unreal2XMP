

//
// Forward declarations
//
class	UMaterial;
class		URenderedMaterial;
class			UBitmapMaterial;
class			UHardwareShader;
class				UTexture;

class	UModifier;

class FTexture;
class FCubemap;


//
// EFillMode
//
enum EFillMode
{
	FM_Wireframe	= 0,
	FM_FlatShaded	= 1,
	FM_Solid		= 2,
};

/*-----------------------------------------------------------------------------
	UMaterial.
-----------------------------------------------------------------------------*/

class ENGINE_API UMaterial : public UObject
{
	DECLARE_ABSTRACT_CLASS(UMaterial,UObject,0,Engine)

	UMaterial*	FallbackMaterial;
	UMaterial*	DefaultMaterial;
	BITFIELD	UseFallback:1		GCC_PACK(4);
    BITFIELD	Validated:1;

	// Constructor.
	UMaterial();

	// UObject interface
	void Serialize( FArchive& Ar );

	// UMaterial interface
	virtual UBOOL CheckCircularReferences( TArray<UMaterial*>& History );
	virtual UBOOL GetValidated() { return Validated; }
	virtual void SetValidated( UBOOL InValidated ) { Validated = InValidated; }
	virtual void PreSetMaterial(FLOAT TimeSeconds) {};

	// Getting information about a combined material:
	virtual INT MaterialUSize() { return 0; }
	virtual INT MaterialVSize() { return 0; }
	virtual UBOOL RequiresSorting() { return 0; }
	virtual UBOOL IsTransparent() { return 0; }
	virtual BYTE RequiredUVStreams() { return 1; }
	virtual UBOOL RequiresNormal() { return 0; }

	// Fallback handling
	static void ClearFallbacks();
	virtual UMaterial* CheckFallback();
	virtual UBOOL HasFallback() { return FallbackMaterial != NULL; }

	//!! OLDVER
	UMaterial* ConvertPolyFlagsToMaterial( UMaterial* InMaterial, DWORD InPolyFlags );
};

/*-----------------------------------------------------------------------------
	URenderedMaterial.
-----------------------------------------------------------------------------*/

class ENGINE_API URenderedMaterial : public UMaterial
{
	DECLARE_ABSTRACT_CLASS(URenderedMaterial,UMaterial,0,Engine)
};

/*-----------------------------------------------------------------------------
	UBitmapMaterial.
-----------------------------------------------------------------------------*/
class ENGINE_API UBitmapMaterial : public URenderedMaterial
{
	DECLARE_ABSTRACT_CLASS(UBitmapMaterial,URenderedMaterial,0,Engine)

	BYTE		Format;				// ETextureFormat.
	BYTE		UClampMode;			// Texture U clamp mode
	BYTE		VClampMode;			// Texture V clamp mode

	BYTE		UBits, VBits;		// # of bits in USize, i.e. 8 for 256.
	INT			USize, VSize;		// Size, must be power of 2.
	INT			UClamp, VClamp;		// Clamped width, must be <= size.
	


	// UBitmapMaterial interface.
	virtual FBaseTexture* GetRenderInterface() = 0;
	virtual UBitmapMaterial* Get( FTime Time, UViewport* Viewport ) { return this; }

	// UMaterial Interface
	virtual INT MaterialUSize() { return USize; }
	virtual INT MaterialVSize() { return VSize; }
};

/*-----------------------------------------------------------------------------
	UProxyBitmapMaterial
-----------------------------------------------------------------------------*/
class ENGINE_API UProxyBitmapMaterial : public UBitmapMaterial
{
	DECLARE_CLASS(UProxyBitmapMaterial,UBitmapMaterial,0,Engine);

private:

	FBaseTexture*	TextureInterface;

public:

	// UProxyBitmapMaterial interface.
	void SetTextureInterface(FBaseTexture* InTextureInterface)
	{
		TextureInterface = InTextureInterface;
		Format = TextureInterface->GetFormat();
		UClampMode = TextureInterface->GetUClamp();
		VClampMode = TextureInterface->GetVClamp();
		UClamp = USize = TextureInterface->GetWidth();
		VClamp = VSize = TextureInterface->GetHeight();
		UBits = appCeilLogTwo(UClamp);
		VBits = appCeilLogTwo(VClamp);
	}

	// UBitmapMaterial interface.
	virtual FBaseTexture* GetRenderInterface() { return TextureInterface; }
	virtual UBitmapMaterial* Get( FTime Time, UViewport* Viewport ) { return this; }
};

/*-----------------------------------------------------------------------------
	UTexCoordMaterial
-----------------------------------------------------------------------------*/
class ENGINE_API UTexCoordMaterial : public URenderedMaterial
{
    DECLARE_CLASS(UTexCoordMaterial,URenderedMaterial,0,Engine)

	class UBitmapMaterial* Texture;
    class UTexCoordGen* TextureCoords;

	// UMaterial interface
	virtual INT MaterialUSize() { return Texture ? Texture->MaterialUSize() : 0; }
	virtual INT MaterialVSize() { return Texture ? Texture->MaterialVSize() : 0; }
};

/*-----------------------------------------------------------------------------
	UHardwareShader
-----------------------------------------------------------------------------*/


// When updating this, also update ShaderConstantNames in UnEdSrv.cpp and the enum in HardwareShader.uc...sorry
enum ESConstant
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
	EVC_AmbientLightColor,
	EVC_Max
};


/**
 * Used to specify constants for a vertex shader. 
 */ 
struct FSConstantsInfo
{
	FSConstantsInfo() : Type(EVC_Unused), Value(0.0f, 0.0f, 0.0f, 0.0f) {}

	ESConstant Type;
	FPlane Value;
};

// Update HardwareShader.uc if you change ANY of these values!
const INT MAX_PASSES = 4;
const INT MAX_TEXTURE_STAGES = 8;
#define NUM_VSHADER_CONSTANTS 50 
#define NUM_PSHADER_CONSTANTS 8

class ENGINE_API UHardwareShader : public URenderedMaterial
{
    DECLARE_CLASS(UHardwareShader,URenderedMaterial,0,Engine)

public:

	//-----------------------------------------------------------------------------
	// Vertex Shader Data

	FSConstantsInfo VSConstants[NUM_VSHADER_CONSTANTS][MAX_PASSES];

	FString VertexShaderText[MAX_PASSES];

	/** 
	 * Indexed array of where the streams will show up
	 * if StreamMapping[0] == NORMAL then v0 will contain the vertex normal
	 */
	TArray<EFixedVertexFunction> StreamMapping;


	//-----------------------------------------------------------------------------
	// Pixel Shader Data

	FSConstantsInfo PSConstants[NUM_PSHADER_CONSTANTS][MAX_PASSES];

	FString PixelShaderText[MAX_PASSES];

	// Textures to be used by each stage/pass
	// 2D array, PASSES x STAGES
	UTexture* Textures[32];

	FVertexShader* VertexShaders[MAX_PASSES];
	FPixelShader* PixelShaders[MAX_PASSES];

	INT NumPasses;

public:

	/**
	 * Constructor
	 */
	UHardwareShader();

	virtual void PostLoad();
	virtual void Destroy();
	virtual void PostEditChange();

	void Changed();

	UBOOL AlphaBlending[MAX_PASSES];
	UBOOL AlphaTest[MAX_PASSES];
	BYTE AlphaRef[MAX_PASSES];
	UBOOL ZTest[MAX_PASSES];
	UBOOL ZWrite[MAX_PASSES];
	DWORD SrcBlend[MAX_PASSES];
	DWORD DestBlend[MAX_PASSES];

	// TODO Al: Yank this with the template bug is resolved, temp hack
	void InitDuplicate();
};
 
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/


