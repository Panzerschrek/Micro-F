#pragma once
#include "micro-f.h"
#include "aircraft.h"
#include "autopilot.h"

class mf_GameLogic;

class mf_Enemy
{
public:
	mf_Enemy( mf_Aircraft::Type type, int hp, mf_GameLogic* game_logic, const mf_Aircraft* player_aircraft );
	~mf_Enemy();

	const mf_Aircraft* GetAircraft() const;
	mf_Aircraft* GetAircraft();

	void Tick( float dt );

private:
	mf_Aircraft aircraft_;
	mf_Autopilot autopilot_;

	mf_GameLogic* game_logic_;
	const mf_Aircraft* player_aircraft_;
	bool shot_pressed_;
};

inline const mf_Aircraft* mf_Enemy::GetAircraft() const
{
	return &aircraft_;
}

inline mf_Aircraft* mf_Enemy::GetAircraft()
{
	return &aircraft_;
}