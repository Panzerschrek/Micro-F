#pragma once
#include "micro-f.h"

class mf_Aircraft
{
public:
	enum Type
	{
		F1949,
		F2XXX,
		V1,
		LastType
	};

	mf_Aircraft( Type type = F1949, int hp= 1000, const float* pos= NULL, float course= 0.0f );
	~mf_Aircraft();

	void Tick( float dt );

	const Type GetType() const;

	const float* Pos() const;
	const float* Angle() const;
	const float* AxisVec( unsigned int vec ) const;
	const float* Velocity() const;
	const float* AngularSpeed() const;
	const float* AngularAcceleration() const;

	void CalculateAngles( float* out_angles ) const;

	void SetThrottle( float throttle );
	float Throttle() const;

	float GetMachinegunConeAngle() const;

	int HP() const;
	void AddHP( int hp_bonus );

	int RocketsCount() const;
	void AddRockets( int rocket_bonus );

	void SetType( Type type );

	void SetPos( const float* pos );
	void SetVelocity( const float* vel );

	void SetAxis( const float* x_axis, const float* y_axis, const float* z_axis );

	void ThrottleUp( float dt );
	void ThrottleDown( float dt );

	void SetPitchFactor( float pitch );
	void SetYawFactor( float yaw );
	void SetRollFactor( float roll );

	void MachinegunShot( float time );
	float LastMachinegunShotTime() const;

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

	int hp_;
	int rockets_count_;

	float last_machinegun_shot_time_;

	float max_engines_trust_; // N
	float wings_area_; // m^2
	float wings_span_length_; // m
	float fuselage_length_;
	float fuselage_diameter_;
	float mass_; //kg
	float inertia_moment_[3]; // g / m^2
	float velocity_[3]; // m/s
	float acceleration_[3]; // m/s^2
	float angular_speed_[3];// 1/s
	float angular_acceleration_[3]; // 1/s^2

#ifdef MF_DEBUG
public:
	float debug_angle_of_attack_deg_;
	float debug_cxk_;
	float debug_cyk_;
	float debug_pitch_control_factor_;
	float debug_yaw_control_factor_;
	float debug_roll_control_factor_;
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

inline void mf_Aircraft::SetThrottle( float throttle )
{
	throttle_= throttle;
}

inline float mf_Aircraft::Throttle() const
{
	return throttle_;
}

inline const float* mf_Aircraft::Velocity() const
{
	return velocity_;
}

inline const float* mf_Aircraft::AngularSpeed() const 
{
	return angular_speed_;
}

inline const float* mf_Aircraft::AngularAcceleration() const
{
	return angular_acceleration_;
}

inline int mf_Aircraft::HP() const
{
	return hp_;
}

inline void mf_Aircraft::AddHP( int hp_bonus )
{
	hp_+= hp_bonus;
}

inline int mf_Aircraft::RocketsCount() const
{
	return rockets_count_;
}

inline void mf_Aircraft::AddRockets( int rocket_bonus )
{
	rockets_count_+= rocket_bonus;
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

inline void mf_Aircraft::MachinegunShot( float time )
{
	last_machinegun_shot_time_= time;
}

inline float mf_Aircraft::LastMachinegunShotTime() const
{
	return last_machinegun_shot_time_;
}