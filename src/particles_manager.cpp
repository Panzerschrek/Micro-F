#include "micro-f.h"
#include "particles_manager.h"
#include "aircraft.h"

#define MF_SMOKE_MAX_LIFETIME 7.0f

mf_ParticlesManager::mf_ParticlesManager()
	: particle_count_(0)
	, prev_tick_time_(0.0f), current_tick_time_(0.0000001f), dt_(current_tick_time_ - prev_tick_time_)
	, spawn_counter_(0)
{
}

mf_ParticlesManager::~mf_ParticlesManager()
{
}

void mf_ParticlesManager::Tick( float dt )
{
	prev_tick_time_= current_tick_time_;
	current_tick_time_+= dt;
	dt_= current_tick_time_ - prev_tick_time_;

	Particle* particle= particles_;
	for( unsigned int i= 0; i< particle_count_; )
	{
		float d_pos[3];
		Vec3Mul( particle->direction,
			dt_ * ( particle->velocity + 0.5f * dt_ * particle->acceleration ),
			d_pos );
		Vec3Add( particle->pos, d_pos );
		particle->velocity+= particle->acceleration * dt_;
		if( particle->velocity < 0.0f ) particle->velocity= 0.0f;

		if( current_tick_time_ -  particle->spawn_time >= particle->life_time )
		{
			if( i != particle_count_ - 1 )
				*particle= particles_[ particle_count_ - 1 ];
			particle_count_--;
			continue;
		}
		/*switch( particles_[i].type )
		{
		case Particle::JetEngineFire:
			break;
		case Particle::Smoke:
			break;
		default:
			break;
		};*/
		i++;
		particle++;
	}
}

void mf_ParticlesManager::AddEnginesTrail( const mf_Aircraft* aircraft )
{
	const float c_smoke_particles_per_second= 60.0f;
	float particles_per_second= ( aircraft->Throttle() + 1.0f ) * 0.5f * c_smoke_particles_per_second;

	unsigned int smoke_particle_count= (unsigned int)
		( mf_Math::floor(current_tick_time_ * particles_per_second) - mf_Math::ceil(prev_tick_time_ * particles_per_second) )
		+ 1u;
	float aircraft_velocity= Vec3Len( aircraft->Velocity() );

	static const float c_lifetime[8]=
	{
		1.0f, 2.5f, 3.0f, 3.5f, 4.0f, 5.0f, 6.0f, MF_SMOKE_MAX_LIFETIME
	};

	Particle* particle= particles_ + particle_count_;
	float partice_pos[3];
	float particle_step[3];
	float particle_dir[3];
	float particle_velocity;
	float particle_acceleration;
	VEC3_CPY( partice_pos, aircraft->Pos() );
	Vec3Mul( aircraft->Velocity(), -dt_, particle_step );
	Vec3Mul( aircraft->Velocity(), - 1.0f / aircraft_velocity, particle_dir );
	particle_velocity= aircraft_velocity * 0.25f;
	particle_acceleration= -aircraft_velocity * aircraft_velocity * 0.125f;
	for( unsigned int i= 0; i< smoke_particle_count; i++, Vec3Add( partice_pos, particle_step ), particle++ )
	{
		particle->type= Particle::JetEngineTrail;
		VEC3_CPY( particle->pos, partice_pos );
		VEC3_CPY( particle->direction, particle_dir );
		particle->velocity= particle_velocity;
		particle->acceleration= particle_acceleration;
		particle->spawn_time= current_tick_time_;
		particle->life_time= c_lifetime[ spawn_counter_ % (sizeof(c_lifetime) / sizeof(float)) ];
		spawn_counter_++;
	}
	particle_count_+= smoke_particle_count;
}

void mf_ParticlesManager::PrepareParticlesVertices( mf_ParticleVertex* out_vertices ) const
{
	const Particle* particle= particles_;
	mf_ParticleVertex* vertex= out_vertices;
	for( unsigned int i= 0; i< particle_count_; i++, particle++, vertex++ )
	{
		VEC3_CPY( vertex->pos_size, particle->pos );
		switch( particle->type )
		{
		case Particle::JetEngineTrail:
			{
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MF_SMOKE_MAX_LIFETIME );
				vertex->pos_size[3]= 0.5f + 10.0f * lifetime_k;

				vertex->luminance= ( 1.0f - lifetime_k * 32.0f );
				if( vertex->luminance < 0.0f ) vertex->luminance= 0.0f;

				float alpha= ( 1.0f - lifetime_k );
				alpha*= alpha * 0.25f;
				vertex->transparency_multipler__backgound_multipler__texture_id__reserved[0]= 0;
				vertex->transparency_multipler__backgound_multipler__texture_id__reserved[1]=
					(unsigned char)( alpha * 254.0f );
				vertex->transparency_multipler__backgound_multipler__texture_id__reserved[2]= 0;
			}
			break;
		default: break;
		} // switch type
	}
}