#pragma once
#include "micro-f.h"

class mf_Aircraft
{
public:
	enum Type
	{
		F1949,
		F2XXX,
		LastType
	};

	mf_Aircraft( Type type = F1949 );
	~mf_Aircraft();

	void Tick( float dt );

	const Type GetType() const;

	const float* Pos() const;
	const float* Angle() const;
	const float* AxisVec( unsigned int vec ) const;
	const float* Velocity() const;

	float Throttle() const;

	unsigned int HP() const;

	void SetType( Type type );

	void SetPos( const float* pos );
	void SetVelocity( const float* vel );

	void ThrottleUp( float dt );
	void ThrottleDown( float dt );

	void SetPitchFactor( float pitch );
	void SetYawFactor( float yaw );
	void SetRollFactor( float roll );

private:
	Type type_;
	float pos_[3];
	float angle_[3];

	// 0 - pitch
	// 1 - roll
	// 2 - yaw
	float axis_[3][3]; // local x, y, z vectors in global space

	// [-1;1]
	float pitch_factor_;
	float yaw_factor_;
	float roll_factor_;

	//[0;1]
	float throttle_;

	unsigned int hp_; // [0;1]

	float max_engines_trust_; // N
	float wings_area_; // m^2
	float wings_span_length_; // m
	float mass_; //kg
	float velocity_[3]; // m/s
	float acceleration_[3]; // m/s^2
	float angular_speed_[3];// 1/s
	float angular_acceleration_[3]; // 1/s^2

#ifdef MF_DEBUG
public:
	float debug_angle_of_attack_deg_;
	float debug_cxk_;
	float debug_cyk_;
#endif
};

inline const mf_Aircraft::Type mf_Aircraft::GetType() const
{
	return type_;
}

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

inline float mf_Aircraft::Throttle() const
{
	return throttle_;
}

inline const float* mf_Aircraft::Velocity() const
{
	return velocity_;
}

inline unsigned int mf_Aircraft::HP() const
{
	return hp_;
}

inline void mf_Aircraft::SetType( Type type )
{
	type_= type;
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