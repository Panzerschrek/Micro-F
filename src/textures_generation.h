#pragma once
#include "micro-f.h"

class mf_Texture;

enum mf_TerrainTexture
{
	TextureDirt,
	TextureDirwWithGrass,
	LastTexture
};
extern void (* const terrain_texture_gen_func[])(mf_Texture* t);

void GenF1949Texture( mf_Texture* tex );

void GenDirtTexture( mf_Texture* tex );
void GenDirtWithGrassTexture( mf_Texture* tex );