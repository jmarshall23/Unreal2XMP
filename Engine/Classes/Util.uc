//=============================================================================
// Util.uc
// $Author: Mfox $
// $Date: 12/14/02 8:26p $
// $Revision: 51 $
//=============================================================================
//NEW: file

class Util extends Actor // never instantiated and extending Actor makes things easier
	native 
	abstract;

//=============================================================================
// Low-level utility functions, e.g. string parsing, formatting, generic math
// functions etc.
//=============================================================================

const DefaultGetFloatPrecision	= 3;

//=============================================================================
//@ spawning
//=============================================================================

static function Object SummonClass( class<Object> NewClass, string ClassName, Controller C, Pawn P, bool bUseViewRotation )
{
	local bool bStaticOld;
	local bool bOldColActors, bOldBlockActors, bOldBlockPlayers, bOldbOldCollideWorld;
	local Rotator SummonRotation;
	local vector SpawnLocation;
	local Object Obj;
	
	if( class<Actor>(NewClass) != None )
	{
		// ghost the player first
		if( P != None )
		{
			bOldColActors		= P.bCollideActors;
			bOldBlockActors		= P.bBlockActors;
			bOldBlockPlayers	= P.bBlockPlayers;
			bOldbOldCollideWorld= P.bCollideWorld;
					
			P.SetCollision( false, false, false );
			P.bCollideWorld = false;
		}
		
		if( bUseViewRotation )
		{
			SummonRotation = C.GetViewRotation();
		}
		else
		{
			// only take Yaw from summoner
			SummonRotation = C.Rotation;
			SummonRotation.Roll = 0;
			SummonRotation.Pitch = 0;
		}
		// summon some distance away from and above summoner
		if( class<Actor>(NewClass) != None )
		{
			// make sure static things (e.g. decorations) can be summoned
			bStaticOld = class<Actor>(NewClass).default.bStatic;
			C.SetClassStatic( class<Actor>(NewClass), false );
		}
		if( P != None )
			SpawnLocation = P.Location;
		else
			SpawnLocation = C.Location;
			
		Obj = C.Spawn( class<Actor>(NewClass),,,SpawnLocation + 72 * vector(C.Rotation) + vect(0,0,1) * 15, SummonRotation );

		if( class<Actor>(NewClass) != None )
		{
			C.SetClassStatic( class<Actor>(NewClass), bStaticOld );
		}

		// restore previous collision
		if( P != None )
		{
			P.SetCollision( bOldColActors, bOldBlockActors, bOldBlockPlayers );
			P.bCollideWorld = bOldbOldCollideWorld; 
		}
	}
	else if( NewClass != None )
	{
		// object
		Obj = new NewClass;
	}
	else
	{
		C.DM( "SummonClass: class " $ ClassName $ " not found!" );
	}
	
	return Obj;
}

//=============================================================================
//@ string parsing/processing natives
//=============================================================================

native static final function string GetFloatString( float F, optional byte Precision/*=DefaultGetFloatPrecision*/ );
native static final function string GetVectorString( vector V, optional byte Precision/*=DefaultGetFloatPrecision*/ );
native static final function string GetRotatorString( rotator R, optional byte Precision/*=DefaultGetFloatPrecision*/ );
native static final function bool EndsInDigit( string S );
native static final function TruncateDigits( out string S );
native static final function StripSpaces( out string S, bool bStripLeading, bool bStripTrailing );
native static final function bool ValidNameString( string S );
native static final function bool ValidFloatString( string S );
native static final function bool ValidIntString( string S );
native static final function bool ValidByteString( string S );
native static final function bool ValidBoolString( string S );
native static final function int ParseString( out string S, out string SOut, optional string SDefault );
native static final function int ParseName( out string S, out name NOut, optional bool bAdd, optional name NDefault );
native static final function int ParseFloat( out string S, out float FOut, optional float FDefault );
native static final function int ParseInt( out string S, out int IOut, optional int IDefault );
native static final function int ParseByte( out string S, out byte BOut, optional byte BDefault );
native static final function int ParseBool( out string S, out byte BOut, optional byte BDefault );
native static final function int ParseVector( out string S, out vector VOut, optional vector VDefault );
native static final function string PadString( coerce string S, int PadLen, optional bool bRightJustify, optional string PadStr );

//=============================================================================
//@ string parsing/processing
//=============================================================================

//------------------------------------------------------------------------------
// Returns given time padded out to N characters.

static final function string GetPaddedTime( float Time, int Len, optional bool bRightJustify, optional string PadStr )
{
	return PadString( GetFloatString(Time), Len, bRightJustify, PadStr );
}

//------------------------------------------------------------------------------
// Converts time in seconds into a string representation (hh:mm:ss)

static final function string SecondsToTime( float TimeSeconds, optional bool bNoSeconds )
{
	local int Hours, Minutes, Seconds;
	local string HourString, MinuteString, SecondString;
	
	Seconds = int(TimeSeconds);
	Minutes = Seconds / 60;
	Hours   = Minutes / 60;
	Seconds = Seconds - (Minutes * 60);
	Minutes = Minutes - (Hours * 60);
	
	if( Seconds < 10 )
		SecondString = "0"$Seconds;
	else
		SecondString = string(Seconds);

	if( Minutes < 10 )
		MinuteString = "0"$Minutes;
	else
		MinuteString = string(Minutes);

	if( Hours < 10 )
		HourString = "0"$Hours;
	else
		HourString = string(Hours);

	if( bNoSeconds )
		return HourString$":"$MinuteString;
	else
		return HourString$":"$MinuteString$":"$SecondString;
}

//------------------------------------------------------------------------------

static final function string GetPhysicsString( Actor A )
{
	return EnumStr( enum'EPhysics', A.Physics );
}

//-----------------------------------------------------------------------------

static final function name GetNameSafe( Object O )
{
	if( O != None )
		return O.Name;
	else
		return 'NA';
}

//-----------------------------------------------------------------------------

static function string StripPathFromFileName( string InName )
{
	local string FileName;
	local int Index, InLen, LastPos;

	if( Instr( InName, "\\" ) != -1 )
	{
		// find last slash
		InLen = Len(InName);
		for( Index=0; Index<InLen; Index++ )
		{
			if( Mid(InName, Index, 1) == "\\" )
				LastPos = Index;
		}
		if( LastPos > 0 )
			LastPos++;

		FileName = Mid( InName, LastPos );

		return FileName;
	}
	else
	{
		return InName;
	}
}

//=============================================================================
//@ math
//=============================================================================

/*-----------------------------------------------------------------------------
Solves the quadratic formula given a, b, c (ax^2 + bx + c = 0) with the 
solution(s) given by:

	(-b +/- sqrt( b^2 - 4ac )) / 2a
	
Returns the number of solutions (0, 1 or 2)
*/

static final function int GetQuadraticSolutions( float a, float b, float c, out float Solution1, out float Solution2 )
{
	local float Radical;
	local float RadicalRoot;
	local float TwoA;
	
	// pathological cases
	if( a ~= 0 )
	{
		// bx + c = 0
		if( b ~= 0 )
		{
			// c = 0
			return 0;
		}
		else
		{
			Solution1 = -c / b;
			return 1;
		}
	}
		
	Radical = b*b - 4*a*c;

	if( Radical < 0 )
		return 0;
		
	if( Radical ~= 0 )
	{
		Solution1 = -b / (2*a);
		Solution2 = Solution1;
		return 1;
	}

	RadicalRoot = Sqrt( Radical );
	TwoA= 2*a;
	Solution1 = (-b + RadicalRoot) / TwoA;
	Solution2 = (-b - RadicalRoot) / TwoA;
	return 2;
}

//-----------------------------------------------------------------------------
// Returns true if the given vectors are aproximately equal.
//-----------------------------------------------------------------------------

static final function bool VectorAproxEqual( vector FirstVector, vector SecondVector )
{
	return ( ( FirstVector.x ~= SecondVector.x ) &&
			( FirstVector.y ~= SecondVector.y ) &&
			( FirstVector.z ~= SecondVector.z ) );
}

//-----------------------------------------------------------------------------
// Linearly scales between MinRangeVal/MaxRangeVal given a value and a 
// corresponding range.
//-----------------------------------------------------------------------------
// Val:				value to scale
// RangeMin:		minimum of range for value			(should be <= RangeMax)
// RangeMax:		maximum of range for value
// MinRangeVal:		value to return at min for range
// MaxRangeVal:		value to return at max for range
//-----------------------------------------------------------------------------
// Returns:			scaled value
//-----------------------------------------------------------------------------

static final function float ScaleLinear( float Val, float RangeMin, float RangeMax, float MinRangeVal, float MaxRangeVal )
{
	local float ReturnedVal;
	local float LinearScaleFactor;
	local float DeltaRange, TempFloat;

	if( Val <= RangeMin )
	{
		ReturnedVal = MinRangeVal;
	}
	else if( Val >= RangeMax )
	{
		ReturnedVal = MaxRangeVal;
	}
	else
	{
		if( MaxRangeVal < MinRangeVal )
		{
			TempFloat = MinRangeVal;
			MaxRangeVal = MinRangeVal;
			MinRangeVal = TempFloat;
			DeltaRange = (RangeMax - Val);
		}
		else
		{
			DeltaRange = (Val - RangeMin);
		}

    	LinearScaleFactor = (MaxRangeVal - MinRangeVal) / (RangeMax - RangeMin);
		ReturnedVal	= DeltaRange*LinearScaleFactor + MinRangeVal;
	}

	return ReturnedVal;
}

//-----------------------------------------------------------------------------

static final function ConformRot( out rotator Source, rotator Image, rotator Allowance )
{
	if( Abs(Source.Yaw   - Image.Yaw)   < Allowance.Yaw )   Source.Yaw   = Image.Yaw;
	if( Abs(Source.Pitch - Image.Pitch) < Allowance.Pitch ) Source.Pitch = Image.Pitch;
	if( Abs(Source.Roll  - Image.Roll)  < Allowance.Roll )  Source.Roll  = Image.Yaw;
}

//-----------------------------------------------------------------------------

static final function IncrementRotatorParam( out int Param, int Amount )
{
	Param = (Param + Amount) & 0xFFFF;
}

//-----------------------------------------------------------------------------

static final function bool VectAproxEqual( vector v1, vector v2, float Allowance )
{
	return (	Abs(v1.x-v2.x) < Allowance && 
				Abs(v1.y-v2.y) < Allowance &&
				Abs(v1.z-v2.z) < Allowance );
}

//------------------------------------------------------------------------------
// Sets given rotation parameter (yaw, pitch, roll) to 0 or 32768 depending on
// which value is closer to the original value.

static final function ZeroRotParam( out int Val )
{
	Val = Val & 0xFFFF;

	if( Val < 16384 || Val >= 49152 )
	{
		Val = 0;
	}
	else
	{
		Val = 32768;
	}
}

//------------------------------------------------------------------------------
// Sets given rotation parameter (yaw, pitch, roll) to given value or given value
// plus 32768 depending on which is closer to the original value.

static final function SetLandedRotParam( out int Val, int DesiredVal )
{
	// normalize angle
	Val = Val & 0xFFFF;

	if( abs(Val - DesiredVal) < 16384 || abs(Val - DesiredVal) >= 49152 )
	{
		Val = DesiredVal;
	}
	else
	{
		Val = DesiredVal + 32768;
	}

	// normalize angle
	Val = Val & 0xFFFF;
}

//-----------------------------------------------------------------------------

static final function bool RotationEquivalent( Rotator FirstRotation, Rotator SecondRotation, float RotationComponentTolerance )
{
	local bool bRotationEquivalent;
	local Rotator RotationDifference, AbsRotationDifference;

	RotationDifference = Normalize( SecondRotation - FirstRotation );
	AbsRotationDifference.Roll = abs( RotationDifference.Roll );
	AbsRotationDifference.Pitch = abs( RotationDifference.Pitch );
	AbsRotationDifference.Yaw = abs( RotationDifference.Yaw );
			
	if( ( AbsRotationDifference.Roll <= RotationComponentTolerance ) &&
		( AbsRotationDifference.Pitch <= RotationComponentTolerance ) &&
		( AbsRotationDifference.Yaw <= RotationComponentTolerance ) )
	{
		bRotationEquivalent = true;
	}

	return bRotationEquivalent;
}

//------------------------------------------------------------------------------
// Returns vector perpendicular to given vector, ignoring z component.

static final function vector PerpendicularXY( vector V )
{
	local vector VOut;

	VOut.x = -V.y;
	VOut.y =  V.x;

	return VOut;
}

//------------------------------------------------------------------------------
// Calculated the closest point on the given Actor's collision cylinder
// to the given location.

static final function vector CalcClosestCollisionPoint( Actor Other, vector Loc )
{
	// ripped from UPrimitive::LineCheck.
	// I really should simply expose UPrimitive::LineCheck to UnrealScript instead.
	// This probably can be simplified more since we are assuming the end point is 
	// the colliding actor's location.

	local vector HitNormal;

	local vector Start, End;
	local vector Extent;

	local float TopZ, BotZ;

	local float T0, T1, T;

	local float Kx, Ky;

	local float Vx, Vy;
	local float A, B, C;
	local float Discrim;

	local float Dir;

	local float R2A;

	local float ResultTime;

	if( Other == None )
		return vect(0,0,0);

	HitNormal = vect(0,0,0);

	Start = Loc;
	End = Other.Location;

	Extent.X = Other.CollisionRadius;
	Extent.Y = Other.CollisionRadius;
	Extent.Z = Other.CollisionHeight;

	TopZ = End.Z + Extent.Z;
	BotZ = End.Z - Extent.Z;

	// Clip to top of cylinder.
	T0 = 0.0; 
	T1 = 1.0;
	if( Start.Z > TopZ && End.Z < TopZ )
	{
		T = (TopZ - Start.Z) / (End.Z - Start.Z);
		if( T > T0 )
		{
			T0 = FMax( T0, T );
			HitNormal = vect(0,0,1);
		}
	}
	else if( Start.Z < TopZ && End.Z > TopZ )
	{
		T1 = FMin( T1, (TopZ - Start.Z) / (End.Z - Start.Z) );
	}

	// Clip to bottom of cylinder.
	if( Start.Z < BotZ && End.Z > BotZ )
	{
		T = (BotZ - Start.Z) / (End.Z - Start.Z);
		if( T > T0 )
		{
			T0 = FMax( T0, T );
			HitNormal = vect(0,0,-1);
		}
	}
	else if( Start.Z > BotZ && End.Z < BotZ )
	{
		T1 = FMin( T1, (BotZ - Start.Z) / (End.Z - Start.Z) );
	}

	// Reject.
	if( T0 >= T1 )
		return vect(0,0,0);

	// Test setup.
	Kx = Start.X - End.X;
	Ky = Start.Y - End.Y;

	// 2D circle clip about origin.
	Vx        = End.X - Start.X;
	Vy        = End.Y - Start.Y;
	A         = Vx*Vx + Vy*Vy;
	B         = 2.0 * (Kx*Vx + Ky*Vy);
	C         = Kx*Kx + Ky*Ky - Square(Extent.X);
	Discrim   = B*B - 4.0*A*C;

	// If already inside sphere, oppose further movement inward.
	if( C < Square(1.0) && Start.Z > BotZ && Start.Z < TopZ )
	{
		Dir = ((End - Start) * vect(1,1,0)) dot (Start - End);
		if( Dir < -0.1 )
		{
			HitNormal = Normal((Start - End) * vect(1,1,0));
			return Start;
		}
		else
		{
			return vect(0,0,0);
		}
	}

	// No intersection if discriminant is negative.
	if( Discrim < 0 )
	{
		return vect(0,0,0);
	}

	// Unstable intersection if velocity is tiny.
	if( A < Square(0.0001) )
	{
		// Outside.
		if( C > 0 )
		{
			return vect(0,0,0);
		}
	}
	else
	{
		// Compute intersection times.
		Discrim	= Sqrt(Discrim);
		R2A		= 0.5/A;
		T1		= FMin( T1, (Discrim-B) * R2A );
		T		= -(Discrim+B) * R2A;
		if( T > T0 )
		{
			T0 = T;
			HitNormal	= (Start + (End-Start)*T0 - End);
			HitNormal.Z	= 0;
			HitNormal	= Normal(HitNormal);
		}
		if( T0 >= T1 )
		{
			return vect(0,0,0);
		}
	}
	ResultTime = FClamp( (T0 - 0.001), 0.0, 1.0 );
	return Start + (End-Start) * ResultTime;
}

//-----------------------------------------------------------------------------
// GetDistanceBetweenCylinders:
//
// Return the distance between the 2 given cylinders (origin, radius and
// half-height for each). Can possibly return a negative value?
//-----------------------------------------------------------------------------

static final function float GetDistanceBetweenCylinders(
		vector FirstOrigin, float FirstRadius, float FirstHalfHeight,
 		vector SecondOrigin, float SecondRadius, float SecondHalfHeight )
{
	local float DistanceBetween;
	local vector OriginDifference, OriginDifferenceNormal;
	local vector FirstSurfaceLocation, SecondSurfaceLocation;
		
	//Log( "::Util::GetDistanceBetweenCylinders" );
	OriginDifference = SecondOrigin - FirstOrigin;
	OriginDifference.z = 0;
	OriginDifferenceNormal = Normal( OriginDifference );
		
	FirstSurfaceLocation = FirstOrigin + ( OriginDifferenceNormal * FirstRadius );
	SecondSurfaceLocation = SecondOrigin - ( OriginDifferenceNormal * SecondRadius );
	
	//Log( "::Util::GetDistanceBetweenCylinders: SecondOrigin.z - SecondHalfHeight: " $ SecondOrigin.z - SecondHalfHeight );
	//Log( "::Util::GetDistanceBetweenCylinders: FirstOrigin.z + FirstHalfHeight:   " $ FirstOrigin.z + FirstHalfHeight );
	//Log( "::Util::GetDistanceBetweenCylinders: FirstOrigin.z - FirstHalfHeight:   " $ FirstOrigin.z - FirstHalfHeight );
	//Log( "::Util::GetDistanceBetweenCylinders: SecondOrigin.z + SecondHalfHeight: " $ SecondOrigin.z + SecondHalfHeight );

	if( ( SecondOrigin.z - SecondHalfHeight ) > ( FirstOrigin.z + FirstHalfHeight ) )
	{
		//1st cylinder is above 2nd cylinder 
		//distance is taken from closest point on bottom of 1st cylinder to 
		//closest point on top of 2nd cylinder (within the connecting plane)
		SecondSurfaceLocation.z -= SecondHalfHeight;
		FirstSurfaceLocation.z += FirstHalfHeight;
		DistanceBetween = VSize( FirstSurfaceLocation - SecondSurfaceLocation );
		//Log( "::Util::GetDistanceBetweenCylinders Case 1" );
	}
	else if( ( FirstOrigin.z - FirstHalfHeight ) >	( SecondOrigin.z + SecondHalfHeight ) )
	{
		//1st cylinder is below 2nd cylinder 
		//distance is taken from closest point on top of 1st cylinder to 
		//closest point on bottom of 2nd cylinder (within the connecting plane)
		FirstSurfaceLocation.z -= FirstHalfHeight;
		SecondSurfaceLocation.z += SecondHalfHeight;
		DistanceBetween = VSize( FirstSurfaceLocation - SecondSurfaceLocation );
		//Log( "::Util::GetDistanceBetweenCylinders Case 2" );
	}
	else
	{
		//cylinders are at least partly on the same horizontal plane
		//distance is the distance between the surface locations
		//projected down to the z = 0 plane
		FirstSurfaceLocation.z = 0;
		SecondSurfaceLocation.z = 0;
		DistanceBetween = VSize( FirstSurfaceLocation - SecondSurfaceLocation );
		//Log( "::Util::GetDistanceBetweenCylinders: DistanceBetween: " $ DistanceBetween );

		//if the collision cylinders overlap, DistanceBetween is -ve
		if( VSize( OriginDifference ) < ( FirstRadius + SecondRadius ) )
		{
			DistanceBetween = -DistanceBetween;
		}
		//Log( "::Util::GetDistanceBetweenCylinders Case 3" );
	}
	
	//Log( "::Util::GetDistanceBetweenCylinders FirstOrigin: " $ FirstOrigin );
	//Log( "::Util::GetDistanceBetweenCylinders FirstRadius: " $ FirstRadius );
	//Log( "::Util::GetDistanceBetweenCylinders FirstHalfHeight: " $ FirstHalfHeight );
	//Log( "::Util::GetDistanceBetweenCylinders SecondOrigin: " $ SecondOrigin );
	//Log( "::Util::GetDistanceBetweenCylinders SecondRadius: " $ SecondRadius );
	//Log( "::Util::GetDistanceBetweenCylinders SecondHalfHeight: " $ SecondHalfHeight );
	//Log( "::Util::GetDistanceBetweenCylinders OriginDifference: " $ OriginDifference );
	//Log( "::Util::GetDistanceBetweenCylinders OriginDifferenceNormal: " $ OriginDifferenceNormal );
	//Log( "::Util::GetDistanceBetweenCylinders FirstSurfaceLocation : " $ FirstSurfaceLocation  );
	//Log( "::Util::GetDistanceBetweenCylinders SecondSurfaceLocation: " $ SecondSurfaceLocation );
	//Log( "::Util::GetDistanceBetweenCylinders returning " $ DistanceBetween );

	return DistanceBetween;
}

//-----------------------------------------------------------------------------
// GetDistanceBetweenActors:
// Wrapper for GetDistanceBetweenCylinders for 2 actors.
//-----------------------------------------------------------------------------

static final function float GetDistanceBetweenActors( Actor A1, Actor A2 )
{
	return GetDistanceBetweenCylinders( A1.Location, A1.CollisionRadius, A1.CollisionHeight,
										A2.Location, A2.CollisionRadius, A2.CollisionHeight );
}

/*-----------------------------------------------------------------------------
Returns true if the given line (between LineStart and LineEnd) intersects with
the polygon defined by the vertices in Points and sets IntesectionPoint 
accordingly. 

The vertices in Points are assumed to be in the same plane and the polygon is 
assumed to be convex.
*/

static native final function bool LineConvexPolygonIntersection( vector LineStart, vector LineEnd, array<vector> Points, out vector IntersectionPoint );
static native final function float PointPlaneDistance( vector Point, vector PlaneBase, vector PlaneNormal );
static native final function bool LineBoxIntersection( vector LineStart, vector LineEnd, vector BoxMin, vector BoxMax );

/*-----------------------------------------------------------------------------
Gets the intersection of a sphere centered at SphereCenter with radius
SphereRadius and a line with endpoints at LinePointU and LinePointV. The
function returns true if an intersection occurs. Additionally, the out
parameters IntersectionPoint1 and IntersectionPoint1 are set to the distance
of the intersection points from LinePointU. Otherwise, the function returns
false and leaves IntersectionPoint1 and IntersectionPoint1 unchanged. 

Algorithm stolen from graphics gems I.
*/

static final function bool GetSphereIntersection( out float IntersectionDistance1, out float IntersectionDistance2, vector SphereCenter, float SphereRadius, vector LinePosition, vector LineNormal )
{
	local vector G;
	local float a, b, c;
	local int nIntersections;
	
	G = LinePosition - SphereCenter;
	
	a = LineNormal dot LineNormal;
	b = 2 * ( LineNormal dot G );
	c = ( G dot G ) - Square( SphereRadius );

	nIntersections = class'Util'.static.GetQuadraticSolutions( a, b, c, IntersectionDistance1, IntersectionDistance2 );

	return( nIntersections > 0 );
}

//-----------------------------------------------------------------------------

static final function bool IsLocationInActorSphere( 
	Actor OriginActor,
	vector TestLocation,
	float SphereRadius )
{
	//Log( "::Util::IsLocationInActorSphere" );
	return IsLocationInSphere( OriginActor.Location, SphereRadius, TestLocation );
}

//-----------------------------------------------------------------------------

static final function bool IsLocationInSphere( 
	vector SphereOrigin,
	float SphereRadius,
	vector TestLocation	)
{
	local bool bLocationInActorSphere;
	local vector Difference;
	local float ObjectDistance;
	
	//Log( "::Util::IsLocationInSphere" );
	Difference = TestLocation - SphereOrigin;
	ObjectDistance = VSize( Difference );
	
	//Log( "		ObjectDistance " $ ObjectDistance );
	//Log( "		SphereRadius " $ SphereRadius );
	//is the pawn close enough to the Item location
	bLocationInActorSphere = ( ObjectDistance <= SphereRadius );
	
	//Log( "::Util::IsLocationInSphere returning " $ bLocationInActorSphere );
	return bLocationInActorSphere;
}

//-----------------------------------------------------------------------------

static final function bool IsLocationInRadius( 
	vector Origin,	
	float Radius,
	vector TestLocation )
{
	local vector Difference;
	local float ObjectDistance;
	
	//Log( "::Util::IsLocationInRadius" );
	Difference = TestLocation - Origin;
	Difference .Z = 0;
	ObjectDistance = VSize( Difference );
	//Log( "		ObjectDistance " $ ObjectDistance );
	//Log( "		Radius " $ Radius );
	//is the pawn close enough to the Item location
	return ( ObjectDistance <= Radius );
}

//-----------------------------------------------------------------------------

static final function bool IsLocationInCylinder( 
	vector CylinderOrigin,
	float CylinderRadius,
	float CylinderHeight,
	vector TestLocation )
{
	local bool bLocationInCylinder;
	
	//Log( "::Util::IsLocationInCylinder" );
	if( ( TestLocation.Z >= ( CylinderOrigin.Z - CylinderHeight ) ) &&
			( TestLocation.Z <= ( CylinderOrigin.Z + CylinderHeight ) ) )
	{
		bLocationInCylinder = IsLocationInRadius( CylinderOrigin, CylinderRadius, TestLocation );
	}

	return bLocationInCylinder;
}

//-----------------------------------------------------------------------------

static final function bool IsLocationInActorCylinder( Actor OriginActor, vector TestLocation )
{
	//Log( "::Util::IsLocationInActorCylinder" );
	return IsLocationInCylinder( 
				OriginActor.Location,
				OriginActor.CollisionRadius, 
				OriginActor.CollisionHeight, 
				TestLocation );
}

//-----------------------------------------------------------------------------

static final function bool DoCylindersIntersect( vector Location1, float CollisionRadius1, float CollisionHeight1, vector Location2, float CollisionRadius2, float CollisionHeight2 )
{
	// x/y test
	if( VSize2D( Location1 - Location2 ) > (CollisionRadius1 + CollisionRadius2) )
		return false;
	// z tests
	if( (Location1.Z - CollisionHeight1) > (Location2.Z + CollisionHeight2) )
		return false;
	if( (Location1.Z + CollisionHeight1) < (Location2.Z - CollisionHeight2) )
		return false;
		
	return true;
}

//-----------------------------------------------------------------------------

static final function bool DoActorCylindersIntersect( Actor A1, Actor A2 )
{
	return DoCylindersIntersect( A1.Location, A1.CollisionRadius, A1.CollisionHeight, A2.Location, A2.CollisionRadius, A2.CollisionHeight );
}

//-----------------------------------------------------------------------------
// Shifts the given Actor's location by the given Offset and shifts the
// actor's mesh in the opposite direction (so the Actor doesn't appear to move).
// Useful with carcasses to line up CC with the mesh for example.

static final function bool ShiftCollisionCylinder( Actor SourceActor, float HeightChange )
{
	SourceActor.PrePivot.Z -= HeightChange;
	return SourceActor.SetLocation( SourceActor.Location + vect(0,0,1) * HeightChange );
}

//-----------------------------------------------------------------------------
// Sets the given Actor's collision cylinder to the given Radius, Height then
// shifts the actor's mesh in the opposite direction so this remains lined up
// with the bottom of the CC and sets the CC location so that the bottom of
// the CC remains at the same height. Assumes that the new CC can fit in the 
// current (adjusted) location.

static final function bool AdjustCollisionSize( Actor SourceActor, float NewRadius, float NewHeight )
{
	local bool bResult;
	local float OldHeight, OldRadius, OldPrePivotZ;

	// have to adjust collision size first or setlocation could fail
	bResult = false;
	OldHeight = SourceActor.CollisionHeight;
	OldRadius = SourceActor.CollisionRadius;
	OldPrePivotZ = SourceActor.PrePivot.Z;
	if( SourceActor.SetCollisionSize( NewRadius, NewHeight ) )
	{
		bResult = ShiftCollisionCylinder( SourceActor, (NewHeight - OldHeight) );
		if( !bResult )
		{
			SourceActor.SetCollisionSize( OldRadius, OldHeight ); 
			SourceActor.PrePivot.Z = OldPrePivotZ;
		}
	}

	return bResult;
}

//-----------------------------------------------------------------------------
// Determines whether or not the given actor will telefrag another Actor  if
// it is placed at the given location.
// (Note: This is not the fastest function in the world.)
//------------------------------------------------------------------------------

static final function bool ActorFits( Actor MovingActor, vector DesiredLocation, float ActorFitsRadius )
{
	local Actor IterA;
	local float RadiusDiff, HeightDiff;
	local vector Diff;
	local bool bFits;

	// Filter out bogus data.
	if( MovingActor == None )
	{
		return false;	// Should this return true?  Since a non-existance Actor could theretically fit anywhere - if you could move it.
	}

	bFits = true;

	// Check all blocking actors for overlapping collision cylinders.
	if( MovingActor.bBlockActors || MovingActor.bBlockPlayers )
	{
		foreach MovingActor.RadiusActors(class'Actor', IterA, ActorFitsRadius, DesiredLocation )
		{
			if( IterA != MovingActor && !IterA.IsA( 'Mover' ) )
			{
				if( IterA.bBlockActors || IterA.bBlockPlayers )
				{
					Diff = IterA.Location - DesiredLocation;
					HeightDiff = Diff.z;
					Diff.z = 0;
					RadiusDiff = VSize( Diff );

					if
					(	IterA.CollisionRadius + MovingActor.CollisionRadius >= RadiusDiff	// Using >= to be safe.  > is probably sufficient.
					&&	IterA.CollisionHeight + MovingActor.CollisionHeight >= HeightDiff
					)
					{
						bFits = false;
						break;	// No need to go on.
					}
				}
			}
		}
	}

	return bFits;
}

//=============================================================================
//@ randomness
//=============================================================================

//-----------------------------------------------------------------------------
// Randomly modifies the given float by +/- given %.
//
// e.g. PerturbFloatPercent( 100.0, 20.0) will return a value in 80.0..120.0
//-----------------------------------------------------------------------------

static final function float PerturbFloatPercent( float Num, float PerturbPercent )
{
	local float Perturb;

	Perturb = 2.0*PerturbPercent / 100.0;

	return Num + Num * ( ( Perturb * FRand() - Perturb / 2.0 ) );
}

//-----------------------------------------------------------------------------
// Randomly modifies the given int by +/- given #.
//
// e.g. PerturbInt( 100, 20) will return a value in 80..120
//-----------------------------------------------------------------------------

static final function int PerturbInt( int Num, int PerturbPlusMinus )
{
	return Num + Rand( 2*PerturbPlusMinus +  1 ) - PerturbPlusMinus;
}

//-----------------------------------------------------------------------------
// Randomize given rotator param by +/- the given range, wrapping if necessary.
//-----------------------------------------------------------------------------

static final function RandomizeRotatorParam( out int Param, int PerturbMin, int PerturbMax )
{
	//Param = (Param + Rand( 2*(PerturbMax-PerturbMin) + 1 ) - (PerturbMax-PerturbMin)) & 0xFFFF;

	if( FRand() < 0.5 )
		Param = (Param - Rand( PerturbMax-PerturbMin ) - PerturbMin) & 0xFFFF;
	else
		Param = (Param + Rand( PerturbMax-PerturbMin ) + PerturbMin) & 0xFFFF;
}

//-----------------------------------------------------------------------------
// Randomize a rotator's members +/- over the given range.
//-----------------------------------------------------------------------------

static final function RandomizeRotator( 
	out rotator TargetRotator,
	int RollPerturbMin, 
	int RollPerturbMax, 
	int YawPerturbMin, 
	int YawPerturbMax, 
	int PitchPerturbMin,
	int PitchPerturbMax )
{
/* testing
	local rotator OldRotator;
	local int YawChange, PitchChange;

	OldRotator = TargetRotator;

	Log( "RandomizeRotator" );
	Log( "  RollPerturbMin:  " $ RollPerturbMin );
	Log( "  RollPerturbMax:  " $ RollPerturbMax );
	Log( "  YawPerturbMin:   " $ YawPerturbMin );
	Log( "  YawPerturbMax:   " $ YawPerturbMax );
	Log( "  PitchPerturbMin: " $ PitchPerturbMin );
	Log( "  PitchPerturbMax: " $ PitchPerturbMax );
*/

	RandomizeRotatorParam( TargetRotator.Roll,  RollPerturbMin, RollPerturbMax  );
	RandomizeRotatorParam( TargetRotator.Yaw,   YawPerturbMin, YawPerturbMax );
	RandomizeRotatorParam( TargetRotator.Pitch, PitchPerturbMin, PitchPerturbMax );

/* testing
	YawChange	= TargetRotator.Yaw - OldRotator.Yaw;
	PitchChange	= TargetRotator.Pitch - OldRotator.Pitch;

	if( YawChange > 32768 )
		YawChange = 65535 - YawChange;
	if( PitchChange > 32768 )
		PitchChange = 65535 - PitchChange;

	Log( "Yaw in:    " $ OldRotator.Yaw );
	Log( "Yaw out:   " $ TargetRotator.Yaw );
	Log( "Change:    " $ YawChange );
	
	Log( "Pitch in:  " $ OldRotator.Pitch );
	Log( "Pitch out: " $ TargetRotator.Pitch );
	Log( "Change:    " $ PitchChange );
*/
}

//-----------------------------------------------------------------------------
// Calculate a direction vector based on a general direction, and the amount of
// spread (in degrees) allowed.
//-----------------------------------------------------------------------------

static final function vector CalcSprayDirection( rotator Direction, float Spread )
{
	local float Radius, ZDelta, YDelta;
	local vector X, Y, Z, Offset;

	Radius = Tan( Spread / 2.0 * DegreesToRadians );
	ZDelta = Radius - (2 * Radius * FRand());
	YDelta = Radius - (2 * Radius * FRand());
	
	GetAxes( Direction, X, Y, Z );
	
	Offset = (ZDelta * Z) + (YDelta * Y);
	
	return vector(Direction) + Offset;
}

//=============================================================================
//@ tracing
//=============================================================================

//-----------------------------------------------------------------------------
// Recursively traces until we hit something (BSP or Actor). Returns hit 
// BSP/Actor or None if reached limit.
//------------------------------------------------------------------------------

native static final function Actor TraceRecursive
(	Actor			Instance,					// instance Actor to call trace from (pass in any Actor - Self is usually a good choice)
	out vector		HitLocation,				// location of hit BSP/Actor
	out vector		HitNormal,					// hit normal
	vector			StartLoc,					// location to start tracing from
	optional bool 	bTraceActors,				// (false) whether to trace actors
	optional float	TraceInterval,				// (500) how far to trace per iteration
	optional vector	TraceDirection,				// (down) direction to trace in
	optional float	TraceLimit,					// (infinite) how far to trace before giving up
	optional vector Extent,						// (0,0,0) extents
	optional int	TraceFlags,					// (TRACE_AllColliding | TRACE_ProjTargets) trace flags
	optional bool	bTraceVisibleNonColliding	// (false) whether to trace visible, non-colliding actors (e.g. debug traces)
);

//-----------------------------------------------------------------------------
// Recursively calls SingleLineCheck until we hit something.
//-----------------------------------------------------------------------------

/*!!mdf-tbd: not included because this has a dependency on the CheckResult 
  struct which I'd rather not merge. If code which depends on this gets
  merged in, try to find a work-around, merge CheckResult only if needed.

native static final function bool RSLC
(	out CheckResult	Hit,
	Actor			Source,
	vector			Start,
	optional vector	TraceDirection,
	optional float	TraceInterval,
	optional float	TraceLimit,
	optional int	TraceFlags,
	optional vector	Extent
);
*/

/*-----------------------------------------------------------------------------
GetHitActorInfo:

Traces from given PlayerPawn's eyes along his ViewRotation and returns what
was hit first along with the corresponding HitLocation and HitNormal.

If bTraceVisibleNonColliding is set trace will also "hit" visible actors which 
aren't normally hit because they aren't in the collision hash (don't collide)
or because ShouldTrace returns false (e.g. pickups). This is only really useful
for debug traces and should be avoided otherwise at this can be slow.
*/

static final function Actor GetHitActorInfo( Controller C, out vector HitLocation, out vector HitNormal, optional bool bTraceVisibleNonColliding )
{					 
	local vector StartTrace;
	local Actor HitActor;

	if( C != None )
	{
		if( C.Pawn != None )
		{
			StartTrace		= C.Pawn.Location; 
			StartTrace.Z   += C.Pawn.BaseEyeHeight;
		}
		else
		{
			StartTrace		= vect(0,0,0);
		}

		HitActor = TraceRecursive( C, HitLocation, HitNormal, StartTrace, true, 0.0, vector(C.GetViewRotation()), , , , bTraceVisibleNonColliding );
	}

	return HitActor;		
}

//-----------------------------------------------------------------------------

static final function Actor GetHitActor( Controller C, optional bool bTraceVisibleNonColliding )
{					 
	local vector HitLocation, HitNormal;

	return GetHitActorInfo( C, HitLocation, HitNormal, bTraceVisibleNonColliding );
}

//-----------------------------------------------------------------------------

static final function bool TraceFromActor( 
		Actor GivenActor,
		vector DestinationPoint,
		optional bool bCollideActors,
		optional bool bUseExtents )
{
	local vector TraceHitLocation, TraceHitNormal, GivenActorExtents;
	local Actor TraceHitActor;
	//Log( "::Util::TraceFromActor" );
	
	if( bUseExtents )
	{
		GivenActorExtents.x = GivenActor.CollisionRadius;
		GivenActorExtents.y = GivenActor.CollisionRadius;
		GivenActorExtents.z = GivenActor.CollisionHeight;
	}

	TraceHitActor = GivenActor.Trace( TraceHitLocation, TraceHitNormal,
			DestinationPoint, GivenActor.Location,  bCollideActors, GivenActorExtents );
	
	if( TraceHitActor != none )
	{
		//Log( "::Util::TraceFromActor TraceHitActor: " $ TraceHitActor );
	}
	
	return TraceHitActor != None;
}

//-----------------------------------------------------------------------------

static function vector GetLocationAtActor( Actor TargetActor, float SourceCollisionRadius, float SourceCollisionHeight )
{
	local vector ReturnedLocation;
	
	ReturnedLocation = TargetActor.Location;
	ReturnedLocation.Z += (SourceCollisionHeight - TargetActor.CollisionHeight) + CCFloatHeight;
	if( SourceCollisionRadius > TargetActor.CollisionRadius )
		ReturnedLocation.Z += (SourceCollisionRadius - TargetActor.CollisionRadius);
		
	return ReturnedLocation;
}

//=============================================================================
//@ various
//=============================================================================

//-----------------------------------------------------------------------------
// Note: if current skin isn't the default one, won't override this with a 
// subsequent non-default skin.

static function SpamTextures( Actor TargetActor, Material Tex )
{
	local int ii;

	//TargetActor.DMTN( "SpamTextures: " $ Tex );
	if( TargetActor.Skins.Length == 0 )
		TargetActor.Skins.Length = 6; // should be close enough

	if( Tex == None )
	{
		// restore regular skins
		for( ii=0; ii<TargetActor.Skins.Length; ii++ )
			TargetActor.Skins[ii] = TargetActor.default.Skins[ii];
		TargetActor.Texture = TargetActor.default.Texture;
	}
	else if( TargetActor.Texture == TargetActor.default.Texture )
	{
		// set to given skin
		for( ii=0; ii<TargetActor.Skins.Length; ii++ )
			TargetActor.Skins[ii] = Tex;
		TargetActor.Texture = Tex;
	}
}

//-----------------------------------------------------------------------------

defaultproperties
{
}
//#endif