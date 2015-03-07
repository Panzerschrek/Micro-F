#include "models.h"
#include "../src/aircraft.h"

namespace mf_Models
{

const unsigned char f1949[]=
#include "f1949.h"
;

const unsigned char f2xxx[]=
#include "f2xxx.h"
;

const unsigned char cube[]=
#include "cube.h"
;

const unsigned char icosahedron[]=
#include "icosahedron.h"
;

const unsigned char star[]=
#include "star.h"
;

const unsigned char* aircraft_models[mf_Aircraft::LastType]=
{
	f1949,
	f2xxx
};

} // namespace mf_Models