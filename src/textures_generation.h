#pragma once
#include "micro-f.h"

class mf_Texture;

enum mf_TerrainTexture
{
	TextureDirt,
	TextureDirtWithGrass,
	TextureSand,
	TextureRock,
	LastTexture
};
extern void (* const terrain_texture_gen_func[])(mf_Texture* t);

void GenF1949Texture( mf_Texture* tex );

void GenSunTexture( mf_Texture* tex );

void GenDirtTexture( mf_Texture* tex );
void GenDirtWithGrassTexture( mf_Texture* tex );
void GenSandTexture( mf_Texture* tex );
void GenRockTexture( mf_Texture* tex );