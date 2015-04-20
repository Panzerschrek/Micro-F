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
		aircraft_->CalculateAngles( angle );

		float target_azimuth_vec[3];
		SphericalCoordinatesToVec( target_azimuth_, 0.0f, target_azimuth_vec );
		float course_vec[3]= { aircraft_->AxisVec(1)[0], aircraft_->AxisVec(1)[1], 0.0f };
		Vec3Normalize( course_vec );

		float azimuth_course_cross[3];
		Vec3Cross( target_azimuth_vec, course_vec, azimuth_course_cross );
		float course_angle_delta= mf_Math::asin( mf_Math::clamp(-c_almost_one, c_almost_one, azimuth_course_cross[2] ) );
		if( mf_Math::fabs(course_angle_delta) < MF_2PI * 0.003f ) course_angle_delta= 0.0f;

		// target roll
		float target_y_angle= mf_Math::clamp( -c_max_y_angle, c_max_y_angle, 1.5f * mf_Math::sign(course_angle_delta) * mf_Math::sqrt(mf_Math::fabs(course_angle_delta)) );
		float y_angle_delta= target_y_angle - angle[1];
		*in_out_roll_factor= mf_Math::clamp( -1.0f, 1.0f,
			+5.0f * y_angle_delta +
			-8.0f * aircraft_->AngularSpeed()[1]
			);

		*in_out_pitch_factor= mf_Math::clamp( -1.0f, 1.0f,
			mf_Math::fabs(course_angle_delta) *
			max( 0.0f, ( c_max_y_angle - 2.0f * mf_Math::fabs(y_angle_delta) ) / c_max_y_angle ) *
			0.5f * 
			mf_Math::fabs(mf_Math::sin(angle[1])) );

		*in_out_pitch_factor+= mf_Math::clamp( -0.1f, 0.1f, (target_altitude_ - aircraft_->Pos()[2]) * 0.003f - aircraft_->Velocity()[2] * 0.008f ) / max( 0.1f, mf_Math::cos(angle[1]) );
		*in_out_pitch_factor= mf_Math::clamp( -1.0f, 1.0f, *in_out_pitch_factor );

		*in_out_yaw_factor= mf_Math::clamp( -1.0f, 1.0f, -40.0f * aircraft_->AngularSpeed()[2] );
	}
	else if( mode_ == ModeReachAltitude )
	{
		float altitude_delta= target_altitude_ - aircraft_->Pos()[2];

		// good
		//*in_out_pitch_factor= altitude_delta * 0.01f - (aircraft_->Velocity()[2]) * 0.01f;
		// better
		//*in_out_pitch_factor= altitude_delta * 0.005f - (aircraft_->Velocity()[2]) * 0.01f;
		*in_out_pitch_factor= altitude_delta * 0.003f - aircraft_->Velocity()[2] * 0.008f;

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