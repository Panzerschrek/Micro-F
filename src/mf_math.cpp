#include "micro-f.h"
#include "mf_math.h"

#define MF_USE_D3DXMATH

#ifdef MF_USE_D3DXMATH
#include <d3dx9math.h>
#endif

namespace Mat4InvertData
{
	static const char sign[]=
	{
		1, -1, 1, -1,
		-1, 1, -1, 1,
		1, -1, 1, -1,
		-1, 1, -1, 1
	};
	static const unsigned char indeces[]=
	{
		5, 6, 7, 9, 10, 11, 13, 14, 15,
		4, 6, 7, 8, 10, 11, 12, 14, 15,
		4, 5, 7, 8,  9, 11, 12, 13, 15,
		4, 5, 6, 8,  9, 10, 12, 13, 14,
	
		1, 2, 3, 9, 10, 11, 13, 14, 15,
		0, 2, 3, 8, 10, 11, 12, 14, 15,
		0, 1, 3, 8,  9, 11, 12, 13, 15,
		0, 1, 2, 8,  9, 10, 12, 13, 14,

		1, 2, 3, 5, 6, 7, 13, 14, 15,
		0, 2, 3, 4, 6, 7, 12, 14, 15,
		0, 1, 3, 4, 5, 7, 12, 13, 15,
		0, 1, 2, 4, 5, 6, 12, 13, 14,

		1, 2, 3, 5, 6, 7, 9, 10, 11,
		0, 2, 3, 4, 6, 7, 8, 10, 11,
		0, 1, 3, 4, 5, 7, 8,  9, 11,
		0, 1, 2, 4, 5, 6, 8,  9, 10
	};
};

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

void Vec3Cross( const float* v0, const float* v1, float* v_dst )
{
#ifdef MF_USE_D3DXMATH
	D3DXVec3Cross( (D3DXVECTOR3*) v_dst, (D3DXVECTOR3*) v0, (D3DXVECTOR3*) v1 );
#else
	v_dst[0]= v0[1] * v1[2] - v0[2] * v1[1];
	v_dst[1]= v0[2] * v1[0] - v0[0] * v1[2];
	v_dst[2]= v0[0] * v1[1] - v0[1] * v1[0];
#endif
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

void SphericalCoordinatesToVec( float longitude, float latitude, float* out_vec )
{
	out_vec[0]= out_vec[1]= mf_Math::cos( latitude );
	out_vec[2]= mf_Math::sin( latitude );

	out_vec[0]*= -mf_Math::sin( longitude );
	out_vec[1]*= +mf_Math::cos( longitude );
}

void VecToSphericalCoordinates( const float* vec, float* out_longitude, float* out_latitude )
{
	*out_latitude= asin(vec[2]);

	float lat_cos2= 1.0f - vec[2] * vec[2];
	float vec_y= vec[1] / mf_Math::sqrt(lat_cos2);

	*out_longitude= acos(vec_y);
	if ( vec[0] > 0.0f ) *out_longitude= MF_2PI - *out_longitude;
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
#ifdef MF_USE_D3DXMATH
	D3DXMatrixIdentity( (D3DXMATRIX*) m );
#else
	unsigned int i= 0;
	for( i= 1; i< 15; i++ )
		m[i]= 0.0f;
	for( i= 0; i< 16; i+=5 )
		m[i]= 1.0f;
#endif
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

static float Mat3Det( const float* m )
{
return
	m[0] * ( m[4] * m[8] - m[5] * m[7] ) -
	m[1] * ( m[3] * m[8] - m[5] * m[6] ) +
	m[2] * ( m[3] * m[7] - m[4] * m[6] );
}

void Mat4Invert( const float* m, float* out_m )
{
#ifdef MF_USE_D3DXMATH
	float det;
	D3DXMatrixInverse( (D3DXMATRIX*) out_m, &det, (D3DXMATRIX*) m );
#else
	float mat3x3[9];

	for( unsigned int i= 0, i9= 0; i< 16; i++, i9+= 9 )
	{
		for( unsigned int j= 0; j< 9; j++ )
			mat3x3[j]= m[ Mat4InvertData::indeces[i9+j] ];
		out_m[i]= float(Mat4InvertData::sign[i]) * Mat3Det(mat3x3);
	}

	float det= out_m[0] * m[0] + out_m[1] * m[1] + out_m[2] * m[2] + out_m[3] * m[3];
	float inv_det= 1.0f / det;

	for( unsigned int i= 0; i< 16; i++ )
		out_m[i]*= inv_det;

	Mat4Transpose(out_m);
#endif
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
#ifdef MF_USE_D3DXMATH
	D3DXVec3TransformCoord( (D3DXVECTOR3*) v_dst, (D3DXVECTOR3*) v, (D3DXMATRIX*) m );
#else
	for( unsigned int i= 0; i< 3; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+4] + v[2] * m[i+8] + m[i+12];
#endif
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

void Vec4Mat4Mul( const float* v, const float* m, float* v_dst )
{
	for( unsigned int i= 0; i< 4; i++ )
		v_dst[i]= v[0] * m[i] + v[1] * m[i+4] + v[2] * m[i+8] + v[3] * m[i+12];
}

void Mat4Mul( const float* m0, const float* m1, float* m_dst )
{
#ifdef MF_USE_D3DXMATH
	D3DXMatrixMultiply( (D3DXMATRIX*)m_dst, (D3DXMATRIX*)m0,(D3DXMATRIX*) m1 );
#else
	unsigned int i, j;

	for( i= 0; i< 4; i++ )
		for( j= 0; j< 16; j+=4 )
		{
			m_dst[ i + j ]=
				m0[ 0 + j ] * m1[ i ]     + m0[ 1 + j ] * m1[ 4 + i ] +
				m0[ 2 + j ] * m1[ 8 + i ] + m0[ 3 + j ] * m1[ 12 + i ];
		}
#endif
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
#ifdef MF_USE_D3DXMATH
	D3DXMatrixRotationX( (D3DXMATRIX*)m, a );
#else
	Mat4Identity(m);
	float s= mf_Math::sin(a), c= mf_Math::cos(a);
	m[5 ]= c;
	m[9 ]= -s;
	m[6 ]= s;
	m[10]= c;
#endif
}

void Mat4RotateY( float* m, float a )
{
#ifdef MF_USE_D3DXMATH
	D3DXMatrixRotationY( (D3DXMATRIX*)m, a );
#else
	Mat4Identity(m);
	float s= mf_Math::sin(a), c= mf_Math::cos(a);
	m[0]= c;
	m[8]= s;
	m[2]= -s;
	m[10]= c;
#endif
}

void Mat4RotateZ( float* m, float a )
{
#ifdef MF_USE_D3DXMATH
	D3DXMatrixRotationZ( (D3DXMATRIX*)m, a );
#else
	Mat4Identity(m);
	float s= mf_Math::sin(a), c= mf_Math::cos(a);
	m[0]= c;
	m[4]= -s;
	m[1]= s;
	m[5]= c;
#endif
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
#ifdef MF_USE_D3DXMATH
	D3DXMatrixPerspectiveFovLH( (D3DXMATRIX*)m, fov, aspect, z_near, z_far );
#else
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
#endif
}

void Mat4RotateAroundVector( float* m, const float* vec, float angle )
{
	float normalized_vec[3];
	VEC3_CPY( normalized_vec, vec );
	Vec3Normalize( normalized_vec );

	float vec_abs[3]= { mf_Math::fabs(vec[0]), mf_Math::fabs(vec[1]), mf_Math::fabs(vec[2]) };

	// get component of vector with max length
	unsigned int max_component= 0;
	float max_component_length= vec_abs[0];
	if ( vec_abs[1] > max_component_length )
	{
		max_component_length= vec_abs[1];
		max_component= 1;
	}
	if ( vec_abs[2] > max_component_length )
	{
		max_component_length= vec_abs[2];
		max_component= 2;
	}
	
	// select second basis vector 
	static const float second_basis_vectors[]=
	{
		0.0f, 1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f
	};
	float second_basis_vector_correctied[3];

	// get third basis vector
	float thirt_basis_vector[3];
	Vec3Cross( normalized_vec, &second_basis_vectors[max_component * 3], thirt_basis_vector );
	// corect second basis vector ( for true orthogonal space )
	Vec3Cross( thirt_basis_vector, normalized_vec, second_basis_vector_correctied );

	float convert_to_vector_space_matrix[16];
	Mat4Identity( convert_to_vector_space_matrix );
	convert_to_vector_space_matrix[ 0]= normalized_vec[0];
	convert_to_vector_space_matrix[ 4]= normalized_vec[1];
	convert_to_vector_space_matrix[ 8]= normalized_vec[2];
	convert_to_vector_space_matrix[ 1]= second_basis_vector_correctied[0];
	convert_to_vector_space_matrix[ 5]= second_basis_vector_correctied[1];
	convert_to_vector_space_matrix[ 9]= second_basis_vector_correctied[2];
	convert_to_vector_space_matrix[ 2]= thirt_basis_vector[0];
	convert_to_vector_space_matrix[ 6]= thirt_basis_vector[1];
	convert_to_vector_space_matrix[10]= thirt_basis_vector[2];

	float rotate_x_matrix[16];
	Mat4RotateX( rotate_x_matrix, angle );

	float invert_transform_matrix[16];
	Mat4Invert( convert_to_vector_space_matrix, invert_transform_matrix );

	float mat[16];
	Mat4Mul( convert_to_vector_space_matrix, rotate_x_matrix, mat );
	Mat4Mul( mat, invert_transform_matrix, m );

	// result matrix = convert_to_svector_space * rotate_x * convert_to_initial_space
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

void DoubleMat4Transpose( double* m )
{
	double tmp;
	for( unsigned int j= 0; j< 3; j++ )
		for( unsigned int i= j; i< 4; i++ )
		{
			tmp= m[i + j*4 ];
			m[ i + j*4]= m[ j + i*4 ];
			m[ j + i*4 ]= tmp;
		}
}

// double matrix functions for terrain generation

static double DoubleMat3Det( const double* m )
{
return
	m[0] * ( m[4] * m[8] - m[5] * m[7] ) -
	m[1] * ( m[3] * m[8] - m[5] * m[6] ) +
	m[2] * ( m[3] * m[7] - m[4] * m[6] );
}

void DoubleMat4Invert( const double* m, double* out_m )
{
	double mat3x3[9];

	for( unsigned int i= 0, i9= 0; i< 16; i++, i9+= 9 )
	{
		for( unsigned int j= 0; j< 9; j++ )
			mat3x3[j]= m[ Mat4InvertData::indeces[i9+j] ];
		out_m[i]= double(Mat4InvertData::sign[i]) * DoubleMat3Det(mat3x3);
	}

	double det= out_m[0] * m[0] + out_m[1] * m[1] + out_m[2] * m[2] + out_m[3] * m[3];
	double inv_det= 1.0f / det;

	for( unsigned int i= 0; i< 16; i++ )
		out_m[i]*= inv_det;

	DoubleMat4Transpose(out_m);
}

// mf_Rand class

const unsigned int mf_Rand::rand_max_= 0x7FFF;

mf_Rand::mf_Rand( unsigned int seed )
	: r(seed)
{
}

unsigned int mf_Rand::Rand()
{
	r= ( ( 22695477 * r + 1 ) & 0x7FFFFFFF );
	return r>>16;
}

float mf_Rand::RandF( float max )
{
	return max * float(Rand()) / float(rand_max_);
}

float mf_Rand::RandF( float min, float max )
{
	return (max - min) * float(Rand()) / float(rand_max_) + min;
}

unsigned int mf_Rand::RandI( unsigned int min, unsigned int max )
{
	return (max - min) * Rand() / rand_max_ + min;
}

unsigned int mf_Rand::RandI( unsigned int max )
{
	return max * Rand() / rand_max_;
}