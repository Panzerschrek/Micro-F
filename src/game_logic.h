#pragma once
#include "level.h"

class mf_Player;


struct mf_Powerup
{
	enum Type
	{
		Star,
		LastType
	};
	Type type;
	float pos[3];
	int health_bonus;
	int stars_bonus;
};

class mf_GameLogic
{
public:
	mf_GameLogic( mf_Player* player );
	~mf_GameLogic();

	void Tick( float dt );

	const mf_Level* GetLevel() const;
	const mf_Powerup* GetPowerups() const;
	unsigned int GetPowerupCount() const;

private:
	void PlaceStars();

private:
	mf_Level level_;
	mf_Player* player_;

	mf_Powerup* powerups_;
	unsigned int powerup_count_;
};

inline const mf_Level* mf_GameLogic::GetLevel() const
{
	return &level_;
}

inline const mf_Powerup* mf_GameLogic::GetPowerups() const
{
	return powerups_;
}

inline unsigned int mf_GameLogic::GetPowerupCount() const
{
	return powerup_count_;
}