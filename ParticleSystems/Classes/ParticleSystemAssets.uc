//=============================================================================
// $Author: Sbrown $
// $Date: 2/06/02 2:50p $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	ParticleSystemAssets.uc
// Author:	Aaron R Leiby
// Date:	25 July 2000
//------------------------------------------------------------------------------
// Description:  
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class ParticleSystemAssets extends Object;

#exec OBJ LOAD FILE=Textures\U2Particles.utx PACKAGE=ParticleSystems

#exec AUDIO IMPORT FILE=Sounds\glass_break_02.wav	GROUP=Glass

#exec AUDIO IMPORT FILE=Sounds\bltglass2.wav		GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\bltglass3.wav		GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\bltglass4.wav		GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\bltglass6.wav		GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\bltglass7.wav		GROUP=Glass

#exec AUDIO IMPORT FILE=Sounds\GlassBreak_01.wav GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\GlassBreak_02.wav GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\GlassBreak_03.wav GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\GlassBreak_04.wav GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\GlassBreak_05.wav GROUP=Glass
#exec AUDIO IMPORT FILE=Sounds\GlassBreak_06.wav GROUP=Glass

