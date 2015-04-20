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
		ModeReachAltitude,
		ModeTurnToAzimuth
	};

	mf_Autopilot( const mf_Aircraft* aircraft );
	~mf_Autopilot();

	void SetMode( Mode mode );
	void SetTargetAltitude( float altitude );
	void SetTargetAzimuth( float azimuth );

	void GetControlResult( float* in_out_pitch_factor, float* in_out_yaw_factor, float* in_out_roll_factor );
private:

	Mode mode_;
	const mf_Aircraft* aircraft_;

	float target_altitude_;
	float target_azimuth_;
};

inline void mf_Autopilot::SetTargetAltitude( float altitude )
{
	target_altitude_= altitude;
}

inline void mf_Autopilot::SetTargetAzimuth( float azimuth )
{
	target_azimuth_= azimuth;
}