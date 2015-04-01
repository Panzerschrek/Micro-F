#pragma once
#include "micro-f.h"
#include "mf_math.h"

class mf_Aircraft;

/*
 Input - texture with premultipled alpha.
 Blending mode: GL_ONE, GL_ONE_MINUS_SRC_ALPHA
 color.xyz=
	albedo_tex.xyz * sun_light * albedo_tex.a * ( 1.0 - background_multipler ) ) +
	brightness_tex.xyz * luminance
 color.a= background_multipler * albedo_tex.a
*/
#pragma pack(push, 1)
struct mf_ParticleVertex
{
	float pos_size[4];
	float luminance;
	unsigned char transparency_multipler__backgound_multipler__texture_id__reserved[4];

};
#pragma pack(pop)

#define MF_MAX_PARTICLES 8192

class mf_ParticlesManager
{
public:

	enum ParticleTexture
	{
		TextureEngineSmoke,
		TextureEngineFire,
		LastTexture
	};

	mf_ParticlesManager();
	~mf_ParticlesManager();

	void Tick( float dt );
	void AddEnginesTrail( const mf_Aircraft* aircraft );
	void AddBlast( const float* pos );

	unsigned int GetParticlesCount() const;
	void PrepareParticlesVertices( mf_ParticleVertex* out_vertices ) const;
private:

	struct Particle
	{
		enum Type
		{
			JetEngineTrail,
			LastType
		};
		Type type;
		float pos[3];
		float direction[3];
		float velocity;
		float acceleration;
		float spawn_time;
		float life_time;
	};

	float prev_tick_time_;
	float current_tick_time_;
	float dt_;
	mf_Rand randomizer_;
	unsigned int spawn_counter_;

	Particle particles_[ MF_MAX_PARTICLES ];
	unsigned int particle_count_;
};

inline unsigned int mf_ParticlesManager::GetParticlesCount() const
{
	return particle_count_;
}
