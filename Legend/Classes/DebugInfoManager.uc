//=============================================================================
// DebugInfoManager.uc
// $Author: Mfox $
// $Date: 9/03/02 9:12p $
// $Revision: 9 $
//=============================================================================

class DebugInfoManager extends Object
	config(User)
	abstract;

/*-----------------------------------------------------------------------------
Abstract base class for HUD mutators which are used to show information on the 
screen for debugging/tracing purposes. Implement actual information gathering 
through the SetInfoStrings function in derived classes (should fill the 
InfoStrings -- first entry must be set, other entries can be empty for spacing).
*/

var config int XOffset;								// X offset for displayed strings
var config int YOffset;								// Y offset for displayed strings
var config bool bConsoleEcho;						// echo display to console
var config bool bLogfileEcho;						// echo display to log file

var array<string> InfoStrings;						// filled in by subclasses
var byte bEnabled;									// 0=disabled, >0 = various levels of "enabled"
var int MaxEnabled;									// subclasses can support multiple levels of "enabledness"

var PlayerController PCOwner;

//-----------------------------------------------------------------------------

function DoDrawText( Canvas Canvas, coerce string Str )
{
	Canvas.DrawText( Str, true );

	if( bConsoleEcho && PCOwner.Player != None && PCOwner.Player.Console != None )
	{
		// echo to console
		PCOwner.Player.Console.Message( Str, 1.0 );
	}
	
	if( bLogfileEcho )
	{
		// echo to log file
		Log( Str );
	}
}

//-----------------------------------------------------------------------------

simulated interface function PostRender( Canvas Canvas )
{
	local int ii;
	local float XL, YL;
	local float XPos, YPos;

	// note: if we don't update with every Canvas painting, the text will
	// "flicker" and the information won't be as accurate (e.g. more state
	// changes etc. might be lost) so we pretty much need to do this for
	// every frame.

	InfoStrings.Length = 0;
	SetInfoStrings();

	if( InfoStrings.Length != 0 )
	{
		Canvas.Font = Canvas.DebugFont;
		Canvas.Style = Canvas.default.Style;

		Canvas.StrLen( "TEST", XL, YL );
		Canvas.DrawColor.R = 0;
		Canvas.DrawColor.G = 255;
		Canvas.DrawColor.B = 0;

		XPos = XOffset;
		YPos = YOffset;
		if( YPos < 0 )
		{
			// if < 0 YOffset is offset from bottom
			YPos = Canvas.SizeY - (YL*InfoStrings.Length + -YPos);
	    }

		for( ii=0; ii<InfoStrings.Length; ii++ )
		{
			Canvas.SetPos( XPos, YPos );
			DoDrawText( Canvas, InfoStrings[ii] );
			YPos+=YL;
		}
	}
}

//-----------------------------------------------------------------------------

function SetInfoStrings();

//-----------------------------------------------------------------------------

function ClearInfoStrings()
{
	InfoStrings.Length = 0;
}

//-----------------------------------------------------------------------------

function SetEchoToConsole( bool bVal )
{
	bConsoleEcho = bVal;
}

//-----------------------------------------------------------------------------

function bool GetEchoToConsole()
{
	return bConsoleEcho;
}

//-----------------------------------------------------------------------------

function SetEchoToLog( bool bVal )
{
	bLogfileEcho = bVal;
}

//-----------------------------------------------------------------------------

function bool GetEchoToLog()
{
	return bLogfileEcho;
}

//-----------------------------------------------------------------------------

function byte GetEnabled()
{
	return bEnabled;
}
		
//-----------------------------------------------------------------------------

function SetEnabled( byte bVal )
{
	if( bEnabled == 0 && bVal != 0 )
	{
		// becoming enabled
		class'UIConsole'.static.RegisterRenderListener( Self, "PostRender" );
	}
	else if( bEnabled != 0 && bVal == 0 )
	{
		// becoming disabled
		class'UIConsole'.static.UnregisterRenderListener( Self, "PostRender" );
	}

	bEnabled = bVal;
}

//-----------------------------------------------------------------------------

function CycleEnabled()
{
	local byte bEnabledNew;

	bEnabledNew = bEnabled;
	bEnabledNew++;
	if( bEnabledNew > MaxEnabled )
		bEnabledNew = 0;

	SetEnabled( bEnabledNew );
}

//-----------------------------------------------------------------------------

function Cleanup()
{
	// make sure unregistered as a render listener
	if( bEnabled != 0 )
		SetEnabled( 0 );
	PCOwner	= None;
}

//-----------------------------------------------------------------------------

function Initialize( PlayerController PC )
{
	PCOwner = PC;
}

//-----------------------------------------------------------------------------

defaultproperties
{
	XOffset=4
	YOffset=136
	MaxEnabled=1
}
