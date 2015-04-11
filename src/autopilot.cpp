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
	if( mode_ == ModeReachAltitude )
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