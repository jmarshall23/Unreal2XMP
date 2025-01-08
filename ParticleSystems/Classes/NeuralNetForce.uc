//=============================================================================
// $Author: Aleiby $
// $Date: 12/19/01 9:15p $
// $Revision: 1 $
//=============================================================================

//------------------------------------------------------------------------------
// Name:	NeuralNetForce.uc
// Author:	Aaron R Leiby
// Date:	5 June 2001
//------------------------------------------------------------------------------
// Description:	
//------------------------------------------------------------------------------
// How to use this class:
//------------------------------------------------------------------------------

class NeuralNetForce extends Force
	native;

var() bool bLearn;

var() int NumHidden;
var() float Alpha;
var() float Initial;

defaultproperties
{
	Priority=-999
	NumHidden=5
	Alpha=0.5
	Initial=0.1
}
