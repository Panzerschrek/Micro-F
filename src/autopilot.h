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
		ModeHorizontalFlight,
		ModeAzimuthChange // turn to target
	};

	mf_Autopilot( const mf_Aircraft* aircraft );
	~mf_Autopilot();

	void SetMode( Mode mode );
	void SetTargetAzimuth( float azimuth ); // for ModeAzimuthChange

	void GetControlResult( float* in_out_pitch_factor, float* in_out_yaw_factor, float* in_out_roll_factor );
private:

	Mode mode_;
	const mf_Aircraft* aircraft_;
	float azimuth_;
};

inline void mf_Autopilot::SetTargetAzimuth( float azimuth )
{
	azimuth_= azimuth;
}