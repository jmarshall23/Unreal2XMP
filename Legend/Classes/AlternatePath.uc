//=============================================================================
// AlternatePath.
//=============================================================================
class AlternatePath extends NavigationPoint
	notplaceable;

var() byte Team;
var() float SelectionWeight;
var() bool bReturnOnly;

defaultproperties
{
	bObsolete=true
	SelectionWeight=+1.0000
}
