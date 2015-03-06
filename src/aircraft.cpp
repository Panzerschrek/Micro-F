#include "micro-f.h"
#include "aircraft.h"

#include "mf_math.h"

mf_Aircraft::mf_Aircraft()
	: pitch_factor_(0.0f)
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
	Vec3Mul( axis_[0], pitch_factor_, axis_rotate_vec[0] );
	Vec3Mul( axis_[1], roll_factor_ , axis_rotate_vec[1] );
	Vec3Mul( axis_[2], yaw_factor_  , axis_rotate_vec[2] );

	float rotate_vec[3];
	Vec3Add( axis_rotate_vec[0], axis_rotate_vec[1], rotate_vec );
	Vec3Add( rotate_vec, axis_rotate_vec[2] );

	float vec_len= Vec3Len(rotate_vec);
	float rot_angle= vec_len * 0.3f * dt;

	float final_rotate_vec[3];
	{
		float axis_mat[16];
		Mat4Identity( axis_mat );
		VEC3_CPY( &axis_mat[0], axis_[0] );
		VEC3_CPY( &axis_mat[4], axis_[1] );
		VEC3_CPY( &axis_mat[8], axis_[2] );

		float rotate_vec_to_world_space_rotate_mat[16];
		Mat4Identity( rotate_vec_to_world_space_rotate_mat );
		Mat4Invert( axis_mat, rotate_vec_to_world_space_rotate_mat );
		Vec3Mat4Mul( rotate_vec, rotate_vec_to_world_space_rotate_mat, final_rotate_vec );
	}

	if ( vec_len > 0.01f )
	{
		float rotate_mat[16];
		Mat4RotateAroundVector( rotate_mat, final_rotate_vec, rot_angle );

		Vec3Mat4Mul( axis_[0], rotate_mat );
		Vec3Mat4Mul( axis_[1], rotate_mat );
		Vec3Mat4Mul( axis_[2], rotate_mat );
	}
}