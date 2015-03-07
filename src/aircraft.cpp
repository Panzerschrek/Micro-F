#include "micro-f.h"
#include "aircraft.h"

#include "mf_math.h"

static const float g_roll_rotate_speed=  1.0f;
static const float g_pitch_rotate_speed= 2.0f;
static const float g_yaw_rotate_speed=   0.5f;

mf_Aircraft::mf_Aircraft( Type type )
	: type_(type)
	, pitch_factor_(0.0f)
	, yaw_factor_  (0.0f)
	, roll_factor_ (0.0f)
{
	axis_[0][0]= 1.0f;
	axis_[0][1]= 0.0f;
	axis_[0][2]= 0.0f;

	axis_[1][0]= 0.0f;
	axis_[1][1]= 1.0f;
	axis_[1][2]= 0.0f;

	axis_[2][0]= 0.0f;
	axis_[2][1]= 0.0f;
	axis_[2][2]= 1.0f;
}

mf_Aircraft::~mf_Aircraft()
{
}

void mf_Aircraft::Tick( float dt )
{
	float axis_rotate_vec[3][3];
	Vec3Mul( axis_[0], pitch_factor_ * g_pitch_rotate_speed, axis_rotate_vec[0] );
	Vec3Mul( axis_[1], roll_factor_  * g_roll_rotate_speed , axis_rotate_vec[1] );
	Vec3Mul( axis_[2], yaw_factor_   * g_yaw_rotate_speed  , axis_rotate_vec[2] );

	float rotate_vec[3];
	Vec3Add( axis_rotate_vec[0], axis_rotate_vec[1], rotate_vec );
	Vec3Add( rotate_vec, axis_rotate_vec[2] );

	float vec_len= Vec3Len(rotate_vec);
	float rot_angle= vec_len * dt;

	if ( vec_len > 0.001f )
	{
		float rotate_mat[16];
		Mat4RotateAroundVector( rotate_mat, rotate_vec, rot_angle );

		// make final rotation
		Vec3Mat4Mul( axis_[0], rotate_mat );
		Vec3Mat4Mul( axis_[1], rotate_mat );
		Vec3Mat4Mul( axis_[2], rotate_mat );

		// make axis orthogonal and with identity vectors
		Vec3Cross( axis_[0], axis_[1], axis_[2] );
		Vec3Cross( axis_[1], axis_[2], axis_[0] );
		Vec3Normalize( axis_[0] );
		Vec3Normalize( axis_[1] );
		Vec3Normalize( axis_[2] );
	}

	Vec3Mul( axis_[1], 50.0f, velocity_ );
	
	float d_pos[3];
	Vec3Mul( velocity_, dt, d_pos );
	Vec3Add( pos_, d_pos );
}