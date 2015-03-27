#pragma once
#include "level.h"

class mf_Player;

class mf_GameLogic
{
public:
	mf_GameLogic( mf_Player* player );
	~mf_GameLogic();

	const mf_Level* GetLevel() const;

private:
	mf_Level level_;
	mf_Player* player_;
};

inline const mf_Level* mf_GameLogic::GetLevel() const
{
	return &level_;
}