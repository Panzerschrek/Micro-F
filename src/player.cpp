#include "micro-f.h"
#include "player.h"
#include "main_loop.h"

#include "mf_math.h"

#include "mf_model.h"
#include "../models/models.h"

#define MF_FOV_CHANGE_SPEED MF_PI4
#define MF_MIN_FOV MF_PI6
#define MF_MAX_FOV (MF_PI2 + MF_PI6)
#define MF_FOV_STEP (MF_PI6 * 0.25f)
#define MF_INITIAL_FOV (MF_PI2 - MF_FOV_STEP)

#define MF_START_ROCKET_COUNT 3

mf_Player::mf_Player()
	: control_mode_(ModeChooseAircraftType), view_mode_(ViewThirdperson)
	, aircraft_( mf_Aircraft::F2XXX, MF_PLAYER_START_HP )
	, aircraft_to_choose_(mf_Aircraft::F1949)
	, autopilot_(&aircraft_)
	, lifes_(3), is_in_respawn_(false)
	, cam_radius_(10.0f)
	, aspect_(1.0f), fov_(MF_INITIAL_FOV), target_fov_(MF_INITIAL_FOV)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, rotate_clockwise_pressed_(false), rotate_anticlockwise_pressed_(false)
	, score_(0)
	, enemies_count_(0)
	, target_aircraft_(NULL)
{
	pos_[0]= pos_[1]= pos_[2]= 0.0f;
	angle_[0]= angle_[1]= angle_[2]= 0.0f;

	aircraft_.SetPos( pos_ );

	autopilot_.SetMode( mf_Autopilot::ModeKillRotation );
	//autopilot_.SetMode( mf_Autopilot::ModeTurnToAzimuth );
	//autopilot_.SetTargetAzimuth( -MF_PI2 );
	//autopilot_.SetTargetAltitude( /*aircraft_.Pos()[2]*/70.0f );
}

mf_Player::~mf_Player()
{
}

void mf_Player::Tick( float dt )
{
	if( is_in_respawn_)
	{
		if( mf_MainLoop::Instance()->CurrentTime() >= respawn_start_time_ + 4.0f )
		{
			lifes_--;
			SetupInitialAircraftParams();
			is_in_respawn_= false;
		}
		return;
	}

	if( control_mode_ != ModeChooseAircraftType )
	{
		float d_fov= target_fov_ - fov_;
		if( mf_Math::fabs(d_fov) > dt * MF_FOV_CHANGE_SPEED )
			fov_+= dt * MF_FOV_CHANGE_SPEED * mf_Math::sign(d_fov);
		else
			fov_= target_fov_;

		float rotate_vec[]= { 0.0f, 0.0f, 0.0f };
		if(rotate_up_pressed_   ) rotate_vec[0]+=  1.0f;
		if(rotate_down_pressed_ ) rotate_vec[0]+= -1.0f;
		if(rotate_left_pressed_ ) rotate_vec[2]+=  1.0f;
		if(rotate_right_pressed_) rotate_vec[2]+= -1.0f;
		if(rotate_clockwise_pressed_) rotate_vec[1]+= -1.0f;
		if(rotate_anticlockwise_pressed_) rotate_vec[1]+= 1.0f;

		const float rot_speed= 1.75f;
		angle_[0]+= dt * rot_speed * rotate_vec[0];
		//angle_[1]+= dt * rot_speed * rotate_vec[1];
		angle_[2]+= dt * rot_speed * rotate_vec[2];
		
		if( angle_[2] > MF_2PI ) angle_[2]-= MF_2PI;
		else if( angle_[2] < 0.0f ) angle_[2]+= MF_2PI;
		if( angle_[0] > MF_PI2 ) angle_[0]= MF_PI2;
		else if( angle_[0] < -MF_PI2) angle_[0]= -MF_PI2;
		if( angle_[1] > MF_PI ) angle_[1]-= MF_2PI;
		else if( angle_[1] < -MF_PI) angle_[1]+= MF_2PI;
	}
	else
	{
		angle_[0]= -MF_PI6;
		angle_[1]= 0.0f;
		angle_[2]= -MF_PI4;
		aircraft_to_choose_.SetPos( aircraft_.Pos() );

		static const float x_axis[3]= { 1.0f, 0.0f, 0.0f };
		static const float y_axis[3]= { 0.0f, 1.0f, 0.0f };
		static const float z_axis[3]= { 0.0f, 0.0f, 1.0f };

		float axis_transform_mat[16];
		float transformed_axis[2][3];
		Mat4RotateZ( axis_transform_mat, float(clock()) / float(CLOCKS_PER_SEC) * MF_PI2 );
		Vec3Mat4Mul( x_axis, axis_transform_mat, transformed_axis[0] );
		Vec3Mat4Mul( y_axis, axis_transform_mat, transformed_axis[1] );
		aircraft_to_choose_.SetAxis( transformed_axis[0], transformed_axis[1], z_axis );

		float cam_vec[3];
		SphericalCoordinatesToVec( angle_[2], angle_[0], cam_vec );
		cam_vec[2]= 1.0f - 1.25f * ( 1.0f - cam_vec[2] );
		Vec3Mul( cam_vec, -cam_radius_ );
		Vec3Add( aircraft_to_choose_.Pos(), cam_vec, pos_ );
		angle_[1]= 0.0f;
	}

	if ( control_mode_ == ModeDebugFreeFlight )
	{
		float move_vector[]= { 0.0f, 0.0f, 0.0f };

		if(forward_pressed_) move_vector[1]+= 1.0f;
		if(backward_pressed_) move_vector[1]+= -1.0f;
		if(left_pressed_) move_vector[0]+= -1.0f;
		if(right_pressed_) move_vector[0]+= 1.0f;
		if(up_pressed_) move_vector[2]+= 1.0f;
		if(down_pressed_) move_vector[2]+= -1.0f;

		float move_vector_rot_mat[16];
		Mat4RotateZ( move_vector_rot_mat, angle_[2] );

		Vec3Mat4Mul( move_vector, move_vector_rot_mat );

		const float speed= 5.0f * 25.0f;
		float tmp= dt * speed;
		pos_[0]+= tmp * move_vector[0];
		pos_[1]+= tmp * move_vector[1];
		pos_[2]+= tmp * move_vector[2];
	} // if debug free flight
	else if ( control_mode_ == ModeAircraftControl )
	{
		float pitch_factor= 0.0f;
		float roll_factor=  0.0f;
		float yaw_factor=   0.0f;

		if(forward_pressed_) pitch_factor+= -1.0f;
		if(backward_pressed_) pitch_factor+= 1.0f;
		if(left_pressed_) yaw_factor+= 1.0f;
		if(right_pressed_)yaw_factor+= -1.0f;
		if(rotate_clockwise_pressed_) roll_factor+= -1.0f;
		if(rotate_anticlockwise_pressed_) roll_factor+= 1.0f;

		if(up_pressed_) aircraft_.ThrottleUp( dt );
		if(down_pressed_) aircraft_.ThrottleDown( dt );
		
		autopilot_.GetControlResult( &pitch_factor, &yaw_factor, &roll_factor );

		//pitch_factor= 0.0008f; stability pitch factor
		aircraft_.SetPitchFactor( pitch_factor );
		aircraft_.SetRollFactor ( roll_factor  );
		aircraft_.SetYawFactor  ( yaw_factor   );
		aircraft_.Tick( dt );

		if( view_mode_ == ViewThirdperson )
		{
			float cam_vec[3];
			SphericalCoordinatesToVec( angle_[2], angle_[0], cam_vec );
			cam_vec[2]= 1.0f - 1.25f * ( 1.0f - cam_vec[2] );
			Vec3Mul( cam_vec, -cam_radius_ );
			Vec3Add( aircraft_.Pos(), cam_vec, pos_ );
			angle_[1]= 0.0f;
		}
		else
		{
			VEC3_CPY( pos_, aircraft_.Pos() );
			aircraft_.CalculateAngles( angle_ );
		}
	}
}

bool mf_Player::TryRespawn( const float* pos )
{
	if( lifes_ == 0 ) return false;

	aircraft_.SetPos( pos );

	is_in_respawn_= true;
	respawn_start_time_= mf_MainLoop::Instance()->CurrentTime();

	return true;
}

void mf_Player::SetupInitialAircraftParams()
{
	aircraft_.AddHP( MF_PLAYER_START_HP - aircraft_.HP() );
	aircraft_.AddRockets( MF_START_ROCKET_COUNT - aircraft_.RocketsCount() );
	aircraft_.SetThrottle( 1.0f );
	fov_= target_fov_= MF_INITIAL_FOV;

	static const float start_axis[]= { 1.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 1.0f };
	static const float start_vel[]= { 0.0f, 68.0f, 0.0f };
	aircraft_.SetAxis( start_axis, start_axis + 3, start_axis + 6 );
	aircraft_.SetVelocity( start_vel );
}

float mf_Player::GetMachinegunCircleRadius() const
{
	return 0.5f *
		float(mf_MainLoop::Instance()->ViewportHeight()) *
		mf_Math::tan( aircraft_.GetMachinegunConeAngle() ) /
		mf_Math::tan( fov_ * 0.5f );
}

void mf_Player::RotateX( float pixel_delta )
{
	angle_[0]+= pixel_delta * 0.01f * mf_Math::sqrt( mf_Math::tan( fov_ * 0.5f ) );
}

void mf_Player::RotateZ( float pixel_delta )
{
	angle_[2]+= pixel_delta * 0.01f * mf_Math::sqrt( mf_Math::tan( fov_ * 0.5f ) );
}

void mf_Player::ScreenPointToWorldSpaceVec( unsigned int x, unsigned int y,  float* out_vec ) const
{
	mf_MainLoop* main_loop= mf_MainLoop::Instance();
	float vec[3];
	vec[0]= float(x) / float(main_loop->ViewportWidth()) * 2.0f - 1.0f;
	vec[2]= float(y) / float(main_loop->ViewportHeight()) * (-2.0f) + 1.0f;
	vec[1]= 1.0f;

	float t= mf_Math::tan( fov_ * 0.5f );
	vec[0]*= t * float(main_loop->ViewportWidth()) / float(main_loop->ViewportHeight());
	vec[2]*= t;
	Vec3Normalize(vec);

	float rot_x_mat[16];
	float rot_z_mat[16];
	float rot_y_mat[16];
	float mat[16];
	Mat4RotateX( rot_x_mat, angle_[0] );
	Mat4RotateZ( rot_z_mat, angle_[2] );
	Mat4RotateY( rot_y_mat, angle_[1] );

	Mat4Mul( rot_y_mat, rot_x_mat, mat );
	Mat4Mul( mat, rot_z_mat );

	Vec3Mat4Mul( vec, mat, out_vec );
}

void mf_Player::ChooseNextAircraft()
{
	unsigned int type= aircraft_to_choose_.GetType();
	type++;
	type%= mf_Aircraft::LastType;
	aircraft_to_choose_.SetType( mf_Aircraft::Type(type) );
	CalculateCamRadius( aircraft_to_choose_.GetType() );
}

void mf_Player::ChoosePreviousAircraft()
{
	unsigned int type= aircraft_to_choose_.GetType();
	type+= mf_Aircraft::LastType - 1;
	type%= mf_Aircraft::LastType;
	aircraft_to_choose_.SetType( mf_Aircraft::Type(type) );
	CalculateCamRadius( aircraft_to_choose_.GetType() );
}

void mf_Player::ChoseAircraft()
{
	aircraft_.SetType( aircraft_to_choose_.GetType() );
	CalculateCamRadius( aircraft_.GetType() );
	control_mode_= ModeAircraftControl;
}

void mf_Player::AddEnemyAircraft( mf_Aircraft* aircraft )
{
	enemies_aircrafts_[ enemies_count_++ ]= aircraft;
}

void mf_Player::RemoveEnemyAircraft( mf_Aircraft* aircraft )
{
	for( unsigned int i= 0; i< enemies_count_; i++ )
		if( enemies_aircrafts_[i] == aircraft && i < enemies_count_ - 1 )
			enemies_aircrafts_[i]= enemies_aircrafts_[ enemies_count_ - 1 ];

	enemies_count_--;
}

void mf_Player::ZoomIn()
{
	target_fov_-= MF_FOV_STEP;
	if( target_fov_ < MF_MIN_FOV ) target_fov_= MF_MIN_FOV;
}

void mf_Player::ZoomOut()
{
	target_fov_+= MF_FOV_STEP;
	if( target_fov_ > MF_MAX_FOV ) target_fov_= MF_MAX_FOV;
}

void mf_Player::CalculateCamRadius( mf_Aircraft::Type type )
{
	const mf_ModelHeader* header= (const mf_ModelHeader*) mf_Models::aircraft_models[ type ];

	float radius_vecor[3];
	for( unsigned int i= 0; i< 3; i++ )
	{
		float min, max;
		min= -128.0f * header->scale[i] + header->pos[i];
		max=  127.0f * header->scale[i] + header->pos[i];
		radius_vecor[i]= mf_Math::fabs( min );
		float rv2= mf_Math::fabs( max );
		if( rv2 > radius_vecor[i] ) radius_vecor[i]= rv2;
	}

	cam_radius_= 1.2f * Vec3Len( radius_vecor );
	//cam_radius_= 40.0f;
}
