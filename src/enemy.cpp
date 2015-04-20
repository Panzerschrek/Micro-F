#include "micro-f.h"
#include "enemy.h"
#include "mf_math.h"

mf_Enemy::mf_Enemy( mf_Aircraft::Type type, int hp, const mf_Aircraft* player_aircraft )
	: aircraft_( type, hp )
	, autopilot_( &aircraft_ )
	, player_aircraft_(player_aircraft)
{
	autopilot_.SetMode( mf_Autopilot::ModeTurnToAzimuth );
}

mf_Enemy::~mf_Enemy()
{
}

void mf_Enemy::Tick( float dt )
{
	autopilot_.SetTargetAltitude( player_aircraft_->Pos()[2] );
	
	float vec_to_player[3];
	float angle[3];
	Vec3Sub( player_aircraft_->Pos(), aircraft_.Pos(), vec_to_player );
	Vec3Normalize( vec_to_player );
	VecToSphericalCoordinates( vec_to_player, &angle[2], &angle[0] );

	autopilot_.SetTargetAzimuth( angle[2] );

	float yaw= 0.0f, pitch= 0.0f, roll= 0.0f;
	autopilot_.GetControlResult( &pitch, &yaw, &roll );
	aircraft_.SetPitchFactor( pitch );
	aircraft_.SetYawFactor( yaw );
	aircraft_.SetRollFactor( roll );
	aircraft_.Tick(dt);
}