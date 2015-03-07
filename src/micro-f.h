#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include <windows.h>

#include <Gl/gl.h>
#include "gl/glcorearb.h"
#include "gl/wglext.h"
#include "gl/func_declarations.h"

#ifdef MF_DEBUG

#define MF_DEBUG_INFO_STR(str)\
{\
	printf( "%s\n", str );\
}

#define MF_DEBUG_INFO_STR_I(str,integer)\
{\
printf( "%s%d\n", str, integer );\
}

#include <assert.h>
#define MF_ASSERT(x) assert(x)

#else // MF_DEBUG

#define MF_DEBUG_INFO_STR(str)
#define MF_DEBUG_INFO_STR_I(str,integer)
#define MF_ASSERT(x)

#endif // MF_DEBUG