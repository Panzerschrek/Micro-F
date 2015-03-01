#include "micro-f.h"
#include "mf_math.h"

void Vec3Mul( const float* v, float s, float* v_out )
{
	v_out[0]= v[0] * s;
	v_out[1]= v[1] * s;
	v_out[2]= v[2] * s;
}

void Vec3Mul( float* v, float s )
{
	v[0]*= s;
	v[1]*= s;
	v[2]*= s;
}

void Vec3Mul( const float* v0, const float* v1, float* v_out )
{
	v_out[0]= v0[0] * v1[0];
	v_out[1]= v0[1] * v1[1];
	v_out[2]= v0[2] * v1[2];
}

void Vec3Add( float* v0, const float* v1 )
{
	v0[0]+= v1[0];
	v0[1]+= v1[1];
	v0[2]+= v1[2];
}

void Vec3Add( const float* v0, const float* v1, float* v_dst )
{
	v_dst[0]= v0[0] + v1[0];
	v_dst[1]= v0[1] + v1[1];
	v_dst[2]= v0[2] + v1[2];
}

void Vec3Sub( float* v0, const float* v1 )
{
	v0[0]-= v1[0];
	v0[1]-= v1[1];
	v0[2]-= v1[2];
}

void Vec3Sub( const float* v0, const float* v1, float* v_dst )
{
	v_dst[0]= v0[0] - v1[0];
	v_dst[1]= v0[1] - v1[1];
	v_dst[2]= v0[2] - v1[2];
}

float Vec3Dot( const float* v0, const float* v1 )
{
	return v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
}

float Vec3Len( const float* v )
{
	return mf_Math::sqrt( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
}

float Distance( const float* v0, const float* v1 )
{
	float v[]= { v0[0], v0[1], v0[2] };
	Vec3Sub( v, v1 );
	return Vec3Len(v0);
}

void Vec3Normalize( float* v )
{
	float l= 1.0f / Vec3Len(v);

	v[0]*= l;
	v[1]*= l;
	v[2]*= l;
}

void Mat4Identity( float* m )
{
	unsigned int i= 0;
	for( i= 1; i< 15; i++ )
		m[i]= 0.0f;
	for( i= 0; i< 16; i+=5 )
		m[i]= 1.0f;
}

void Mat4Transpose( float* m )
{
	float tmp;
	for( unsigned int j= 0; j< 3; j++ )
		for( unsigned int i= j; i< 4; i++ )
		{
			tmp= m[i + j*4 ];
			m[ i + j*4]= m[ j + i*4 ];
			m[ j + i*4 ]= tmp;
		}
}

void Mat4Scale( float* mat, const float* scale )
{
	Mat4Identity(mat);
	mat[0]= scale[0];
	mat[5]= scale[1];
	mat[10]= scale[2];

}

void Mat4Scale( float* mat, float scale )
{
	Mat4Identity(mat);
	mat[0]= scale;
	mat[5]= scale;
	mat[10]= scale;
}

void Vec3Mat4Mul( const float* v, const float* m, float* v_dst )
{
	for( unsigned int i= 0; i< 3; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+4] + v[2] * m[i+8] + m[i+12];
}

void Vec3Mat4Mul( float* v_dst, const float* m )
{
	float v[3];
	for( unsigned int i= 0; i< 3; i++ )
		v[i]= v_dst[i];

	Vec3Mat4Mul( v, m, v_dst );
}

void Vec3Mat3Mul( const float* v, const float* m, float* v_dst )
{
	for( unsigned int i= 0; i< 3; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+3] + v[2] * m[i+6];
}

void Vec3Mat3Mul( float* v_dst, const float* m )
{
	float v[3];
	for( unsigned int i= 0; i< 3; i++ )
		v[i]= v_dst[i];
	
	Vec3Mat3Mul( v, m, v_dst );
}

void Mat4Mul( const float* m0, const float* m1, float* m_dst )
{
	unsigned int i, j;

	for( i= 0; i< 4; i++ )
		for( j= 0; j< 16; j+=4 )
		{
			m_dst[ i + j ]=
				m0[ 0 + j ] * m1[ i ]     + m0[ 1 + j ] * m1[ 4 + i ] +
				m0[ 2 + j ] * m1[ 8 + i ] + m0[ 3 + j ] * m1[ 12 + i ];
		}
}

void Mat4Mul( float* m0_dst, const float* m1 )
{
	float m0[16];
	for( unsigned int i= 0; i< 16; i++ )
		m0[i]= m0_dst[i];

	Mat4Mul( m0, m1, m0_dst );
}


void Mat4RotateX( float* m, float a )
{
	Mat4Identity(m);
	float s= mf_Math::sin(a), c= mf_Math::cos(a);
	m[5 ]= c;
	m[9 ]= -s;
	m[6 ]= s;
	m[10]= c;
}

void Mat4RotateY( float* m, float a )
{
	Mat4Identity(m);
	float s= mf_Math::sin(a), c= mf_Math::cos(a);
	m[0]= c;
	m[8]= s;
	m[2]= -s;
	m[10]= c;
}

void Mat4RotateZ( float* m, float a )
{
	Mat4Identity(m);
	float s= mf_Math::sin(a), c= mf_Math::cos(a);
	m[0]= c;
	m[4]= -s;
	m[1]= s;
	m[5]= c;
}

void Mat4Translate( float* m, const float* v )
{
	Mat4Identity(m);
	m[12]= v[0];
	m[13]= v[1];
	m[14]= v[2];
}

void Mat4Perspective( float* m, float aspect, float fov, float z_near, float z_far )
{
	float f= 1.0f / mf_Math::tan( fov * 0.5f );

	m[0]= f / aspect;
	m[5]= f;

	f= 1.0f / ( z_far - z_near );
	m[14]= -2.0f * f * z_near * z_far;
	m[10]= ( z_near + z_far ) * f;
	m[11]= 1.0f;

	m[1]= m[2]= m[3]= 0.0f;
	m[4]= m[6]= m[7]= 0.0f;
	m[8]= m[9]= 0.0f;
	m[12]= m[13]= m[15]= 0.0f;
}

void Mat4ToMat3( float* m )
{
	for( unsigned int i= 0; i< 3; i++ )
		m[3+i]= m[4+i];
	for( unsigned int i= 0; i< 3; i++ )
		m[6+i]= m[8+i];
}

void Mat4ToMat3( const float* m_in, float* m_out )
{
	for( unsigned int y= 0; y< 3; y++ )
		for( unsigned int x= 0; x< 3; x++ )
			m_out[x + y*3]= m_in[ x + y*4 ];
}

int RandI( int max )
{
	return RandI( 0, max );
}

int RandI( int min, int max )
{
	return (max - min) * rand() / RAND_MAX + min;
}

float RandF( float max )
{
	return RandF( 0.0f, max );
}

float RandF( float min, float max )
{
	return (max - min) * float(rand()) / float(RAND_MAX) + min;
}