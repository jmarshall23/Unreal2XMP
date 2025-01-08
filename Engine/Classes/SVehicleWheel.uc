class SVehicleWheel extends Object
	native;

// INPUT
var()					float	Steer; // degrees

var()					float	DriveForce; // resultant linear driving force at wheel center
var()					float	LongFriction; // maximum linear longitudinal (roll) friction force
var()					float	LatFriction; // maximum linear longitudinal (roll) friction force
var()					float	LongSlip;
var()					float	LatSlip;
var()					float	ChassisTorque; // Torque applied back to the chassis (equal-and-opposite) from this wheel.

var()					float	TrackVel; // Linear velocity of 'track' at this wheel (unreal scale).

// PARAMS
var()					bool	bPoweredWheel;
var()					bool	bHandbrakeWheel;
var()					bool	bTrackWheel; // If this is a track segment instead of a normal wheel.

var()					enum	ESteerType
{
	ST_Fixed,
	ST_Steered,
	ST_Inverted
} SteerType; // How steering affects this wheel.

var()					name	BoneName;
var()					vector	BoneOffset; // Offset from wheel bone to line check point (middle of tyre). NB: Not affected by scale.
var()					float	WheelRadius; // Length of line check. Usually 2x wheel radius.

var()					float	Softness;
var()					float	PenScale;
var()					float	Restitution;
var()					float	Adhesion;
var()					float	WheelInertia;
var()					float	SuspensionTravel;
var()					float   SuspensionOffset;
var()					float	HandbrakeSlipFactor;
var()					float	HandbrakeFrictionFactor;

var()					name	SupportBoneName; // Name of strut etc. that will be rotated around local X as wheel goes up and down.
var()					EAxis	SupportBoneAxis; // Local axis to rotate support bone around.

// Approximations to Pacejka's Magic Formula
var()					InterpCurve		LongFrictionFunc; // Function of SlipVel (ignored if bTrackWheel)
var()					InterpCurve		LatSlipFunc; // Function of SpinVel (or TrackVel is bTrackWheel)

// OUTPUT

// Calculated on startup
var						vector	WheelPosition; // Wheel center in actor ref frame. Calculated using BoneOffset above.
var						float	SupportPivotDistance; // If a SupportBoneName is specified, this is the distance used to calculate the anglular displacement.

// Calculated each frame
var						bool	bWheelOnGround;
var						float	TireLoad; // Load on tire
var						vector	WheelDir; // Wheel 'forward' in world ref frame. Unit length.
var						vector	WheelAxle; // Wheel axle in world ref frame. Unit length.

var						float	SpinVel; // Radians per sec

var						float   SlipAngle; // Angle between wheel facing direction and wheel travelling direction. In degrees.

var						float	SlipVel;   // Difference in linear velocity between ground and wheel at contact.

var						float	SuspensionPosition; // Output vertical position of wheel
var						float	CurrentRotation;


// Used internally for Karma stuff - DO NOT CHANGE!
var		transient const int		KContact;  

defaultproperties
{
	HandbrakeSlipFactor=1.0
	HandbrakeFrictionFactor=1.0
	Softness=0.05
	PenScale=1.0
	WheelInertia=1.0
	SteerType=ST_Fixed
	SuspensionTravel=50.0
	WheelRadius=35
	SupportBoneAxis=AXIS_X
}
