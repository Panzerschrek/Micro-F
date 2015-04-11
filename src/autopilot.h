#pragma once
#include "micro-f.h"

class mf_Aircraft;

class mf_Autopilot
{
public:
	enum Mode
	{
		ModeDisabled,
		ModeKillRotation,
		ModeReachAltitude
	};

	mf_Autopilot( const mf_Aircraft* aircraft );
	~mf_Autopilot();

	void SetMode( Mode mode );
	void SetTargetAltitude( float altitude );

	void GetControlResult( float* in_out_pitch_factor, float* in_out_yaw_factor, float* in_out_roll_factor );
private:

	Mode mode_;
	const mf_Aircraft* aircraft_;

	float target_altitude_;
};

inline void mf_Autopilot::SetTargetAltitude( float altitude )
{
	target_altitude_= altitude;
}