#pragma once
#include "micro-f.h"
#include "mf_math.h"

class mf_Aircraft;
struct mf_Rocket;

/*
 Input - texture with premultipled alpha.
 Blending mode: GL_ONE, GL_ONE_MINUS_SRC_ALPHA
 color.xyz=
	albedo_tex.xyz * sun_light * albedo_tex.a * ( 1.0 - transparency ) ) +
	brightness_tex.xyz * luminance
 color.a= transparency * albedo_tex.a
*/
#pragma pack(push, 1)
struct mf_ParticleVertex
{
	float pos_size[4];
	float luminance;

	//! 0 - transparency
	//! 1 - diffuse texture id
	//! 2 - luminance texture id
	//! 3 - reserved
	unsigned char t_dt_lt_r[4];

};
#pragma pack(pop)

#define MF_MAX_PARTICLES 8192

class mf_ParticlesManager
{
public:
	mf_ParticlesManager();
	~mf_ParticlesManager();

	void Tick( float dt );
	void AddEnginesTrail( const mf_Aircraft* aircraft );
	void AddBulletTerrainHit( const float* pos );
	void AddBlast( const float* pos );
	void AddPowerupGlow( const float* pos, float glow_factor /* in range 0 - 1*/ );
	void AddPlasmaBall( const float* pos );
	void AddRocketTrail( const mf_Rocket* bullet );

	unsigned int GetParticlesCount() const;
	void PrepareParticlesVertices( mf_ParticleVertex* out_vertices ) const;
	void KillAllParticles();

private:

	void AddF2XXXTrail( const mf_Aircraft* aircraft, unsigned int engine_number );
	void AddV1Trail( const mf_Aircraft* rocket, const float* pos );
	struct Particle
	{
		enum Type
		{
			JetEngineTrail,
			PlasmaEngineTrail,
			PowerupGlow,
			PlasmaBall,
			PlasmaBallTrail,
			BlastFire,
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
