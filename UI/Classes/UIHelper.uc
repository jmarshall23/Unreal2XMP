//=============================================================================
// $Author: Aleiby $
// $Date: 2/15/02 1:07a $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	UIHelper.uc
// Author:	Aaron Leiby
// Date:	23 January 2002
//------------------------------------------------------------------------------
// Description:	Use subclasses of this class to store various UI helper functions.
//------------------------------------------------------------------------------
// Notes:
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class UIHelper extends Object
	native;

//------------------------------------------------------------------------------
native static final function Console			GetConsole();		//{ return class'UIConsole'.static.GetConsole();		}
native static final function UIConsole			GetUIConsole();		//{ return class'UIConsole'.static.GetUIConsole();		}
native static final function PlayerController	GetPlayerOwner();	//{ return class'UIConsole'.static.GetPlayerOwner();	}
native static final function float				GetTimeSeconds();	//{ return class'UIConsole'.static.GetTimeSeconds();	}
native static final function Client				GetClient();		//{ return class'UIConsole'.static.GetClient();			}
native static final function bool ConsoleCommand(string Cmd);		//{ return GetConsole().ConsoleCommand(Cmd);			}


defaultproperties
{
}

