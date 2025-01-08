
#include "xForceFeedback.h"

#ifdef WIN32
#define __IFC__
#endif

#ifdef _XBOX
#undef __IFC__
#endif

int GForceFeedbackAvailable = 0;

#ifdef __IFC__
#include "IFC.h"
static CImmDevice *ImmDevice = NULL;
static CImmProject *ImmProject = NULL;

static const char *PickFile( CImmDevice *Device )
{
    // Would be nice to manually set effect set
    const char *FileName = "../ForceFeedback/other.ifr";
    
    if( Device )
    {
        char ProductName[256];
        if( Device->GetProductName( ProductName, sizeof(ProductName) ) )
        {
            if
            (   !stricmp( ProductName, "Logitech iFeel Mouse" )
            ||  !stricmp( ProductName, "Logitech iFeel MouseMan" )
            ){
                FileName = "../ForceFeedback/ifeel.ifr";
            }
        }
    }
    
    return FileName;
}
#endif


// Make sure this is re-entrant!
void InitForceFeedback(void* hInstance, void* hWnd)
{
#ifdef __IFC__
    if( ImmDevice )
        return;

    // To support Joysticks/Gamepads/DXDevices...
    // pass in valid HINSTANCE and HWND
    ImmDevice = CImmDevice::CreateDevice( (HINSTANCE)hInstance, (HWND)hWnd );
    if( !ImmDevice )
        return;

    ImmProject = new CImmProject;
    if( !ImmProject->OpenFile( PickFile( ImmDevice ), ImmDevice ) )
    {
        ExitForceFeedback();
        return;
    }

    PrecacheForceFeedback();  // move elsewhere?

    GForceFeedbackAvailable = 1;
#else
	hInstance;
	hWnd;
    GForceFeedbackAvailable = 0;
#endif
}

void ExitForceFeedback()
{
#ifdef __IFC__
    if( ImmProject )
    {
        delete ImmProject;
        ImmProject = NULL;
    }

    if( ImmDevice )
    {
        delete ImmDevice;
        ImmDevice = NULL;
    }
#endif
	GForceFeedbackAvailable = 0;
}

void PlayFeedbackEffect( const char* EffectName )
{
#ifdef __IFC__
    if( ImmProject )
        ImmProject->Start( EffectName );
#else
    EffectName;
#endif
}

// Pass NULL to stop all
void StopFeedbackEffect( const char* EffectName )
{
#ifdef __IFC__
    if( ImmProject )
        ImmProject->Stop( EffectName );
#else
    EffectName;
#endif
}

void PrecacheForceFeedback()
{
#ifdef __IFC__
    // Cache all compound effects
    for
    (   int i = 0, NumEffects = ImmProject->GetNumEffects()
    ;   i < NumEffects
    ;   i++
    ){
        if( IMM_EFFECTTYPE_COMPOUND == ImmProject->GetEffectType( ImmProject->GetEffectName( i ) ) )
        {
            ImmProject->CreateEffectByIndex( i );
        }
    }
#endif
}

