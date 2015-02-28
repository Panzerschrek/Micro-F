#pragma once
#include "micro-f.h"

class mf_DrawingModel;

// segments - east to west
// partition - south to north
void GenGeosphere( mf_DrawingModel* model, unsigned int segments, unsigned int partition );