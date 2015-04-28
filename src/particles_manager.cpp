#include "micro-f.h"
#include "particles_manager.h"
#include "textures_generation.h"
#include "aircraft.h"
#include "game_logic.h"

#define MF_SMOKE_MAX_LIFETIME 7.0f
#define MF_PLASMA_ENGINE_PARTICLE_LIFETIME 0.5f
#define MF_PLASMABALL_TRAIL_PARTICLE_LIFETIME 0.6f
#define MF_BLAST_FIRE_LIFETIME 1.0f

static void TransformAircraftPoint( const mf_Aircraft* aircraft, const float* in_pos, float* out_pos )
{
	float mat[16];
	Mat4Identity( mat );
	VEC3_CPY( mat+0, aircraft->AxisVec(0) );
	VEC3_CPY( mat+4, aircraft->AxisVec(1) );
	VEC3_CPY( mat+8, aircraft->AxisVec(2) );

	Vec3Mat4Mul( in_pos, mat, out_pos );
	Vec3Add( out_pos, aircraft->Pos() );
}

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

void mf_ParticlesManager::AddBulletTerrainHit( const float* pos )
{
	const unsigned int c_particles_count= 32;

	Particle* particle= particles_ + particle_count_;
	for( unsigned int i= 0; i< c_particles_count; i++, particle++ )
	{
		VEC3_CPY( particle->pos, pos );
		for( unsigned int j= 0; j< 3; j++ )
			particle->direction[j]= randomizer_.RandF( -1.0f, 1.0f );
		Vec3Normalize( particle->direction );
		particle->velocity= 20.0f;
		particle->acceleration= 0.0f;
		particle->type= Particle::JetEngineTrail;
		particle->spawn_time= current_tick_time_;
		particle->life_time= MF_SMOKE_MAX_LIFETIME * 0.5f;
	}
	particle_count_+= c_particles_count;
}

void mf_ParticlesManager::AddBlast( const float* pos )
{
	const unsigned int c_particles_count= 80;

	Particle* particle= particles_ + particle_count_;
	for( unsigned int i= 0; i< c_particles_count; i++, particle++ )
	{
		VEC3_CPY( particle->pos, pos );
		for( unsigned int j= 0; j< 3; j++ )
			particle->direction[j]= randomizer_.RandF( -1.0f, 1.0f );
		Vec3Normalize( particle->direction );
		particle->velocity= 40.0f;
		particle->acceleration= -30.0f;
		particle->type= Particle::BlastFire;
		particle->spawn_time= current_tick_time_;
		particle->life_time= MF_BLAST_FIRE_LIFETIME;
	}
	particle_count_+= c_particles_count;
}

void mf_ParticlesManager::AddPowerupGlow( const float* pos, float glow_factor )
{
	Particle* particle= particles_ + particle_count_;

	VEC3_CPY( particle->pos, pos );
	particle->velocity= 0.0f;
	particle->acceleration= 0.0f;
	particle->type= Particle::PowerupGlow;
	particle->spawn_time= current_tick_time_;
	particle->life_time= -glow_factor; // hack, write it to lifetime

	particle_count_++;
}

void mf_ParticlesManager::AddPlasmaBall( const float* pos )
{
	Particle* particle= particles_ + particle_count_;

	VEC3_CPY( particle->pos, pos );
	particle->velocity= 0.0f;
	particle->acceleration= 0.0f;
	particle->type= Particle::PlasmaBall;
	particle->spawn_time= current_tick_time_;
	particle->life_time= 0.0f;

	particle_count_++;
}

void mf_ParticlesManager::AddRocketTrail( const mf_Rocket* rocket )
{
	const float c_particles_per_meter= 0.3f;

	float particles_per_second= c_particles_per_meter * rocket->velocity;

	unsigned int particle_count= (unsigned int)
		( mf_Math::floor(current_tick_time_ * particles_per_second) - mf_Math::ceil(prev_tick_time_ * particles_per_second) )
		+ 1u;

	Particle* particle= particles_ + particle_count_;
	float partice_pos[3];
	float particle_step[3];
	float particle_dir[3];
	VEC3_CPY( partice_pos, rocket->pos );
	Vec3Mul( rocket->dir, -1.0f, particle_dir );
	Vec3Mul( particle_dir, 1.0f / c_particles_per_meter, particle_step );
	float dt= (1.0f / c_particles_per_meter) / rocket->velocity;
	float t= current_tick_time_;

	float unused;
	float part= modf(current_tick_time_ * particles_per_second, &unused );
	t-= dt * part;
	float pos_add_vec[3];
	Vec3Mul( particle_step, part, pos_add_vec );
	Vec3Add( partice_pos, pos_add_vec );

	for( unsigned int i= 0; i< particle_count; i++, Vec3Add( partice_pos, particle_step ), t-= dt, particle++ )
	{
		particle->type= Particle::PlasmaBallTrail;
		VEC3_CPY( particle->pos, partice_pos );
		VEC3_CPY( particle->direction, particle_dir );
		particle->velocity= 0.0f;
		particle->acceleration= 0.0f;
		particle->spawn_time= t;
		particle->life_time= MF_PLASMABALL_TRAIL_PARTICLE_LIFETIME;
		spawn_counter_++;
	}
	particle_count_+= particle_count;
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
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MF_PLASMA_ENGINE_PARTICLE_LIFETIME );
				vertex->pos_size[3]= 0.25f;

				vertex->luminance= 1.0f - lifetime_k;
				vertex->luminance*= vertex->luminance;

				vertex->t_dt_lt_r[0]= 0;
				vertex->t_dt_lt_r[1]= 0;
				vertex->t_dt_lt_r[2]= TextureEnginePlasma;
			}
			break;
		case Particle::PowerupGlow:
			{
				vertex->pos_size[3]= 8.0f;
				vertex->luminance= -particle->life_time;
				vertex->t_dt_lt_r[0]= 0;
				vertex->t_dt_lt_r[1]= 0;
				vertex->t_dt_lt_r[2]= TexturePowerupGlow;
			}
			break;
		case Particle::PlasmaBall:
			{
				vertex->pos_size[3]= 1.8f;
				vertex->luminance= 1.0f;
				vertex->t_dt_lt_r[0]= 0;
				vertex->t_dt_lt_r[1]= 0;
				vertex->t_dt_lt_r[2]= TexturePlasmaBall;
			}
			break;
		case Particle::PlasmaBallTrail:
			{
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MF_PLASMABALL_TRAIL_PARTICLE_LIFETIME );
				vertex->pos_size[3]= 1.1f - 0.6f * lifetime_k;
				vertex->luminance= 1.0f + lifetime_k * 1.0f;
				vertex->t_dt_lt_r[0]= 0;
				vertex->t_dt_lt_r[1]= 0;
				vertex->t_dt_lt_r[2]= TexturePlasmaBall;
			}
			break;
		case Particle::BlastFire:
			{
				float lifetime_k= ( current_tick_time_ - particle->spawn_time ) * ( 1.0f / MF_BLAST_FIRE_LIFETIME );
				vertex->pos_size[3]= 1.0f + 9.0f * lifetime_k;

				vertex->luminance= 0.5f * ( 1.0f - 1.5f * lifetime_k );
				if( vertex->luminance < 0.0f ) vertex->luminance= 0.0f;

				float alpha= 0.33f * ( 1.0f - lifetime_k );
				vertex->t_dt_lt_r[0]= (unsigned char)( alpha * 254.0f );
				vertex->t_dt_lt_r[1]= TextureEngineSmoke;
				vertex->t_dt_lt_r[2]= TextureFire;
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
	TransformAircraftPoint( aircraft, c_engines_pos + engine_number*3, partice_pos );
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
		particle->life_time= MF_PLASMA_ENGINE_PARTICLE_LIFETIME;
		spawn_counter_++;
	}
	particle_count_+= particle_count;
}

void mf_ParticlesManager::AddV1Trail( const mf_Aircraft* aircraft )
{
	static const float c_v1_engine_pos[3]= { 0.0f, -4.7f, 0.9f };

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
	float particle_pos[3];
	float particle_step[3];
	float particle_dir[3];
	float particle_velocity;
	float particle_acceleration;
	TransformAircraftPoint( aircraft, c_v1_engine_pos, particle_pos );
	Vec3Mul( aircraft->Velocity(), -dt_ , particle_step );
	Vec3Mul( aircraft->Velocity(), - 1.0f / aircraft_velocity, particle_dir );
	particle_velocity= aircraft_velocity * 0.25f;
	particle_acceleration= -aircraft_velocity * aircraft_velocity * 0.125f;
	for( unsigned int i= 0; i< smoke_particle_count; i++, Vec3Add( particle_pos, particle_step ), particle++ )
	{
		particle->type= Particle::JetEngineTrail;
		VEC3_CPY( particle->pos, particle_pos );
		VEC3_CPY( particle->direction, particle_dir );
		particle->velocity= particle_velocity;
		particle->acceleration= particle_acceleration;
		particle->spawn_time= current_tick_time_;
		particle->life_time= c_lifetime[ spawn_counter_ % (sizeof(c_lifetime) / sizeof(float)) ];
		spawn_counter_++;
	}
	particle_count_+= smoke_particle_count;
}