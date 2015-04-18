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
}

void mf_Autopilot::GetControlResult( float* in_out_pitch_factor, float* in_out_yaw_factor, float* in_out_roll_factor )
{
	if( mode_ == ModeTurnToAzimuth )
	{
		const float c_max_y_angle= MF_PI2 - MF_PI12;
		const float c_almost_one= 0.9999999f;

		float elevation= mf_Math::asin( mf_Math::clamp( -c_almost_one, c_almost_one, aircraft_->AxisVec(1)[2] ) );
		float angle[3];
		{
			VecToSphericalCoordinates( aircraft_->AxisVec(1), &angle[0], &angle[2] );

			float rot_x_mat[16];
			float rot_z_mat[16];
			float rot_mat[16];
			Mat4RotateX( rot_x_mat, -angle[0] );
			Mat4RotateZ( rot_z_mat, -angle[2] );
			Mat4Mul( rot_z_mat, rot_x_mat, rot_mat );

			float x_vec[3];
			Vec3Mat4Mul( aircraft_->AxisVec(0), rot_mat, x_vec );
			angle[1]= - atan2( x_vec[2], x_vec[0] );
		}
		float z_rot_speed=
			aircraft_->AxisVec(0)[2] * aircraft_->AngularSpeed()[0] +
			aircraft_->AxisVec(1)[2] * aircraft_->AngularSpeed()[1] +
			aircraft_->AxisVec(2)[2] * aircraft_->AngularSpeed()[2];
		
		float target_azimuth_vec[3];
		SphericalCoordinatesToVec( target_azimuth_, 0.0f, target_azimuth_vec );
		float course_vec[3]= { aircraft_->AxisVec(1)[0], aircraft_->AxisVec(1)[1], 0.0f };
		Vec3Normalize( course_vec );

		float azimuth_course_cross[3];
		Vec3Cross( target_azimuth_vec, course_vec, azimuth_course_cross );
		float course_angle_delta= mf_Math::asin( mf_Math::clamp(-c_almost_one, c_almost_one, azimuth_course_cross[2] ) );
		if( mf_Math::fabs(course_angle_delta) < MF_2PI * 0.003f ) course_angle_delta= 0.0f;
		printf( "z_rot_speed: %f\n", z_rot_speed );

		// target roll
		float target_y_angle= mf_Math::clamp( -c_max_y_angle, c_max_y_angle, 0.5f * course_angle_delta );

		float y_angle_delta= target_y_angle - angle[1];
		*in_out_roll_factor= mf_Math::clamp( -1.0f, 1.0f,
			+8.0f * y_angle_delta * y_angle_delta * y_angle_delta
			- 0.5f * aircraft_->AngularSpeed()[1]
			);

		*in_out_pitch_factor= mf_Math::clamp( -1.0f, 1.0f,
			mf_Math::fabs(course_angle_delta) *
			0.3f *
			mf_Math::fabs(mf_Math::sin(angle[1]))
			- elevation * 2.0f + 2.0f * aircraft_->AngularSpeed()[0] );
	}
	else if( mode_ == ModeReachAltitude )
	{
		float altitude_delta= target_altitude_ - aircraft_->Pos()[2];

		// good
		//*in_out_pitch_factor= altitude_delta * 0.01f - (aircraft_->Velocity()[2]) * 0.01f;
		// better
		//*in_out_pitch_factor= altitude_delta * 0.005f - (aircraft_->Velocity()[2]) * 0.01f;
		*in_out_pitch_factor= altitude_delta * 0.003f - (aircraft_->Velocity()[2]) * 0.008f;

		if( *in_out_pitch_factor > 0.1f) *in_out_pitch_factor= 0.1f;
		else if( *in_out_pitch_factor < -0.1f) *in_out_pitch_factor= -0.1f;
	}

	if( mode_ == ModeKillRotation || mode_ == ModeReachAltitude )
	{
		const float c_lost_control_eps= 0.005f;

		if( mf_Math::fabs(*in_out_pitch_factor) < c_lost_control_eps )
		{
			*in_out_pitch_factor= -50.0f * aircraft_->AngularSpeed()[0];
			*in_out_pitch_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_pitch_factor );
		}
		if( mf_Math::fabs(*in_out_roll_factor) < c_lost_control_eps )
		{
			*in_out_roll_factor= -100.0f * aircraft_->AngularSpeed()[1];
			*in_out_roll_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_roll_factor );
		}
		if( mf_Math::fabs(*in_out_yaw_factor) < c_lost_control_eps )
		{
			*in_out_yaw_factor= -50.0f * aircraft_->AngularSpeed()[2];
			*in_out_yaw_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_yaw_factor );
		}
	}
}