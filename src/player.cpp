#include "micro-f.h"
#include "player.h"
#include "main_loop.h"

#include "mf_math.h"

#include "mf_model.h"
#include "../models/models.h"

#define MF_FOV_CHANGE_SPEED MF_PI6
#define MF_MIN_FOV MF_PI6
#define MF_MAX_FOV (MF_PI2 + MF_PI6)
#define MF_FOV_STEP (MF_PI6 * 0.25f)
#define MF_INITIAL_FOV (MF_PI2 - MF_FOV_STEP)

mf_Player::mf_Player()
	: control_mode_(ModeDebugFreeFlight)
	, aircraft_(mf_Aircraft::V1)
	, autopilot_(&aircraft_)
	, cam_radius_(10.0f)
	, aspect_(1.0f), fov_(MF_INITIAL_FOV), target_fov_(MF_INITIAL_FOV)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, rotate_clockwise_pressed_(false), rotate_anticlockwise_pressed_(false)
	, score_(0)
{
	pos_[0]= pos_[1]= pos_[2]= 0.0f;
	angle_[0]= angle_[1]= angle_[2]= 0.0f;

	aircraft_.SetPos( pos_ );

	CalculateCamRadius();

	autopilot_.SetMode( mf_Autopilot::ModeKillRotation );
}

mf_Player::~mf_Player()
{
}

void mf_Player::Tick( float dt )
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

		float cam_vec[3];
		SphericalCoordinatesToVec( angle_[2], angle_[0], cam_vec );
		Vec3Add( aircraft_.Pos(), cam_vec, pos_ );
		Vec3Mul( cam_vec, -cam_radius_ );
		Vec3Add( aircraft_.Pos(), cam_vec, pos_ );
	}
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
	float mat[16];
	Mat4RotateX( rot_x_mat, angle_[0] );
	Mat4RotateZ( rot_z_mat, angle_[2] );
	Mat4Mul( rot_x_mat, rot_z_mat, mat );
	Vec3Mat4Mul( vec, mat, out_vec );
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

void mf_Player::CalculateCamRadius()
{
	const mf_ModelHeader* header= (const mf_ModelHeader*) mf_Models::aircraft_models[ aircraft_.GetType() ];

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
