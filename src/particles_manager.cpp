#include "micro-f.h"
#include "particles_manager.h"
#include "textures_generation.h"
#include "aircraft.h"

#define MF_SMOKE_MAX_LIFETIME 7.0f
#define MF_PLASMA_ENGINE_PARTICEL_LIFETIME 0.5f

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

		if( current_tick_time_ - particle->spawn_time >= particle->life_time )
		{
			particle_count_--;
			if( i == particle_count_ )
				break;
			*particle= particles_[ particle_count_ ];
		}
		else
		{
			i++;
			particle++;
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
	}
}

void mf_ParticlesManager::AddEnginesTrail( const mf_Aircraft* aircraft )
{
	if( aircraft->GetType() == mf_Aircraft::F2XXX )
	{
		AddF2XXXTrail( aircraft, 0 );
		AddF2XXXTrail( aircraft, 1 );
	}
	else
		AddV1Trail( aircraft );
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
				vertex->t_dt_lt_r[0]= (unsigned char)( alpha * 254.0f );
				vertex->t_dt_lt_r[1]= TextureEngineSmoke;
				vertex->t_dt_lt_r[2]= TextureEngineFire;
			}
			break;
		case Particle::PlasmaEngineTrail:
			{
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MF_PLASMA_ENGINE_PARTICEL_LIFETIME );
				vertex->pos_size[3]= 0.25f;

				vertex->luminance= 1.0f - lifetime_k;
				vertex->luminance*= vertex->luminance;

				vertex->t_dt_lt_r[0]= 0;
				vertex->t_dt_lt_r[1]= 0;
				vertex->t_dt_lt_r[2]= TextureEnginePlasma;
			}
			break;
		default: break;
		} // switch type
	}
}


void mf_ParticlesManager::AddF2XXXTrail( const mf_Aircraft* aircraft, unsigned int engine_number )
{
	static const float c_engines_pos[2*3]=
	{
		 1.5f, -2.3f, 0.0f,
		-1.5f, -2.3f, 0.0f
	};

	float trail_begin_pos[3];
	{
		float mat[16];
		Mat4Identity( mat );
		VEC3_CPY( mat+0, aircraft->AxisVec(0) );
		VEC3_CPY( mat+4, aircraft->AxisVec(1) );
		VEC3_CPY( mat+8, aircraft->AxisVec(2) );

		Vec3Mat4Mul( c_engines_pos + engine_number*3, mat, trail_begin_pos );
		Vec3Add( trail_begin_pos, aircraft->Pos() );
	}

	const float c_particles_per_meter= 8.0f;

	float aircraft_velocity= Vec3Len( aircraft->Velocity() );
	float particles_per_second= c_particles_per_meter * aircraft_velocity;

	unsigned int particle_count= (unsigned int)
		( mf_Math::floor(current_tick_time_ * particles_per_second) - mf_Math::ceil(prev_tick_time_ * particles_per_second) )
		+ 1u;

	Particle* particle= particles_ + particle_count_;
	float partice_pos[3];
	float particle_step[3];
	float particle_dir[3];
	VEC3_CPY( partice_pos, trail_begin_pos );
	Vec3Mul( aircraft->Velocity(), - 1.0f / aircraft_velocity, particle_dir );
	Vec3Mul( particle_dir, 1.0f / c_particles_per_meter, particle_step );
	float dt= (1.0f / c_particles_per_meter) / aircraft_velocity;
	float t= current_tick_time_;

	float unused;
	float part= modf(current_tick_time_ * particles_per_second, &unused );
	t-= dt * part;
	float pos_add_vec[3];
	Vec3Mul( particle_step, part, pos_add_vec );
	Vec3Add( partice_pos, pos_add_vec );

	for( unsigned int i= 0; i< particle_count; i++, Vec3Add( partice_pos, particle_step ), t-= dt, particle++ )
	{
		particle->type= Particle::PlasmaEngineTrail;
		VEC3_CPY( particle->pos, partice_pos );
		VEC3_CPY( particle->direction, particle_dir );
		particle->velocity= 0.0f;
		particle->acceleration= 0.0f;
		particle->spawn_time= t;
		particle->life_time= MF_PLASMA_ENGINE_PARTICEL_LIFETIME;
		spawn_counter_++;
	}
	particle_count_+= particle_count;
}

void mf_ParticlesManager::AddV1Trail( const mf_Aircraft* aircraft )
{
	const float c_smoke_particles_per_second= 60.0f;
	float particles_per_second= ( aircraft->Throttle() + 1.0f ) * 0.5f * c_smoke_particles_per_second;

	float aircraft_velocity= Vec3Len( aircraft->Velocity() );

	unsigned int smoke_particle_count= (unsigned int)
		( mf_Math::floor(current_tick_time_ * particles_per_second) - mf_Math::ceil(prev_tick_time_ * particles_per_second) )
		+ 1u;

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