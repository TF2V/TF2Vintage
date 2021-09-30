#ifndef SQUIRREL_MATH_H
#define SQUIRREL_MATH_H

#ifdef _WIN32
#pragma once
#endif

#define VECTOR_TYPE_TAG		(SQUserPointer)"Vector"
#define QUATERNION_TYPE_TAG	(SQUserPointer)"Quaternion"
#define MATRIX_TYPE_TAG		(SQUserPointer)"matrix3x4_t"

SQRESULT RegisterMathBindings( HSQUIRRELVM pVM );

#endif