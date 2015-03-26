#include "micro-f.h"

#include "autopilot.h"
#include "aircraft.h"
#include "mf_math.h"

mf_Autopilot::mf_Autopilot( const mf_Aircraft* aircraft )
	: mode_(ModeDisabled)
	, aircraft_(aircraft)
{
}

mf_Autopilot::~mf_Autopilot()
{
}

void mf_Autopilot::SetMode( Mode mode )
{
	mode_= mode;
	if( mode == ModeDirectionControl )
	{
		for( unsigned int i= 0; i< 3; i++ )
		{
			VEC3_CPY( terget_axis_[i], aircraft_->AxisVec(i) );
		}
	}
}

void mf_Autopilot::GetControlResult( float* in_out_pitch_factor, float* in_out_yaw_factor, float* in_out_roll_factor )
{
	if( mode_ == ModeDirectionControl ) // if mode - direction control
	{
		const float c_lost_control_eps= 0.01f;
		const float c_angle_eps= 0.0005f;

		if( mf_Math::fabs( *in_out_pitch_factor ) > c_lost_control_eps||
			mf_Math::fabs( *in_out_yaw_factor   ) > c_lost_control_eps ||
			mf_Math::fabs( *in_out_roll_factor  ) > c_lost_control_eps )
			SetMode(ModeDirectionControl);

		float rotation_to_target_vec[3];
		float angle_to_target;

		{
			float target_axis_mat[16];
			float current_axis_mat[16];
			float mat_mul[16];
			float rotation_mat[16];
			Mat4Identity( target_axis_mat );
			Mat4Identity( current_axis_mat );
			for( unsigned int i= 0; i< 3; i++ )
			{
				VEC3_CPY( target_axis_mat + i*4, terget_axis_[i] );
				VEC3_CPY( current_axis_mat + i*4, aircraft_->AxisVec(i) );
			}
			Mat4Transpose( target_axis_mat ); // like invert for rotation matrices

			Mat4Mul( current_axis_mat, target_axis_mat, mat_mul );
			MatToRotation( mat_mul, rotation_to_target_vec, &angle_to_target );
		}


		if( mf_Math::fabs(angle_to_target) > c_angle_eps )
		{
			if( mf_Math::fabs(*in_out_pitch_factor) < c_lost_control_eps )
			{
				float pitch_angle= rotation_to_target_vec[0] * angle_to_target;
				*in_out_pitch_factor= -5.0f * aircraft_->AngularSpeed()[0] - pitch_angle * 1.1f;
				*in_out_pitch_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_pitch_factor );
			}
			if( mf_Math::fabs(*in_out_roll_factor) < c_lost_control_eps )
			{
				float roll_angle= rotation_to_target_vec[1] * angle_to_target;
				*in_out_roll_factor= -5.0f * aircraft_->AngularSpeed()[1] - roll_angle * 1.1f;
				*in_out_roll_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_roll_factor );
			}
			/*if( mf_Math::fabs(*in_out_yaw_factor) < c_lost_control_eps )
			{
				float yaw_angle= rotation_to_target_vec[2] * angle_to_target;
				*in_out_yaw_factor= -5.0f * aircraft_->AngularSpeed()[2] - yaw_angle * 0.5f;
				*in_out_yaw_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_yaw_factor );
			}*/
		}
	}
}