#include "micro-f.h"
#include "game_logic.h"

#include "player.h"
#include "mf_math.h"

#define MF_MAX_POWERUPS_COUNT 128

mf_GameLogic::mf_GameLogic(mf_Player* player)
	: level_()
	, particles_manager_()
	, player_(player)
	, powerups_(NULL), powerup_count_(0)
{
	PlaceStars();
}

mf_GameLogic::~mf_GameLogic()
{
}

void mf_GameLogic::Tick( float dt )
{
	mf_Aircraft* player_aircraft= (mf_Aircraft*) player_->GetAircraft();

	const float c_powerup_pick_distance2= 16.0f * 16.0f;

	for( unsigned int i= 0; i< powerup_count_; i++ )
	{
		float vec_to_powerup[3];
		Vec3Sub( player_aircraft->Pos(), powerups_[i].pos, vec_to_powerup );
		float dst2= Vec3Dot( vec_to_powerup, vec_to_powerup );
		if( dst2 < c_powerup_pick_distance2 )
		{
			if( i < powerup_count_ - 1 )
				powerups_[i]= powerups_[ powerup_count_ - 1 ];
			powerup_count_--;
			break;
		}
	}

	if( level_.SphereIntersectTerrain( player_aircraft->Pos(), 4.0f ) )
	{
		float pos[3];
		VEC3_CPY( pos, player_aircraft->Pos() );
		pos[2]+= 4.0f;
		player_aircraft->SetPos(pos);
	}

	particles_manager_.Tick( dt );
	particles_manager_.AddEnginesTrail( player_->GetAircraft() );
	particles_manager_.AddEnginesFire( player_->GetAircraft() );
}

void mf_GameLogic::PlaceStars()
{
	static const float c_stars_step_range[2]= { 64.0f, 128.0f };

	mf_Rand randomizer;

	powerups_= new mf_Powerup[ MF_MAX_POWERUPS_COUNT ];
	powerup_count_= 0;

	float y= 32.0f;
	while( y < level_.TerrainSizeY() * level_.TerrainCellSize() )
	{
		powerups_[powerup_count_].type= mf_Powerup::Star;
		powerups_[powerup_count_].stars_bonus= 1;
		powerups_[powerup_count_].pos[0]= level_.GetValleyCenterX( y );
		powerups_[powerup_count_].pos[1]= y;
		powerups_[powerup_count_].pos[2]= level_.TerrainWaterLevel() + 0.25f * randomizer.RandF( 5.0f, level_.TerrainAmplitude() );
		powerup_count_++;

		y+= randomizer.RandF( c_stars_step_range[0], c_stars_step_range[1] );
	}
}