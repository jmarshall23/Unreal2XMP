class TexUIhook extends TexModifier
	editinlinenew
	native;

cpptext
{
	// UTexModifier interface
	virtual FMatrix* GetMatrix(FLOAT TimeSeconds);
}

var() string ComponentName;
var() bool bRelative;
var() bool bPan,bScale;

var ComponentHandle C;
var rectangle Ri,Rf;
var Matrix M;

defaultproperties
{
	bRelative=true
	bPan=true
	bScale=true
}

