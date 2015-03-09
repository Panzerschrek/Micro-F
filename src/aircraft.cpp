#include "micro-f.h"
#include "aircraft.h"

#include "mf_math.h"

static const float g_roll_rotate_speed=  1.0f;
static const float g_pitch_rotate_speed= 2.0f;
static const float g_yaw_rotate_speed=   0.5f;

static const float g_gravitation[]= { 0.0f, 0.0f, -9.8f };

static float AngleOfAttackToLiftForceK( float angle )
{
	angle*= MF_RAD2DEG;
	return 0.0615384615f * angle + 0.1846153846f;
}

mf_Aircraft::mf_Aircraft( Type type )
	: type_(type)
	, pitch_factor_(0.0f)
	, yaw_factor_  (0.0f)
	, roll_factor_ (0.0f)
	, throttle_(1.0f)
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

	// mass/wings_area/velocity - parameters of I-5 aircraft (Soviet Union, 1930)
	wings_area_= 10.25f;
	mass_= 1355.0f;
	velocity_[0]= 0.0f;
	velocity_[1]= 70.0f;
	velocity_[2]= 0.0f;
	acceleration_[0]= 0.0f;
	acceleration_[1]= 0.0f;
	acceleration_[2]= 0.0f;
	angular_speed_[0]= 0.0f;
	angular_speed_[1]= 0.0f;
	angular_speed_[2]= 0.0f;
	angular_acceleration_[0]= 0.0f;
	angular_acceleration_[2]= 0.0f;
	angular_acceleration_[3]= 0.0f;
}

mf_Aircraft::~mf_Aircraft()
{
}

void mf_Aircraft::Tick( float dt )
{
	/*float axis_rotate_vec[3][3];
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
*/
	// calculations of position of aircraft before calculations of forces
	{
		float d_pos[3];
		float d_acc_pos[3];
		float half_acceleration[3];
		Vec3Mul( acceleration_, 0.5f * dt * dt, half_acceleration );
		Vec3Mul( velocity_, dt, d_pos );
		Vec3Mul( half_acceleration, dt, d_acc_pos );
		Vec3Add( d_pos, d_acc_pos );
		Vec3Add( pos_, d_pos );

		float d_vel[3];
		Vec3Mul( acceleration_, dt, d_vel );
		Vec3Add( velocity_, d_vel );
	}
	// calculation of forces
	{
		VEC3_CPY( acceleration_, g_gravitation );

		float lift_force_k = AngleOfAttackToLiftForceK( 5.0f * MF_DEG2RAD );
		const float ro= 1.225f;

		float vel2= velocity_[1] * velocity_[1];

		float force= 0.5f * lift_force_k * ro * vel2 * wings_area_;
		acceleration_[2]+= force / mass_;
	}
}

void mf_Aircraft::ThrottleUp( float dt )
{
	const float throttle_up_speed= 0.75f;
	throttle_+= dt * throttle_up_speed;
	if( throttle_ > 1.0f ) throttle_= 1.0f;
}

void mf_Aircraft::ThrottleDown( float dt )
{
	const float throttle_down_speed= 1.0f;
	throttle_-= dt * throttle_down_speed;
	if( throttle_ < 0.0f ) throttle_= 0.0f;
}