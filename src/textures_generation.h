#pragma once
#include "micro-f.h"

class mf_Texture;

enum mf_TerrainTexture
{
	TextureDirt,
	TextureDirtWithGrass,
	TextureSand,
	TextureRock,
	LastTerrainTexture
};

enum mf_GuiTexture
{
	TextureNaviball,
	TextureControlPanel,
	TextureThrottleBar,
	TextureThrottleIndicator,
	TextureVerticalSpeedIndicator,
	TextureNaviballGlass,
	TextureGuiButton,
	TextureMenuBackgound,
	LastGuiTexture
};

enum mf_StaticLevelObjectTexture
{
	TexturePalmBark,
	TextureOakBark,
	TextureOakLeafs,
	TextureSpruceBark,
	TextureSpruceBranch,
	TextureBirchBark,
	LastStaticLevelObjectTexture
};

extern void (* const terrain_texture_gen_func[])(mf_Texture* t);
extern void (* const aircraft_texture_gen_func[])(mf_Texture* t);
extern void (* const gui_texture_gen_func[])(mf_Texture* t);
extern void (* const static_level_object_texture_gen_func[])(mf_Texture* t);

void GenSunTexture( mf_Texture* tex );
void GenMoonTexture( mf_Texture* textures ); // input - array of six textures - cubemap sides

void GenSmokeParticle( mf_Texture* tex );