#pragma once
#include "micro-f.h"

class mf_Aircraft
{
public:
	mf_Aircraft();
	~mf_Aircraft();

	void Tick( float dt );

	const float* Pos() const;
	const float* Angle() const;
	const float* AxisVec( unsigned int vec ) const;
	const float* Velocity() const;

	unsigned int HP() const;

	void SetPos( const float* pos );
	void SetVelocity( const float* vel );

	void SetPitchFactor( float pitch );
	void SetYawFactor( float yaw );
	void SetRollFactor( float roll );

private:
	float pos_[3];
	float angle_[3];
	float velocity_[3];

	// 0 - pitch
	// 1 - roll
	// 2 - yaw
	float axis_[3][3]; // local x, y, z vectors in global space

	// [-1;1]
	float pitch_factor_;
	float yaw_factor_;
	float roll_factor_;

	float throttle_;

	unsigned int hp_; // [0;1]

};

inline const float* mf_Aircraft::Pos() const
{
	return pos_;
}

inline const float* mf_Aircraft::Angle() const
{
	return angle_;
}

inline const float* mf_Aircraft::AxisVec( unsigned int vec ) const
{
	return axis_[vec];
}

inline const float* mf_Aircraft::Velocity() const
{
	return velocity_;
}

inline unsigned int mf_Aircraft::HP() const
{
	return hp_;
}

inline void mf_Aircraft::SetPos( const float* pos )
{
	pos_[0]= pos[0];
	pos_[1]= pos[1];
	pos_[2]= pos[2];
}

inline void mf_Aircraft::SetVelocity( const float* vel )
{
	velocity_[0]= vel[0];
	velocity_[1]= vel[1];
	velocity_[2]= vel[2];
}

inline void mf_Aircraft::SetPitchFactor( float pitch )
{
	pitch_factor_= pitch;
}

inline void mf_Aircraft::SetYawFactor( float yaw )
{
	yaw_factor_= yaw;
}

inline void mf_Aircraft::SetRollFactor( float roll )
{
	roll_factor_= roll;
}