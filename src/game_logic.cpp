#include "micro-f.h"
#include "game_logic.h"
#include "sound_engine.h"

#include "player.h"
#include "mf_math.h"

#define MF_FORCEFIELD_DAMAGE_RADIUS 8.0f

namespace PowerupsTables
{

int stars_bonus_table[mf_Powerup::LastType]=
{
	1, 0
};

int health_bonus_table[mf_Powerup::LastType]=
{
	0, 200
};

} // namespace PowerupsTables

mf_GameLogic::mf_GameLogic(mf_Player* player)
	: level_()
	, particles_manager_()
	, player_(player)
	, powerup_count_(0), bullets_count_(0)
{
	PlacePowerups();
}

mf_GameLogic::~mf_GameLogic()
{
}

void mf_GameLogic::Tick( float dt )
{
	mf_Aircraft* player_aircraft= (mf_Aircraft*) player_->GetAircraft();

	const float c_powerup_pick_distance2= 16.0f * 16.0f;

	for( unsigned int i= 0; i< bullets_count_; )
	{
		mf_Bullet* bullet= bullets_ + i;
		if( bullet->velocity == mfInf() )
		{
			float pos[3];
			if( level_.BeamIntersectTerrain( bullet->pos, bullet->dir, false, pos ) )
			{
				particles_manager_.AddBulletTerrainHit( pos );
			}
			if( i != bullets_count_ - 1 )
				*bullet= bullets_[ bullets_count_ - 1 ];
			bullets_count_--;
			continue;
		}
		else
		{
		}
		i++;
	}

	for( unsigned int i= 0; i< powerup_count_; i++ )
	{
		float vec_to_powerup[3];
		Vec3Sub( player_aircraft->Pos(), powerups_[i].pos, vec_to_powerup );
		float dst2= Vec3Dot( vec_to_powerup, vec_to_powerup );
		if( dst2 < c_powerup_pick_distance2 )
		{
			player_aircraft->AddHP( powerups_[i].health_bonus );
			player_->AddScorePoints( powerups_[i].stars_bonus );

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
		player_aircraft->AddHP( - ( player_aircraft->HP() + 100 ) );
	}

	{ // calculate collision with forcefield
		float vec_to_forcefield[3];
		vec_to_forcefield[0]= player_aircraft->Pos()[0] - float(level_.TerrainSizeX()) * level_.TerrainCellSize() * 0.5f;
		vec_to_forcefield[1]= 0;
		vec_to_forcefield[2]= player_aircraft->Pos()[2] - level_.ForcefieldZPos();
		float dist_to_forcefield= Vec3Len( vec_to_forcefield );
		if( dist_to_forcefield > level_.ForcefieldRadius() )
			player_aircraft->AddHP( - ( player_aircraft->HP() + 100 ) );
		else if( level_.ForcefieldRadius() - dist_to_forcefield < MF_FORCEFIELD_DAMAGE_RADIUS )
			player_aircraft->AddHP( -50 );
	}


	particles_manager_.Tick( dt );
	particles_manager_.AddEnginesTrail( player_->GetAircraft() );
}

void mf_GameLogic::PlayerShot( const float* dir )
{
	mf_Bullet* bullet= &bullets_[ bullets_count_ ];
	bullet->type= mf_Bullet::ChaingunBullet;
	bullet->owner= (mf_Aircraft*)player_->GetAircraft();
	VEC3_CPY( bullet->pos, bullet->owner->Pos() );
	VEC3_CPY( bullet->dir, dir );
	bullet->velocity= mfInf();

	bullets_count_++;

	mf_SoundEngine::Instance()->AddSingleSound( SoundMachinegunShot, 1.0f, 1.0f, NULL );
}

void mf_GameLogic::PlacePowerups()
{
	static const float c_stars_step_range[2]= { 64.0f, 128.0f };

	powerup_count_= 0;

	float y= 32.0f;
	while( y < level_.TerrainSizeY() * level_.TerrainCellSize() )
	{
		mf_Powerup::Type type= mf_Powerup::Type( randomizer_.Rand() % mf_Powerup::LastType );
		powerups_[powerup_count_].type= type;
		powerups_[powerup_count_].stars_bonus= PowerupsTables::stars_bonus_table[type];
		powerups_[powerup_count_].health_bonus= PowerupsTables::health_bonus_table[type];
		powerups_[powerup_count_].pos[0]= level_.GetValleyCenterX( y );
		powerups_[powerup_count_].pos[1]= y;
		powerups_[powerup_count_].pos[2]= level_.TerrainWaterLevel() + 0.25f * randomizer_.RandF( 5.0f, level_.TerrainAmplitude() );
		powerup_count_++;

		y+= randomizer_.RandF( c_stars_step_range[0], c_stars_step_range[1] );
	}
}