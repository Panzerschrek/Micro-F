#include "micro-f.h"
#include "game_logic.h"
#include "sound_engine.h"
#include "../models/models.h"

#include "player.h"
#include "mf_math.h"
#include "enemy.h"

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


static mf_SoundType AircraftTypeToEngineSoundType( mf_Aircraft::Type type )
{
	switch( type )
	{
	case mf_Aircraft::V1:
		return SoundPulsejetEngine;
	case mf_Aircraft::F1949:
	case mf_Aircraft::F2XXX:
	default:
		return SoundPlasmajetEngine;
	};
}

static float ThrottleToEngineSoundPitch( float throttle )
{
	const float min_pitch= 0.8f;
	const float max_pitch= 1.2f;

	const float a= -2.0f * ( max_pitch - min_pitch );
	const float b=  3.0f * ( max_pitch - min_pitch );
	const float c= 0.0f;
	const float d= min_pitch;

	return d + throttle * ( c + throttle * ( b + throttle * a ) );
}

static float ThrottleToEngineSoundVolumeScaler( float throttle )
{
	if( throttle > 0.5f ) return 1.0f;
	return throttle * 2.0f;
}

static bool IsBulletOutsideWorld( const float* pos )
{
	return
		pos[0] < -512.0f || pos[0] > 32768.0f ||
		pos[1] < -512.0f || pos[1] > 32768.0f ||
		pos[2] < 0.0f || pos[2] > 1024.0f;
}

mf_GameLogic::mf_GameLogic(mf_Player* player)
	: level_()
	, particles_manager_()
	, player_(player)
	, powerup_count_(0), bullets_count_(0), enemies_count_(0)
	, player_sound_(NULL)
{
	for( unsigned int i= 0; i< mf_Aircraft::LastType; i++ )
		aircrafts_models_[i].LoadFromMFMD( mf_Models::aircraft_models[i] );

	PlacePowerups();
	SpawnEnemy();

	player_sound_= mf_SoundEngine::Instance()->CreateSoundSource( AircraftTypeToEngineSoundType( player_->GetAircraft()->GetType() ) );
	player_sound_->Play();
}

mf_GameLogic::~mf_GameLogic()
{
}

void mf_GameLogic::Tick( float dt )
{
	for( unsigned int i= 0; i< enemies_count_; i++ )
		enemies_[i]->Tick(dt);

	mf_Aircraft* player_aircraft= player_->GetAircraft();

	const float c_powerup_pick_distance2= 16.0f * 16.0f;

	for( unsigned int i= 0; i< bullets_count_; )
	{
		mf_Bullet* bullet= bullets_ + i;
		float intersection_pos[3];
		float bullet_travle_distance= dt * bullet->velocity;
		bool is_intersection= level_.BeamIntersectTerrain( bullet->pos, bullet->dir, bullet_travle_distance, false, intersection_pos );

		// search intersection with enemies
		for( unsigned int e= 0; e< enemies_count_; e++ )
		{
			float aircraft_space_dir[3];
			float aircraft_space_pos[3];
			float aircraft_space_hit_pos[3];
			mf_Aircraft* enemy_aircraft= enemies_[e]->GetAircraft();

			float pos_relative_aircraft[3];
			Vec3Sub( bullet->pos, enemy_aircraft->Pos(), pos_relative_aircraft );

			for( unsigned int j= 0; j< 3; j++ )
			{
				aircraft_space_pos[j]= Vec3Dot( pos_relative_aircraft, enemy_aircraft->AxisVec(j) );
				aircraft_space_dir[j]= Vec3Dot( bullet->dir, enemy_aircraft->AxisVec(j) );
			}

			if( aircrafts_models_[enemy_aircraft->GetType()].BeamIntersectModel( aircraft_space_pos, aircraft_space_dir, bullet_travle_distance, aircraft_space_hit_pos ) )
			{
				for( unsigned int j= 0; j< 3; j++ )
					intersection_pos[j]=
						enemy_aircraft->AxisVec(0)[j] * aircraft_space_hit_pos[0] +
						enemy_aircraft->AxisVec(1)[j] * aircraft_space_hit_pos[1] +
						enemy_aircraft->AxisVec(2)[j] * aircraft_space_hit_pos[2] +
						enemy_aircraft->Pos()[j];
				is_intersection= true;
			}
		}

		if( is_intersection )
			particles_manager_.AddBulletTerrainHit( intersection_pos );

		if( is_intersection || bullet->velocity == mfInf() || IsBulletOutsideWorld(bullet->pos) )
		{
			if( i != bullets_count_ - 1 )
				*bullet= bullets_[ bullets_count_ - 1 ];
			bullets_count_--;
			continue;
		}
	
		float d_pos[3];
		Vec3Mul( bullet->dir, bullet_travle_distance, d_pos );
		Vec3Add( bullet->pos, d_pos );
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
			mf_SoundEngine::Instance()->AddSingleSound( SoundPowerupPickup, 1.0f, 1.0f, NULL );

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

	// particels
	particles_manager_.Tick( dt );
	particles_manager_.AddEnginesTrail( player_->GetAircraft() );
	for( unsigned int i= 0; i< enemies_count_; i++ )
		particles_manager_.AddEnginesTrail( enemies_[i]->GetAircraft() );

	// sound
	for( unsigned int i= 0; i< enemies_count_ + 1; i++ )
	{
		mf_Aircraft* aircraft;
		mf_SoundSource* source;
		float volume_scaler;
		if( i == enemies_count_)
		{
			aircraft= player_->GetAircraft();
			source= player_sound_;
			volume_scaler= 1.0f;
		}
		else
		{
			aircraft= enemies_[i]->GetAircraft();
			source= enemies_sounds_[i];
			volume_scaler= 512.0f;
		}
		source->SetOrientation( aircraft->Pos(), aircraft->Velocity() );
		source->SetPitch( ThrottleToEngineSoundPitch( aircraft->Throttle() ) );
		source->SetVolume( volume_scaler * ThrottleToEngineSoundVolumeScaler( aircraft->Throttle() ) );
	}
}

void mf_GameLogic::PlayerShot( const float* dir )
{
	mf_Bullet* bullet= &bullets_[ bullets_count_ ];
	bullet->type= mf_Bullet::ChaingunBullet;
	bullet->owner= player_->GetAircraft();
	VEC3_CPY( bullet->pos, /*bullet->owner->Pos()*/player_->Pos() );
	Vec3Normalize( dir, bullet->dir );
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

void mf_GameLogic::SpawnEnemy()
{
	mf_Enemy* enemy= new mf_Enemy( mf_Aircraft::F2XXX, 100 );
	float spawn_pos[]= { float(level_.TerrainSizeX()/2) * level_.TerrainCellSize(), 0.0f, level_.TerrainAmplitude() };
	enemy->GetAircraft()->SetPos( spawn_pos );
	enemies_[ enemies_count_ ]= enemy;
	enemies_sounds_[ enemies_count_ ]= mf_SoundEngine::Instance()->CreateSoundSource( AircraftTypeToEngineSoundType( enemy->GetAircraft()->GetType() ) );
	enemies_sounds_[ enemies_count_ ]->Play();
	enemies_count_++;
}