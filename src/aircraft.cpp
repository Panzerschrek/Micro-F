#include "micro-f.h"
#include "aircraft.h"

#include "mf_math.h"

static const float g_roll_rotate_speed=  1.0f;
static const float g_pitch_rotate_speed= 2.0f;
static const float g_yaw_rotate_speed=   0.5f;

static const float g_gravitation[]= { 0.0f, 0.0f, -9.8f }; // m / s^2
static const float g_air_weight_destiny= 1.225f; // kg / m^3

static float AngleOfAttackToLiftForceK( float angle )
{
	angle*= MF_RAD2DEG;
	const float critical_angle= 16.5f;
	if( angle > critical_angle )
	{
		//TODO: make some math here
		angle= critical_angle;
	}
	float k= 0.0615384615f * angle + 0.1846153846f;
	if( k < 0.0f)
		k= 0.0f;
	return k;
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

	// mass/wings_area/velocity/trust/ - parameters of I-5 aircraft (Soviet Union, 1930)
	max_engines_trust_= 5000.0f;
	wings_area_= 21.25f;
	wings_span_length_= 10.24f;
	fuselage_length_= 6.78f;
	fiselage_diameter_= 0.7f;

	mass_= 1355.0f;

	inertia_moment_[0]= 0.25f * ( fiselage_diameter_ * fiselage_diameter_ * 0.25f ) +
		(1.0f/12.0f) * fuselage_length_ * fuselage_length_;
	inertia_moment_[1]= 0.5f * ( fiselage_diameter_ * fiselage_diameter_ * 0.25f );
	inertia_moment_[2]= inertia_moment_[0];

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

		float engines_acceleration_vec_[3];
		Vec3Mul( axis_[1], max_engines_trust_ * throttle_ / mass_, engines_acceleration_vec_ );
		Vec3Add( acceleration_, engines_acceleration_vec_ );

		float lift_force_basis_vec[3];
		Vec3Cross( axis_[0], velocity_, lift_force_basis_vec );
		Vec3Normalize( lift_force_basis_vec );

		float angle_of_attack_dot= Vec3Dot( lift_force_basis_vec, axis_[2] );
		if( angle_of_attack_dot > 0.9999f ) angle_of_attack_dot= 0.9999f;
		else if( angle_of_attack_dot< -0.9999f ) angle_of_attack_dot= -0.9999f;
		float angle_of_attack= mf_Math::acos( angle_of_attack_dot );
		{
			float tmp_vec[3];
			Vec3Sub( lift_force_basis_vec, axis_[2], tmp_vec );
			if( Vec3Dot(tmp_vec, axis_[1]) < 0.0f )
				angle_of_attack= -angle_of_attack;
		}
		float lift_force_k_cy = AngleOfAttackToLiftForceK( angle_of_attack );

		float vel2= Vec3Dot( velocity_, velocity_ );

		float lift_force= 0.5f * lift_force_k_cy * g_air_weight_destiny * vel2 * wings_area_;
		float lift_acceleration_vec[3];
		Vec3Mul( lift_force_basis_vec, lift_force / mass_, lift_acceleration_vec );
		Vec3Add( acceleration_, lift_acceleration_vec );

		float normalized_velocity_vec[3];
		VEC3_CPY( normalized_velocity_vec, velocity_ );
		Vec3Normalize( normalized_velocity_vec );
		float c= 0.0f;
		{
			const float cx0= 0.05f; // constant drag k
			float lambda= wings_span_length_ * wings_span_length_ / wings_area_;
			c= cx0 + lift_force_k_cy / ( 1.4f * lambda );
		}
		float wings_drag_force= 0.5f * c * vel2 * g_air_weight_destiny * wings_area_;
		float wings_drag_acceleration_vec[3];
		Vec3Mul( normalized_velocity_vec, -wings_drag_force/ mass_, wings_drag_acceleration_vec );
		Vec3Add( acceleration_, wings_drag_acceleration_vec );

#ifdef MF_DEBUG
		debug_angle_of_attack_deg_= angle_of_attack * MF_RAD2DEG;
		debug_cyk_= lift_force_k_cy;
#endif
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