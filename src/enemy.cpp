#include "micro-f.h"
#include "enemy.h"

mf_Enemy::mf_Enemy( mf_Aircraft::Type type, int hp )
	: aircraft_( type, hp )
	, autopilot_( &aircraft_ )
{
	autopilot_.SetMode( mf_Autopilot::ModeKillRotation );
}

mf_Enemy::~mf_Enemy()
{
}

void mf_Enemy::Tick( float dt )
{
	float yaw= 0.0f, pitch= 0.0f, roll= 0.0f;
	autopilot_.GetControlResult( &pitch, &yaw, &roll );
	aircraft_.SetPitchFactor( pitch );
	aircraft_.SetYawFactor( yaw );
	aircraft_.SetRollFactor( roll );
	aircraft_.Tick(dt);
}