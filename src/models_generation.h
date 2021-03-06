#pragma once
#include "micro-f.h"

class mf_DrawingModel;

// segments - east to west
// partition - south to north
void GenGeosphere( mf_DrawingModel* model, unsigned int segments, unsigned int partition );
void GenCylinder( mf_DrawingModel* model, unsigned int segments, unsigned int partition, bool gen_caps );

void GenForcefieldModel( mf_DrawingModel* model, float radius, float length, const float* pos );
void GenSkySphere( mf_DrawingModel* model, unsigned int partitiion );

extern void (* const level_static_models_gen_func[])(mf_DrawingModel* model);

void GenPalm( mf_DrawingModel* model );
void GenOak( mf_DrawingModel* model );