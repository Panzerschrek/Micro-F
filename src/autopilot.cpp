#include "micro-f.h"

#include "autopilot.h"

mf_Autopilot::mf_Autopilot( const mf_Aircraft* aircraft )
	: mode_(ModeDisabled)
	, aircraft_(aircraft)
{
}

mf_Autopilot::~mf_Autopilot()
{
}