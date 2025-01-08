//=============================================================================
// Directional sunlight
//=============================================================================
class Sunlight extends Light;

#exec Texture Import File=Textures\SunIcon.pcx  Name=SunIcon Mips=Off MASKED=1

defaultproperties
{
	Texture=SunIcon
	bDirectional=True
	LightEffect=LE_Sunlight
}
