#include "micro-f.h"
#include "game_logic.h"
#include "main_loop.h"
#include "sound_engine.h"
#include "../models/models.h"

#include "player.h"
#include "mf_math.h"
#include "enemy.h"

#define MF_FORCEFIELD_DAMAGE_RADIUS 16.0f
#define MF_ROCKET_LIFETIME 12.0f
#define MF_ROCKET_HIT_DISTANCE 10.0f

#define MF_BLAST_SOUND_VOLUME 1024.0f
#define MF_ROCKET_BLAST_SOUND_VOLUME 384.0f
#define MF_MACHINEGUN_SHOT_VOLUME 512.0f

#define MF_MAX_ALIVE_ENEMIES 2
#define MF_SCORE_PER_ENEMY 10
#define MF_SCORE_PER_STAR 13

#define MF_PLAYER_DEATH_HP_BORDER (-250)

namespace PowerupsTables
{

static const int stars_bonus_table[mf_Powerup::LastType]=
{
	MF_SCORE_PER_STAR, 0, 0, 0
};

static const int health_bonus_table[mf_Powerup::LastType]=
{
	0, 200, 0, 0
};

static const int rockets_bonus_table[mf_Powerup::LastType]=
{
	0, 0, 3, 0
};

static const int lifes_bonus_table[mf_Powerup::LastType]=
{
	0, 0, 0, 1
};

} // namespace PowerupsTables

namespace Tables
{

static const int bullets_damage_table[ mf_Bullet::LastType ]=
{
	10, 20, 15
};

static const int rockets_damage_table[ mf_Rocket::LastType ]=
{
	50
};

static const float aircraft_primary_weapon_freq[ mf_Aircraft::LastType ]=
{
	8.0f, 7.0f, 10.0f
};

} // namespace Tables

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

mf_GameLogic::mf_GameLogic( mf_Player* player, unsigned int seed )
	: level_(seed)
	, particles_manager_()
	, player_(player)
	, randomizer_(seed)
	, game_started_(false), game_over_(false)
	, powerup_count_(0), bullets_count_(0), rocket_count_(0), enemies_count_(0)
	, player_sound_(NULL)
{
	for( unsigned int i= 0; i< mf_Aircraft::LastType; i++ )
		aircrafts_models_[i].LoadFromMFMD( mf_Models::aircraft_models[i] );

	PlacePowerups();
}

mf_GameLogic::~mf_GameLogic()
{
}

void mf_GameLogic::StartGame()
{
	game_started_= true;
	game_start_time_= mf_MainLoop::Instance()->CurrentTime();
}

void mf_GameLogic::StopGame()
{
	game_started_= false;
	PlacePowerups();

	mf_SoundEngine::Instance()->DestroySoundSource( player_sound_ );
	player_sound_= NULL;

	for( unsigned int i= 0; i< enemies_count_; i++ )
	{
		mf_SoundEngine::Instance()->DestroySoundSource( enemies_sounds_[i] );
		player_->RemoveEnemyAircraft( enemies_[i]->GetAircraft() );
		player_->SetTargetAircraft( NULL );
		delete enemies_[i];
	}
	enemies_count_= 0;
	bullets_count_= 0;
	rocket_count_= 0;

	particles_manager_.KillAllParticles();
}

void mf_GameLogic::Tick( float dt )
{
	{ // try spawn new enemies
		/*unsigned int alive_enemies_count= 0;
		for( unsigned int i= 0; i< enemies_count_; i++ )
			if( enemies_[i]->GetAircraft()->HP() > 0 )
				alive_enemies_count++;*/
		unsigned int alive_enemies_count= enemies_count_;
		if( alive_enemies_count < MF_MAX_ALIVE_ENEMIES )
		{
			if( ( randomizer_.Rand() & 127 ) == 0 )
				SpawnEnemy();
		}
	}

	{ // select enemy
		mf_MainLoop* main_loop= mf_MainLoop::Instance();
		float player_view_dir[3];
		player_->ScreenPointToWorldSpaceVec( main_loop->ViewportWidth() / 2, main_loop->ViewportHeight() / 2, player_view_dir );
		float max_angle_cos= -1.0f;
		unsigned int nearest_enemy_index= 0xFFFFFFFF;
		for( unsigned int i= 0; i< enemies_count_; i++ )
		{
			float vec_to_enemy[3];
			Vec3Sub( enemies_[i]->GetAircraft()->Pos(), player_->GetAircraft()->Pos(), vec_to_enemy );
			Vec3Normalize( vec_to_enemy );
			float angle_cos= Vec3Dot( vec_to_enemy, player_view_dir );
			if( angle_cos > max_angle_cos )
			{
				max_angle_cos= angle_cos;
				nearest_enemy_index= i;
			}
		}
		if( enemies_count_ != 0 )
			player_->SetTargetAircraft( enemies_[nearest_enemy_index]->GetAircraft() );
		else
			player_->SetTargetAircraft( NULL );
	}

	const float current_time= mf_MainLoop::Instance()->CurrentTime();

	mf_Aircraft* player_aircraft= player_->GetAircraft();

	const float c_powerup_pick_distance2= 16.0f * 16.0f;

	// bullets
	for( unsigned int i= 0; i< bullets_count_; )
	{
		mf_Bullet* bullet= bullets_ + i;
		float intersection_pos[3];
		float bullet_travle_distance= dt * bullet->velocity;
		bool is_intersection= level_.BeamIntersectTerrain( bullet->pos, bullet->dir, bullet_travle_distance, false, intersection_pos );
		mf_Aircraft* hited_target= NULL;

		// search intersection with enemies
		for( unsigned int e= 0; e< enemies_count_ + 1; e++ )
		{
			float aircraft_space_dir[3];
			float aircraft_space_pos[3];
			float aircraft_space_hit_pos[3];
			mf_Aircraft* enemy_aircraft= e == enemies_count_ ? player_->GetAircraft() : enemies_[e]->GetAircraft();

			float pos_relative_aircraft[3];
			Vec3Sub( bullet->pos, enemy_aircraft->Pos(), pos_relative_aircraft );

			for( unsigned int j= 0; j< 3; j++ )
			{
				aircraft_space_pos[j]= Vec3Dot( pos_relative_aircraft, enemy_aircraft->AxisVec(j) );
				aircraft_space_dir[j]= Vec3Dot( bullet->dir, enemy_aircraft->AxisVec(j) );
			}

			if( enemy_aircraft != bullet->owner )
			{
				if( aircrafts_models_[enemy_aircraft->GetType()].BeamIntersectModel( aircraft_space_pos, aircraft_space_dir, bullet_travle_distance, aircraft_space_hit_pos ) )
				{
					for( unsigned int j= 0; j< 3; j++ )
						intersection_pos[j]=
							enemy_aircraft->AxisVec(0)[j] * aircraft_space_hit_pos[0] +
							enemy_aircraft->AxisVec(1)[j] * aircraft_space_hit_pos[1] +
							enemy_aircraft->AxisVec(2)[j] * aircraft_space_hit_pos[2] +
							enemy_aircraft->Pos()[j];
					is_intersection= true;
					hited_target= enemy_aircraft;
				}
			}
		} // for enemies

		if( is_intersection )
		{
			if( hited_target != NULL )
			{
				OnAircraftHit( hited_target, Tables::bullets_damage_table[ bullet->type ] );
				particles_manager_.AddBulletHit( intersection_pos, hited_target->Velocity() );
			}
			else
			{
				static const float vel[3]= { 0.0001f, 0.0001f, 0.0001f };
				particles_manager_.AddBulletHit( intersection_pos, vel );
			}
		}

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

	// rockets
	for( unsigned int i= 0; i< rocket_count_; )
	{
		mf_Rocket* rocket= rockets_ + i;

		float vec_to_target[3];
		if( current_time - rocket->spawn_time > 0.4f && rocket->target != NULL ) // turn rocket if we are far from owner aircraft
		{
			Vec3Sub( rocket->target->Pos(), rocket->pos, vec_to_target );
			Vec3Normalize( vec_to_target );
			float angle_to_target_cos= Vec3Dot( vec_to_target, rocket->dir );
			if( angle_to_target_cos < 0.9999f )
			{
				float rotation_vec[3];
				Vec3Cross( vec_to_target, rocket->dir, rotation_vec );
				float rot_mat[16];
				const float rotation_speed= 3.0f * MF_PI4; // radians per second
				float d_angle= min( rotation_speed * dt, mf_Math::acos(angle_to_target_cos) );
				Mat4RotateAroundVector( rot_mat, rotation_vec, -d_angle );

				Vec3Mat4Mul( rocket->dir, rot_mat );
				Vec3Normalize( rocket->dir );
			}
		}

		bool is_rocket_dead= false;
		float terrain_hit_pos[3];

		if( level_.BeamIntersectTerrain( rocket->pos, rocket->dir, dt * rocket->velocity, false, terrain_hit_pos ) )
		{
			is_rocket_dead= true;
			particles_manager_.AddRocketBlast( terrain_hit_pos );
			mf_SoundEngine::Instance()->AddSingleSound( SoundBlast, MF_ROCKET_BLAST_SOUND_VOLUME, 1.0f, terrain_hit_pos );
		}
		else
		{
			float pos_add[3];
			Vec3Mul( rocket->dir, dt * rocket->velocity, pos_add );
			Vec3Add( rocket->pos, pos_add );
		}

		if( rocket->target != NULL && Distance( rocket->pos, rocket->target->Pos() ) < MF_ROCKET_HIT_DISTANCE )
		{
			OnAircraftHit( rocket->target, Tables::rockets_damage_table[ rocket->type ] );
			is_rocket_dead= true;
			particles_manager_.AddRocketBlast( rocket->pos );
			mf_SoundEngine::Instance()->AddSingleSound( SoundBlast, MF_ROCKET_BLAST_SOUND_VOLUME, 1.0f, rocket->pos );
		}

		if( is_rocket_dead || current_time > rocket->spawn_time + MF_ROCKET_LIFETIME )
		{
			if( i!= rocket_count_ - 1 )
				rockets_[i]= rockets_[ rocket_count_ - 1 ];
			rocket_count_--;
			continue;
		}
		i++;
	}

	// powerups
	for( unsigned int i= 0; i< powerup_count_; i++ )
	{
		float vec_to_powerup[3];
		Vec3Sub( player_aircraft->Pos(), powerups_[i].pos, vec_to_powerup );
		float dst2= Vec3Dot( vec_to_powerup, vec_to_powerup );
		if( dst2 < c_powerup_pick_distance2 )
		{
			player_aircraft->AddHP( powerups_[i].health_bonus );
			player_aircraft->AddRockets( powerups_[i].rockets_bonus );
			player_->AddScorePoints( powerups_[i].stars_bonus );
			player_->AddLifes( powerups_[i].lifes_bonus );
			mf_SoundEngine::Instance()->AddSingleSound( SoundPowerupPickup, 1.0f, 1.0f, NULL );

			if( i < powerup_count_ - 1 )
				powerups_[i]= powerups_[ powerup_count_ - 1 ];
			powerup_count_--;
			break;
		}
	}

	// check collision of player with terrain
	if( level_.SphereIntersectTerrain( player_aircraft->Pos(), 4.0f ) )
	{
		RespawnPlayer();
	}
	// check collision of enemies with terrain
	for( unsigned int i= 0; i< enemies_count_; )
	{
		if( level_.SphereIntersectTerrain( enemies_[i]->GetAircraft()->Pos(), 4.0f ) )
		{
			particles_manager_.AddBlast( enemies_[i]->GetAircraft()->Pos() );
			mf_SoundEngine::Instance()->AddSingleSound( SoundBlast, MF_BLAST_SOUND_VOLUME, 1.0f, enemies_[i]->GetAircraft()->Pos() );
			DespawnEnemy( enemies_[i] );
			continue;
		}
		i++;
	}

	{ // calculate collision with forcefield
		float vec_to_forcefield[3];
		vec_to_forcefield[0]= player_aircraft->Pos()[0] - float(level_.TerrainSizeX()) * level_.TerrainCellSize() * 0.5f;
		vec_to_forcefield[1]= 0;
		vec_to_forcefield[2]= player_aircraft->Pos()[2] - level_.ForcefieldZPos();
		float dist_to_forcefield= Vec3Len( vec_to_forcefield );
		if( level_.ForcefieldRadius() - MF_FORCEFIELD_DAMAGE_RADIUS < dist_to_forcefield
			|| player_aircraft->Pos()[1] < MF_Y_LEVEL_BORDER )
			player_aircraft->AddHP( -int( mf_Math::round( 800.0f * dt ) ) );
	}

	for( unsigned int i= 0; i< enemies_count_; i++ )
		enemies_[i]->Tick(dt);

	if( !player_->IsInRespawn() && !game_over_ )
	{ // check player after enemies tick
		if( player_aircraft->HP() <= MF_PLAYER_DEATH_HP_BORDER )
			RespawnPlayer();
	}

	// particels
	particles_manager_.Tick( dt );
	if( !player_->IsInRespawn() && !game_over_ )
		particles_manager_.AddEnginesTrail( player_->GetAircraft() );
	for( unsigned int i= 0; i< enemies_count_; i++ )
		particles_manager_.AddEnginesTrail( enemies_[i]->GetAircraft() );
	for( unsigned int i= 0; i< powerup_count_; i++ )
	{
		float glow_factor= ( Distance( player_->GetAircraft()->Pos(), powerups_[i].pos ) - 128.0f ) / 512.0f;
		glow_factor= mf_Math::clamp( 0.0f, 1.0f, glow_factor );
		particles_manager_.AddPowerupGlow( powerups_[i].pos , glow_factor );
	}
	for( unsigned int i= 0; i< rocket_count_; i++ )
	{
		particles_manager_.AddPlasmaBall( rockets_[i].pos );
		particles_manager_.AddRocketTrail( &rockets_[i] );
	}
	// show finish  and start text
	{
		static const char* const c_finish_text= "FINISH";
		static const char* const c_back_border_text= "DEATH";
		static const float c_letter_pixel_size= 8.0f;
		float pos[3];
		pos[0]= float( level_.TerrainSizeX() / 2 ) * level_.TerrainCellSize() - float(strlen(c_finish_text) * MF_LETTER_WIDTH) * 0.5f * c_letter_pixel_size;
		pos[1]= float(level_.TerrainSizeY()) * level_.TerrainCellSize() - MF_Y_LEVEL_BORDER;
		pos[2]= level_.TerrainAmplitude() * 0.75f;

		static const float c_x_axis[3]= { c_letter_pixel_size, 0.0f, 0.0f };
		static const float c_x_axis_invert[3]= { -c_letter_pixel_size, 0.0f, 0.0f };
		static const float c_y_axis[3]= { 0.0f, 0.0f, c_letter_pixel_size };

		particles_manager_.AddFlashingText( pos, c_x_axis, c_y_axis, c_finish_text );

		pos[0]= float( level_.TerrainSizeX() / 2 ) * level_.TerrainCellSize() + float(strlen(c_back_border_text) * MF_LETTER_WIDTH) * 0.5f * c_letter_pixel_size;
		pos[1]= MF_Y_LEVEL_BORDER;
		particles_manager_.AddFlashingText( pos, c_x_axis_invert, c_y_axis, c_back_border_text );
	}

	// sound
	if( player_sound_ == NULL )
	{
		player_sound_= mf_SoundEngine::Instance()->CreateSoundSource( AircraftTypeToEngineSoundType( player_->GetAircraft()->GetType() ) );
		player_sound_->Play();
	}
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
	
	// check win/loose
	if( game_over_ )
	{
		mf_MainLoop::Instance()->Loose();
		game_over_= false;
	}
	else if( player_aircraft->Pos()[1] >= float(level_.TerrainSizeY()) * level_.TerrainCellSize() - MF_Y_LEVEL_BORDER )
		mf_MainLoop::Instance()->Win( mf_MainLoop::Instance()->CurrentTime() - game_start_time_ );
}

void mf_GameLogic::ShotBegin( mf_Aircraft* aircraft )
{
	aircraft->MachinegunShot( mf_MainLoop::Instance()->CurrentTime() );
}

void mf_GameLogic::ShotContinue( mf_Aircraft* aircraft, float* dir, bool first_shot )
{
	const float c_machinegun_freq= Tables::aircraft_primary_weapon_freq[ aircraft->GetType() ];

	float dt= mf_MainLoop::Instance()->CurrentTime() - aircraft->LastMachinegunShotTime();
	if( dt * c_machinegun_freq >= 1.0f || first_shot )
	{
		float unused;
		aircraft->MachinegunShot( mf_MainLoop::Instance()->CurrentTime() - modf( dt * c_machinegun_freq, &unused ) / c_machinegun_freq );

		mf_Bullet::Type bullet_type;
		mf_SoundType sound_type;
		switch( aircraft->GetType() )
		{
			case mf_Aircraft::F1949:
				bullet_type= mf_Bullet::AutomaticCannonShell; sound_type= SoundAutomaticCannonShot;
			break;
			case mf_Aircraft::F2XXX:
				bullet_type= mf_Bullet::PlasmaShell; sound_type= SoundPlasmagunShot;
			break;
			case mf_Aircraft::V1:
				bullet_type= mf_Bullet::ChaingunBullet; sound_type= SoundMachinegunShot;
			break;
			default:
				bullet_type= mf_Bullet::ChaingunBullet; sound_type= SoundMachinegunShot; MF_ASSERT(false);
			break;
		};

		mf_Bullet* bullet= &bullets_[ bullets_count_ ];
		bullet->type= bullet_type;
		bullet->owner= aircraft;
		VEC3_CPY( bullet->pos, aircraft->Pos() );

		if( aircraft == player_->GetAircraft() )
		{
			if( player_->TargetAircraft() != NULL )
			{
				Vec3Sub( player_->TargetAircraft()->Pos(), bullet->owner->Pos(), bullet->dir );
				Vec3Normalize( bullet->dir );
				if( Vec3Dot( bullet->dir, bullet->owner->AxisVec(1) ) >= mf_Math::cos(bullet->owner->GetMachinegunConeAngle()) )
				{}
				else Vec3Normalize( bullet->owner->AxisVec(1), bullet->dir );
			}
			else Vec3Normalize( bullet->owner->AxisVec(1), bullet->dir );
		}
		else Vec3Normalize( dir, bullet->dir );
		for( unsigned int i= 0; i< 3; i++ )
			bullet->dir[i] += randomizer_.RandF( -0.01f, 0.01f );
		Vec3Normalize( bullet->dir );

		bullet->velocity= mfInf();

		bullets_count_++;
		mf_SoundEngine::Instance()->AddSingleSound( sound_type, MF_MACHINEGUN_SHOT_VOLUME, 1.0f, aircraft->Pos(), aircraft->Velocity() );
	}
}

void mf_GameLogic::PlayerRocketShot( const float* dir )
{
	mf_Aircraft* aircraft= player_->GetAircraft();
	if( aircraft->RocketsCount() > 0 )
	{
		mf_Rocket* rocket= rockets_ + rocket_count_;

		VEC3_CPY( rocket->dir, dir );
		VEC3_CPY( rocket->pos, player_->GetAircraft()->Pos() );
		rocket->velocity= 100.0f;
		rocket->type= mf_Rocket::PlasmaBall;
		rocket->owner= player_->GetAircraft();
		rocket->target= player_->TargetAircraft();
		rocket->spawn_time= mf_MainLoop::Instance()->CurrentTime();

		aircraft->AddRockets( -1 );
		rocket_count_++;
	}
}

void mf_GameLogic::PlacePowerups()
{
	powerup_count_= 0;

	static const float c_stars_step_range[2]= { 128.0f, 256.0f };
	static const unsigned int c_powerup_place_chance[ mf_Powerup::LastType ]=
		{ 10, 15, 7, 3 };
	unsigned int c_chance_sum= 0;
	for( unsigned int i= 0 ; i< mf_Powerup::LastType; i++ )
		c_chance_sum+= c_powerup_place_chance[i];

	float y= 32.0f;
	while( y < level_.TerrainSizeY() * level_.TerrainCellSize() )
	{
		// select type
		mf_Powerup::Type type= mf_Powerup::LastType;
		unsigned int rand_val= randomizer_.Rand() % c_chance_sum;
		unsigned int prev_sum= 0;
		for( unsigned int i= 0; i< mf_Powerup::LastType; i++ )
		{
			if( rand_val >= prev_sum && rand_val < prev_sum + c_powerup_place_chance[i] )
			{
				type= mf_Powerup::Type(i);
				break;
			}
			prev_sum+= c_powerup_place_chance[i];
		}
		MF_ASSERT( type != mf_Powerup::LastType );

		powerups_[powerup_count_].type= type;
		powerups_[powerup_count_].stars_bonus= PowerupsTables::stars_bonus_table[type];
		powerups_[powerup_count_].health_bonus= PowerupsTables::health_bonus_table[type];
		powerups_[powerup_count_].rockets_bonus= PowerupsTables::rockets_bonus_table[type];
		powerups_[powerup_count_].lifes_bonus= PowerupsTables::lifes_bonus_table[type];
		powerups_[powerup_count_].pos[0]= level_.GetValleyCenterX( y );
		powerups_[powerup_count_].pos[1]= y;
		powerups_[powerup_count_].pos[2]= level_.TerrainWaterLevel() + 0.25f * randomizer_.RandF( 30.0f, level_.TerrainAmplitude() );
		powerup_count_++;

		y+= randomizer_.RandF( c_stars_step_range[0], c_stars_step_range[1] );
	}
}

void mf_GameLogic::SpawnEnemy()
{
	mf_Enemy* enemy= new mf_Enemy(
		mf_Aircraft::Type( randomizer_.Rand() % mf_Aircraft::LastType ),
		100,
		this,
		player_->GetAircraft() );
	enemies_[ enemies_count_ ]= enemy;
	enemies_sounds_[ enemies_count_ ]= mf_SoundEngine::Instance()->CreateSoundSource( AircraftTypeToEngineSoundType( enemy->GetAircraft()->GetType() ) );
	enemies_sounds_[ enemies_count_ ]->Play();
	enemies_count_++;

	player_->AddEnemyAircraft( enemy->GetAircraft() );
}

void mf_GameLogic::DespawnEnemy( mf_Enemy* enemy )
{
	if( enemy->GetAircraft() == player_->TargetAircraft() )
	{
		player_->SetTargetAircraft( NULL );
	}
	player_->RemoveEnemyAircraft( enemy->GetAircraft() );

	for( unsigned int i= 0; i < rocket_count_; i++ )
		if( rockets_[i].target == enemy->GetAircraft() ) rockets_[i].target= NULL;

	unsigned int ind= 0;
	for( unsigned int i= 0; i< enemies_count_; i++ )
		if( enemies_[i] == enemy )
		{
			ind= i; break;
		}

	mf_SoundEngine::Instance()->DestroySoundSource( enemies_sounds_[ind] );
	delete enemy;

	if( ind != enemies_count_ - 1 )
	{
		enemies_sounds_[ind]= enemies_sounds_[ enemies_count_ - 1 ];
		enemies_[ind]= enemies_[ enemies_count_ - 1 ];
	}
	enemies_count_--;
}

void mf_GameLogic::OnAircraftHit( mf_Aircraft* aircraft, int damage )
{
	int old_hp= aircraft->HP();
	aircraft->AddHP( -damage );
	if( aircraft != player_->GetAircraft() && aircraft->HP() <= 0 ) aircraft->SetThrottle(0);

	if( old_hp > 0 && aircraft->HP() <= 0 )
	{
		player_->AddScorePoints( MF_SCORE_PER_ENEMY );
	}
}

void mf_GameLogic::RespawnPlayer()
{
	if( player_->IsInRespawn() )
		return;

	particles_manager_.AddBlast( player_->GetAircraft()->Pos() );
	mf_SoundEngine::Instance()->AddSingleSound( SoundBlast, MF_BLAST_SOUND_VOLUME, 1.0f, player_->GetAircraft()->Pos() );

	mf_SoundEngine::Instance()->DestroySoundSource( player_sound_ );
	player_sound_= NULL;

	float pos[3];
	pos[1]= player_->GetAircraft()->Pos()[1] - 64.0f;
	if( pos[1] < MF_Y_LEVEL_BORDER ) pos[1]= MF_Y_LEVEL_BORDER;
	pos[2]= 1.25f * level_.TerrainAmplitude();
	pos[0]= level_.GetValleyCenterX( pos[1] );

	if( player_->TryRespawn( pos ) )
	{
	}
	else game_over_= true;
}