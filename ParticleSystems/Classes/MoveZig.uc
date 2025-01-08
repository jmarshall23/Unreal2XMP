//=============================================================================
// $Author: Aleiby $
// $Date: 11/04/02 12:26a $
// $Revision: 3 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	MoveZig.uc
// Author:	Aaron R Leiby
// Date:	21 April 2001
//------------------------------------------------------------------------------
// Description:	Will move particles 'Zig' units from their starting position.
// (Only applied when particles are first created.)
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class MoveZig extends Force
	native;

var() Range Zig;		// how far to move.

var() vector DirMax;	// direction to use: defaults to velocity direction if not set.
var() vector DirMin;	// (other half of range)

var() bool bRelative;	// Dir relative to ParticleGenerator's rotation?
var() bool bTrace;		// Check to see if anything is in our way and adjust move accordingly.


defaultproperties
{
}

