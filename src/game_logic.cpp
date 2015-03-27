#include "micro-f.h"
#include "game_logic.h"

#include "player.h"

mf_GameLogic::mf_GameLogic(mf_Player* player)
	: level_()
	, player_(player)
{
}

mf_GameLogic::~mf_GameLogic()
{
}