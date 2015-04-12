#include "models.h"
#include "../src/aircraft.h"
#include "../src/game_logic.h"

namespace mf_Models
{

const unsigned char f1949[]=
#include "f1949.h"
;
const unsigned char f2xxx[]=
#include "f2xxx.h"
;
const unsigned char v1[]=
#include "v-1.h"
;

const unsigned char star[]=
#include "star.h"
;
const unsigned char wrench_hammer[]=
#include "wrench-hammer.h"
;
const unsigned char rockets[]=
#include "rockets.h"
;

const unsigned char cube[]=
#include "cube.h"
;

const unsigned char icosahedron[]=
#include "icosahedron.h"
;

const unsigned char leafs[]=
#include "leafs_icosahedron.h"
;

const unsigned char* aircraft_models[ mf_Aircraft::LastType ]=
{
	f1949,
	f2xxx,
	v1
};

const unsigned char* powerups_models[ mf_Powerup::LastType ]=
{
	star,
	wrench_hammer,
	rockets
};

} // namespace mf_Models