#include "micro-f.h"
#include "enemy.h"
#include "mf_math.h"
#include "game_logic.h"

mf_Enemy::mf_Enemy( mf_Aircraft::Type type, int hp,  mf_GameLogic* game_logic, const mf_Aircraft* player_aircraft )
	: game_logic_(game_logic)
	, aircraft_( type, hp, NULL, game_logic->GetRandomizer()->RandF(MF_2PI) )
	, autopilot_( &aircraft_ )
	, player_aircraft_(player_aircraft)
	, shot_pressed_(false)
{
	mf_Rand* rand= game_logic->GetRandomizer();

	float pos[3];
	SphericalCoordinatesToVec( rand->RandF( -MF_PI2 - MF_PI12, MF_PI2 + MF_PI12 ), 0.0f, pos );
	Vec3Mul( pos, rand->RandF(384.0f, 768.0f) );
	pos[2]= rand->RandF( -40.0f, 40.0f );
	Vec3Add( pos, player_aircraft->Pos() );
	if( pos[2] < 128.0f ) pos[2]= 128.0f;
	aircraft_.SetPos( pos );

	autopilot_.SetMode( mf_Autopilot::ModeKillRotation );
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

	if( Distance( player_aircraft_->Pos(), aircraft_.Pos() ) > 200.0f && mf_Math::fabs(angle[0]) < MF_PI4 )
	{
		autopilot_.SetMode( mf_Autopilot::ModeTurnToAzimuth );
		autopilot_.SetTargetAzimuth( angle[2] );
	}

	if( Vec3Dot( vec_to_player, aircraft_.AxisVec(1) ) >= mf_Math::cos(aircraft_.GetMachinegunConeAngle() * 0.5f) )
	{
		if (shot_pressed_)
		{
			game_logic_->ShotBegin( &aircraft_ );
			game_logic_->ShotContinue( &aircraft_, vec_to_player, true );
			shot_pressed_= true;
		}
		else game_logic_->ShotContinue( &aircraft_, vec_to_player, false );
	}
	else shot_pressed_= false;

	float yaw= 0.0f, pitch= 0.0f, roll= 0.0f;
	autopilot_.GetControlResult( &pitch, &yaw, &roll );
	aircraft_.SetPitchFactor( pitch );
	aircraft_.SetYawFactor( yaw );
	aircraft_.SetRollFactor( roll );
	aircraft_.Tick(dt);
}