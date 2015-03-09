#pragma once
#include "micro-f.h"

#define MF_PI   3.141592653589793238462643383279f
#define MF_2PI  6.283185307179586476925286766559f
#define MF_PI2  1.570796326794896619231321691639f
#define MF_2PI3 2.094395102393195492308428922186f
#define MF_PI3  1.047197551196597746154214461093f
#define MF_PI4  0.785398163397448309615660845819f
#define MF_PI6  0.523598775598298873077107230546f
#define MF_DEG2RAD 0.017453292519943295769236907684886f
#define MF_RAD2DEG 57.295779513082320876798154814105f

#define VEC3_CPY(dst,src) (dst)[0]= (src)[0]; (dst)[1]= (src)[1]; (dst)[2]= src[2];

void Vec3Mul( const float* v, float s, float* v_out );
void Vec3Mul( float* v, float s );
void Vec3Mul( const float* v0, const float* v1, float* v_out );
void Vec3Add( float* v0, const float* v1 );
void Vec3Add( const float* v0, const float* v1, float* v_dst );
void Vec3Sub( float* v0, const float* v1 );
void Vec3Sub( const float* v0, const float* v1, float* v_dst );
float Vec3Dot( const float* v0, const float* v1 );
void Vec3Cross( const float* v0, const float* v1, float* v_dst );
float Vec3Len( const float* v );
float Distance( const float* v0, const float* v1 );
void Vec3Normalize( float* v );

void SphericalCoordinatesToVec( float longitude, float latitude, float* out_vec );
void VecToSphericalCoordinates( const float* vec, float* out_longitude, float* out_latitude );

void Mat4Identity( float* m );
void Mat4Transpose( float* m );
void Mat4Invert( const float* m, float* out_m );
void Mat4Scale( float* mat, const float* scale );
void Mat4Scale( float* mat, float scale );
void Vec3Mat4Mul( const float* v, const float* m, float* v_dst );
void Vec3Mat4Mul( float* v_dst, const float* m );
void Vec3Mat3Mul( const float* v, const float* m, float* v_dst );
void Vec3Mat3Mul( float* v_dst, const float* m );
void Vec4Mat4Mul( const float* v, const float* m, float* v_dst );
void Mat4Mul( const float* m0, const float* m1, float* m_dst );
void Mat4Mul( float* m0_dst, const float* m1 );
void Mat4RotateX( float* m, float a );
void Mat4RotateY( float* m, float a );
void Mat4RotateZ( float* m, float a );
void Mat4Translate( float* m, const float* v );
void Mat4Perspective( float* m, float aspect, float fov, float z_near, float z_far );
void Mat4RotateAroundVector( float* m, const float* vec, float angle );

float Mat3Det( const float* m );
void Mat4ToMat3( float* m );
void Mat4ToMat3( const float* m_in, float* m_out );


void DoubleMat4Invert( const double* m, double* out_m );

// special namespace for replasement for std::math functions
namespace mf_Math
{

inline float sin( float a )
{
	float r;
	__asm
	{
		fld a
		fsin
		fstp r
	}
	return r;
}

inline float cos( float a )
{
	float r;
	__asm
	{
		fld a
		fcos
		fstp r
	}
	return r;
}

inline float tan( float a )
{
	float r;
	__asm
	{
		fld a
		fptan
		fstp r
		fstp r
	}
	return r;
}

inline float sqrt( float a )
{
	float r;
	__asm
	{
		fld a
		fsqrt
		fstp r
	}
	return r;
}

inline float log( float a )
{
	float r;
	__asm
	{
		fldln2
		fld a
		fyl2x
		fstp r
	}
	return r;
}

inline float fabs( float x )
{
	float r;
	__asm
	{
		fld x
		fabs
		fstp r
	}
	return r;
}

inline float sign( float a )
{
	if( a > 0.0f )
		return 1.0f;
	else if ( a < 0.0f )
		return -1.0f;
	else
		return 0.0f;
}

inline float atan( float a )
{
	float r;
	__asm
	{
		fld a
		fld1
		fpatan
		fstp r
	}
	return r;
}

inline float acos( float x )
{
	return ::acos(x);
}

inline float asin( float x )
{
	return ::asin(x);
}

inline float pow( float x, float y )
{
	return ::pow(x,y);
}

inline float exp( float x )
{
	return ::exp(x);
}

inline float round( float x )
{
	return ::ceilf( x + 0.5f );
}

} // namespace mf_Math

class mf_Rand
{
public:
	mf_Rand( unsigned int seeed= 0 );

	unsigned int Rand();
	float RandF( float max );
	float RandF( float min, float max );
	unsigned int RandI( unsigned int min, unsigned int max );
	unsigned int RandI( unsigned int max );

	static const unsigned int rand_max_;
private:
	unsigned int r;
};