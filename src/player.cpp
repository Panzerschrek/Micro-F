#include "micro-f.h"
#include "player.h"

#include "mf_math.h"

mf_Player::mf_Player()
	: control_mode_(ModeAircraftControl)
	, aspect_(1.0f), fov_(MF_PI2)
	, forward_pressed_(false), backward_pressed_(false), left_pressed_(false), right_pressed_(false)
	, up_pressed_(false), down_pressed_(false)
	, rotate_up_pressed_(false), rotate_down_pressed_(false), rotate_left_pressed_(false), rotate_right_pressed_(false)
	, rotate_clockwise_pressed_(false), rotate_anticlockwise_pressed_(false)
{
	pos_[0]= pos_[1]= pos_[2]= 0.0f;
	angle_[0]= angle_[1]= angle_[2]= 0.0f;

	aircraft_.SetPos( pos_ );
}

mf_Player::~mf_Player()
{
}

void mf_Player::Tick( float dt )
{
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
		

		aircraft_.SetPitchFactor( pitch_factor );
		aircraft_.SetRollFactor ( roll_factor  );
		aircraft_.SetYawFactor  ( yaw_factor   );
		aircraft_.Tick( dt );

		float cam_vec[3];
		SphericalCoordinatesToVec( angle_[2], angle_[0], cam_vec );
		Vec3Add( aircraft_.Pos(), cam_vec, pos_ );
		Vec3Mul( cam_vec, -10.0f );
		Vec3Add( aircraft_.Pos(), cam_vec, pos_ );
	}
}

