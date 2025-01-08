//=============================================================================
// SubActionConsoleCommand:
//
// Calls specified console command.
//=============================================================================
class SubActionConsoleCommand extends MatSubAction
	native;

var(ConsoleCommand)	string	Command;		// The command to call.

defaultproperties
{
	Icon=SubActionConsoleCommand
	Desc="ConsoleCommand"
}
