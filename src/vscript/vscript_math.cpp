#include "mathlib/mathlib.h"
#include "vscript/ivscript.h"

//=============================================================================
//
// Misc. Vector/QAngle functions
// 
//=============================================================================

const Vector& ScriptAngleVectors( const QAngle &angles )
{
	static Vector forward;
	AngleVectors( angles, &forward );
	return forward;
}

const QAngle& ScriptVectorAngles( const Vector &forward )
{
	static QAngle angles;
	VectorAngles( forward, angles );
	return angles;
}

const Vector& ScriptVectorRotate( const Vector &in, matrix3x4_t const &mat )
{
	static Vector out;
	VectorRotate( in, mat, out );
	return out;
}

const Vector& ScriptVectorIRotate( const Vector &in, matrix3x4_t const &mat )
{
	static Vector out;
	VectorIRotate( in, mat, out );
	return out;
}

const Vector& ScriptVectorTransform( const Vector &in, matrix3x4_t const &mat )
{
	static Vector out;
	VectorTransform( in, mat, out );
	return out;
}

const Vector& ScriptVectorITransform( const Vector &in, matrix3x4_t const &mat )
{
	static Vector out;
	VectorITransform( in, mat, out );
	return out;
}

const Vector& ScriptCalcClosestPointOnAABB( const Vector &mins, const Vector &maxs, const Vector &point )
{
	static Vector outvec;
	CalcClosestPointOnAABB( mins, maxs, point, outvec );
	return outvec;
}

const Vector& ScriptCalcClosestPointOnLine( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	static Vector outvec;
	CalcClosestPointOnLine( point, vLineA, vLineB, outvec );
	return outvec;
}

float ScriptCalcDistanceToLine( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	return CalcDistanceToLine( point, vLineA, vLineB );
}

const Vector& ScriptCalcClosestPointOnLineSegment( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	static Vector outvec;
	CalcClosestPointOnLineSegment( point, vLineA, vLineB, outvec );
	return outvec;
}

float ScriptCalcDistanceToLineSegment( const Vector &point, const Vector &vLineA, const Vector &vLineB )
{
	return CalcDistanceToLineSegment( point, vLineA, vLineB );
}

float ScriptExponentialDecay( float decayTo, float decayTime, float dt )
{
	return ExponentialDecay( decayTo, decayTime, dt );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void RegisterMathBaseBindings( IScriptVM *pVM )
{
	ScriptRegisterFunction( pVM, RandomFloat, "Generate a random floating point number within a range, inclusive." );
	ScriptRegisterFunction( pVM, RandomInt, "Generate a random integer within a range, inclusive." );
	ScriptRegisterFunction( pVM, RandomFloatExp, "Generate an exponential random floating point number within a range, exclusive" );
	ScriptRegisterFunction( pVM, ApproachAngle, "Returns an angle which approaches the target angle from the input angle with the specified speed." );
	ScriptRegisterFunction( pVM, AngleDiff, "Returns the degrees difference between two yaw angles." );
	ScriptRegisterFunction( pVM, AngleNormalize, "Clamps an angle to be in between -360 and 360." );
	ScriptRegisterFunction( pVM, AngleNormalizePositive, "Clamps an angle to be in between 0 and 360." );
	ScriptRegisterFunction( pVM, AnglesAreEqual, "Checks if two angles are equal based on a given tolerance value." );

	// 
	// Misc. Vector/QAngle functions
	// 
	ScriptRegisterFunctionNamed( pVM, ScriptAngleVectors, "AngleVectors", "Turns an angle into a direction vector." );
	ScriptRegisterFunctionNamed( pVM, ScriptVectorAngles, "VectorAngles", "Turns a direction vector into an angle." );

	ScriptRegisterFunctionNamed( pVM, ScriptVectorRotate, "VectorRotate", "Rotates a vector with a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptVectorIRotate, "VectorIRotate", "Rotates a vector with the inverse of a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptVectorTransform, "VectorTransform", "Transforms a vector with a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptVectorITransform, "VectorITransform", "Transforms a vector with the inverse of a matrix." );

	ScriptRegisterFunction( pVM, CalcSqrDistanceToAABB, "Returns the squared distance to a bounding box." );
	ScriptRegisterFunctionNamed( pVM, ScriptCalcClosestPointOnAABB, "CalcClosestPointOnAABB", "Returns the closest point on a bounding box." );
	ScriptRegisterFunctionNamed( pVM, ScriptCalcDistanceToLine, "CalcDistanceToLine", "Returns the distance to a line." );
	ScriptRegisterFunctionNamed( pVM, ScriptCalcClosestPointOnLine, "CalcClosestPointOnLine", "Returns the closest point on a line." );
	ScriptRegisterFunctionNamed( pVM, ScriptCalcDistanceToLineSegment, "CalcDistanceToLineSegment", "Returns the distance to a line segment." );
	ScriptRegisterFunctionNamed( pVM, ScriptCalcClosestPointOnLineSegment, "CalcClosestPointOnLineSegment", "Returns the closest point on a line segment." );

	ScriptRegisterFunction( pVM, SimpleSplineRemapVal, "remaps a value in [startInterval, startInterval+rangeInterval] from linear to spline using SimpleSpline" );
	ScriptRegisterFunction( pVM, SimpleSplineRemapValClamped, "remaps a value in [startInterval, startInterval+rangeInterval] from linear to spline using SimpleSpline" );
	ScriptRegisterFunction( pVM, Bias, "The curve is biased towards 0 or 1 based on biasAmt, which is between 0 and 1." );
	ScriptRegisterFunction( pVM, Gain, "Gain is similar to Bias, but biasAmt biases towards or away from 0.5." );
	ScriptRegisterFunction( pVM, SmoothCurve, "SmoothCurve maps a 0-1 value into another 0-1 value based on a cosine wave" );
	ScriptRegisterFunction( pVM, SmoothCurve_Tweak, "SmoothCurve peaks at flPeakPos, flPeakSharpness controls the sharpness of the peak" );
	ScriptRegisterFunctionNamed( pVM, ScriptExponentialDecay, "ExponentialDecay", "decayTo is factor the value should decay to in decayTime" );
}