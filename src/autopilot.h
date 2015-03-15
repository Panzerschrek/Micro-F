#pragma once
#include "micro-f.h"

class mf_Aircraft;

class mf_Autopilot
{
	enum Mode
	{
		ModeDisabled,
		ModeDirectionControl, // save yaw/pitch/rall
		ModeHorizontalFlight,
		ModeAzimuthChange // turn to target
	};
public:
	mf_Autopilot( const mf_Aircraft* aircraft );
	~mf_Autopilot();

	void SetMode( Mode mode );
	void SetTargetAzimuth( float azimuth ); // for ModeAzimuthChange

	void GetControlResult( float* out_pitch_factor, float* out_yaw_factor, float* out_roll_factor );
private:

	Mode mode_;
	const mf_Aircraft* aircraft_;
	float azimuth_;
};

inline void mf_Autopilot::SetMode( Mode mode )
{
	mode_= mode;
}

inline void mf_Autopilot::SetTargetAzimuth( float azimuth )
{
	azimuth_= azimuth;
}