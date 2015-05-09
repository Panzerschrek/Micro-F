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
	TextureNumbers,
	TextureNaviballGlass,
	TextureGuiButton,
	TextureMenuBackgound,
	TextureTargetAircraft,
	LastGuiTexture
};

enum mf_StaticLevelObjectTexture
{
	TextureOakBark,
	TextureOakLeafs,
	TextureSpruceBark,
	TextureSpruceBranch,
	TextureBirchBark,
	TextureBirchLeafs,
	LastStaticLevelObjectTexture
};

enum mf_ParticleTexture
{
	TextureEngineSmoke,
	TextureEngineFire,
	TextureEnginePlasma,
	TexturePowerupGlow,
	TexturePlasmaBall,
	TextureFire,
	LastParticleTexture
};

extern void (* const terrain_texture_gen_func[])(mf_Texture* t);
extern void (* const aircraft_texture_gen_func[])(mf_Texture* t);
extern void (* const gui_texture_gen_func[])(mf_Texture* t);
extern void (* const static_level_object_texture_gen_func[])(mf_Texture* t);
extern void (* const particles_texture_gen_func[])(mf_Texture* t);

void GenSunTexture( mf_Texture* tex );
void GenMoonTexture( mf_Texture* tex );
void GenForcefieldTexture( mf_Texture* tex );

void GenSmokeParticle( mf_Texture* tex );