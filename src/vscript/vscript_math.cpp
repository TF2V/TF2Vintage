#include "mathlib/mathlib.h"
#include "vscript/ivscript.h"

//=============================================================================
//
// matrix3x4_t
// 
//=============================================================================

matrix3x4_t const& ScriptConcatTransforms( matrix3x4_t const &mat1, matrix3x4_t const &mat2 )
{
	static matrix3x4_t result;
	ConcatTransforms( mat1, mat2, result );
	return result;
}

matrix3x4_t const& ScriptMatrixCopy( matrix3x4_t const &mat )
{
	static matrix3x4_t result;
	MatrixCopy( mat, result );
	return result;
}

matrix3x4_t const& ScriptMatrixInvert( matrix3x4_t mat )
{
	static matrix3x4_t result;
	MatrixInvert( mat, result );
	return result;
}

bool ScriptMatricesAreEqual( matrix3x4_t const &mat1, matrix3x4_t const &mat2 )
{
	return MatricesAreEqual( mat1, mat2 );
}

const Vector& ScriptMatrixGetColumn( matrix3x4_t const &mat, int column )
{
	static Vector outvec;
	outvec.Zero();
	MatrixGetColumn( mat, column, outvec );
	return outvec;
}

matrix3x4_t const& ScriptMatrixSetColumn( const Vector& vecset, int column, matrix3x4_t const& mat )
{
	static matrix3x4_t result;
	MatrixCopy( mat, result );
	MatrixSetColumn( vecset, column, result );
	return result;
}

QAngle const& ScriptMatrixAngles( matrix3x4_t const& mat )
{
	static QAngle result;
	MatrixAngles( mat, result );
	return result;
}

matrix3x4_t const& ScriptAngleMatrix( const QAngle& angset )
{
	static matrix3x4_t result;
	AngleMatrix( angset, result );
	return result;
}

matrix3x4_t const& ScriptAngleIMatrix( const QAngle& angset )
{
	static matrix3x4_t result;
	AngleIMatrix( angset, result );
	return result;
}

matrix3x4_t const& ScriptGetIdentityMatrix( void )
{
	static matrix3x4_t result{
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f
	};
	return result;
}

matrix3x4_t const& ScriptSetScaleMatrix( float x, float y, float z )
{
	static matrix3x4_t result;
	SetScaleMatrix( x, y, z, result );
	return result;
}

//=============================================================================
//
// Quaternion
// 
//=============================================================================

Quaternion const& ScriptQuaternionAdd( Quaternion const &p, Quaternion const &q )
{
	static Quaternion result;
	QuaternionAdd( p, q,  result );
	return result;
}

Quaternion const& ScriptMatrixQuaternion( matrix3x4_t const &mat )
{
	static Quaternion result;
	MatrixQuaternion( mat, result );
	return result;
}

matrix3x4_t const& ScriptQuaternionMatrix( Quaternion const &q )
{
	static matrix3x4_t result;
	QuaternionMatrix( q, result );
	return result;
}

QAngle const& ScriptQuaternionAngles( Quaternion const &q )
{
	static QAngle result;
	QuaternionAngles( q, result );
	return result;
}

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

	// 
	// matrix3x4_t
	// 
	ScriptRegisterFunctionNamed( pVM, ScriptConcatTransforms, "ConcatTransforms", "Concatenates two transformation matrices into another matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatrixCopy, "MatrixCopy", "Copies a matrix to another matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatrixInvert, "MatrixInvert", "Inverts a matrix and copies the result to another matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatricesAreEqual, "MatricesAreEqual", "Checks if two matrices are equal." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatrixGetColumn, "MatrixGetColumn", "Gets the column of a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatrixSetColumn, "MatrixSetColumn", "Sets the column of a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatrixAngles, "MatrixAngles", "Gets the angles and position of a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptAngleMatrix, "AngleMatrix", "Sets the angles and position of a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptAngleIMatrix, "AngleIMatrix", "Sets the inverted angles and position of a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptGetIdentityMatrix, "GetIdentityMatrix", "Returns an identity matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptSetScaleMatrix, "SetScaleMatrix", "Scales a matrix." );

	// 
	// Quaternion
	// 
	ScriptRegisterFunctionNamed( pVM, ScriptQuaternionAdd, "QuaternionAdd", "Adds two quaternions together into another quaternion." );
	ScriptRegisterFunctionNamed( pVM, ScriptMatrixQuaternion, "MatrixQuaternion", "Converts a matrix to a quaternion." );
	ScriptRegisterFunctionNamed( pVM, ScriptQuaternionMatrix, "QuaternionMatrix", "Converts a quaternion to a matrix." );
	ScriptRegisterFunctionNamed( pVM, ScriptQuaternionAngles, "QuaternionAngles", "Converts a quaternion to angles." );
}