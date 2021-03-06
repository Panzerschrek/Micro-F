#pragma once
#include "level.h"
#include "particles_manager.h"
#include "mf_math.h"
#include "aircraft.h"
#include "drawing_model.h"
class mf_Player;
class mf_Enemy;
class mf_SoundSource;

#define MF_MAX_POWERUPS 512
#define MF_MAX_BULLETS 256
#define MF_MAX_ROCKETS 128
#define MF_MAX_ENEMIES 32

#define MF_Y_LEVEL_BORDER 512.0f

struct mf_Powerup
{
	enum Type
	{
		Star,
		MiddleHealth,
		Rockets,
		Life,
		LastType
	};
	Type type;
	float pos[3];
	int stars_bonus;
	int health_bonus;
	int lifes_bonus;
	int rockets_bonus;
};

struct mf_Bullet
{
	enum Type
	{
		ChaingunBullet,
		AutomaticCannonShell,
		PlasmaShell,
		LastType
	};
	Type type;
	mf_Aircraft* owner;
	float pos[3];
	float dir[3];
	float velocity; // can be INF
};

struct mf_Rocket
{
	enum Type
	{
		PlasmaBall,
		LastType
	};
	Type type;
	float pos[3];
	float dir[3];
	float velocity;
	float spawn_time;
	mf_Aircraft* owner;
	mf_Aircraft* target;
};

class mf_GameLogic
{
public:
	mf_GameLogic( mf_Player* player, unsigned int seed );
	~mf_GameLogic();

	void StartGame();
	void StopGame();
	bool GameStarted() const;

	void Tick( float dt );
	void ShotBegin( mf_Aircraft* aircraft );
	void ShotContinue( mf_Aircraft* aircraft, float* dir, bool first_shot= false );
	void PlayerRocketShot( const float* dir );

	const mf_Level* GetLevel() const;
	const mf_ParticlesManager* GetParticlesManager() const;
	const mf_Powerup* GetPowerups() const;
	unsigned int GetPowerupCount() const;
	const mf_Enemy* const* GetEnemies() const;
	unsigned int GetEnemiesCount() const;

	mf_Rand* GetRandomizer();

private:
	void PlacePowerups();
	void SpawnEnemy();
	void DespawnEnemy( mf_Enemy* enemy );
	void OnAircraftHit( mf_Aircraft* aircraft, int damage );
	void RespawnPlayer();

private:
	mf_Level level_;
	mf_ParticlesManager particles_manager_;
	mf_Player* player_;

	bool game_started_;
	float game_start_time_;
	bool game_over_;

	mf_Rand randomizer_;

	mf_Powerup powerups_[ MF_MAX_POWERUPS ];
	unsigned int powerup_count_;

	mf_Bullet bullets_[ MF_MAX_BULLETS ];
	unsigned int bullets_count_;

	mf_Rocket rockets_[ MF_MAX_ROCKETS ];
	unsigned int rocket_count_;

	mf_Enemy* enemies_[ MF_MAX_ENEMIES ];
	mf_SoundSource* enemies_sounds_[ MF_MAX_ENEMIES ];
	unsigned int enemies_count_;

	mf_SoundSource* player_sound_;

	mf_DrawingModel aircrafts_models_[ mf_Aircraft::LastType ];

	float player_last_shot_time_;
};

inline bool mf_GameLogic::GameStarted() const
{
	return game_started_;
}

inline const mf_Level* mf_GameLogic::GetLevel() const
{
	return &level_;
}

inline const mf_ParticlesManager* mf_GameLogic::GetParticlesManager() const
{
	return &particles_manager_;
}

inline const mf_Powerup* mf_GameLogic::GetPowerups() const
{
	return powerups_;
}

inline unsigned int mf_GameLogic::GetPowerupCount() const
{
	return powerup_count_;
}

inline const mf_Enemy* const* mf_GameLogic::GetEnemies() const
{
	return enemies_;
}

inline unsigned int mf_GameLogic::GetEnemiesCount() const
{
	return enemies_count_;
}

inline mf_Rand* mf_GameLogic::GetRandomizer()
{
	return &randomizer_;
}