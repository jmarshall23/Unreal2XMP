/*-----------------------------------------------------------------------------
	UClient.
-----------------------------------------------------------------------------*/

//
// Client, responsible for tracking viewports.
//
class Client extends Object
	abstract
	config
	native
	noexport;

	enum ETextureDetail
	{
		UltraHigh,	// +4
		VeryHigh,	// +3
		High,		// +2
		Higher,		// +1
		Normal,		// No-change
		Lower,		// -1
		Low,		// -2 
		VeryLow,	// -3 
		UltraLow,	// -4
	};

	// Variables.
	var Engine					Engine;
	var array<Viewport>			Viewports;

	// Configurable.
	var config bool				CaptureMouse;
	var config bool				ScreenFlashes;
	var config bool				NoLighting;
	var config bool				Decals;
	var config bool				NoDynamicLights;
	var config bool				NoFractalAnim;
	var config bool				Coronas;
	var config bool				DecoLayers;
	var config bool				Projectors;
	var config bool				PlayerShadows;
	var config bool				ReportDynamicUploads;
	var config int				WindowedViewportX;
	var config int				WindowedViewportY;
	var config int				WindowedOffsetX; //NEW (mdf) windowed mode offset support
	var config int				WindowedOffsetY; //NEW (mdf) windowed mode offset support
	var config int				FullscreenViewportX;
	var config int				FullscreenViewportY;
	var config float			Brightness;
	var config float			Contrast;
	var config float			Gamma;
	var config float			MouseSpeed;					// GUI Mouse Speed
	var config float			MinDesiredFrameRate;
	var config byte				ParticleDensity;
	var config byte				ParticleSafeMode;
	var config ETextureDetail	TextureDetailDefault;
	var config ETextureDetail	TextureDetailWorld;
	var config ETextureDetail	TextureDetailPlayerSkin;
	var config ETextureDetail	TextureDetailWeaponSkin;
	var config ETextureDetail	TextureDetailTerrain;
	var config ETextureDetail	TextureDetailInterface;
	var config ETextureDetail	TextureDetailRenderMap;
	var config ETextureDetail	TextureDetailLightmap;
	var        Viewport			LastCurrent;				// The viewport that was last current

defaultproperties
{
	MouseSpeed=1.5
	WindowedOffsetX=-1
	WindowedOffsetY=-1
}
