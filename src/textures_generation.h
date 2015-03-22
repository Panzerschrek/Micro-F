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

extern void (* const terrain_texture_gen_func[])(mf_Texture* t);
extern void (* const aircraft_texture_gen_func[])(mf_Texture* t);
extern void (* const gui_texture_gen_func[])(mf_Texture* t);
extern void (* const static_level_object_texture_gen_func[])(mf_Texture* t);

void GenNaviballTexture( mf_Texture* tex );
void GenControlPanelTexture( mf_Texture* tex );
void GenThrottleBarTexture( mf_Texture* tex );
void GenThrottleIndicatorTexture( mf_Texture* tex );
void GenVerticalSpeedIndicatorTexture( mf_Texture* tex );
void GenGuiButtonTexture( mf_Texture* tex );
void GenMenuBackgroundTexture( mf_Texture* tex );

void GenF1949Texture( mf_Texture* tex );
void GenF2XXXTexture( mf_Texture* tex );
void GenV1Texture( mf_Texture* tex );

void GenSunTexture( mf_Texture* tex );
void GenMoonTexture( mf_Texture* textures ); // input - array of six textures - cubemap sides
void GenSkyboxTexture( mf_Texture* textures ); // input - array of six cubemaps

void GenDirtTexture( mf_Texture* tex );
void GenDirtWithGrassTexture( mf_Texture* tex );
void GenSandTexture( mf_Texture* tex );
void GenRockTexture( mf_Texture* tex );