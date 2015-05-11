#include "micro-f.h"
#include "level.h"

#include "mf_math.h"
#include "textures_generation.h"
#include "main_loop.h"

#define MF_MAX_VALLEY_WAY_POINTS 1024

static unsigned short Noise2( int x,  int y, int seed )
{
	/*int n= x + y * 57;
	n= (n << 13) ^ n;
	return (unsigned short)(
		((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /0x7fff
		);*/
	const int X_NOISE_GEN = 1;
	const int Y_NOISE_GEN = 31337;
	const int Z_NOISE_GEN = 263;
	const int SEED_NOISE_GEN = 1013;
	int n= (
		X_NOISE_GEN      * x
		+ Y_NOISE_GEN    * y
		+ Z_NOISE_GEN    * 0
		+ SEED_NOISE_GEN * seed)
		& 0x7fffffff;
		n = (n >> 13) ^ n;
	return (unsigned short)( (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff );
}

static unsigned short InterpolatedNoise( unsigned int x, unsigned int y, unsigned int k, int seed )
{
	unsigned int step= 1<<k;
	unsigned int X= x >> k;
	unsigned int Y= y >> k;

	unsigned int noise[4]=
	{
		Noise2(X, Y, seed ),
		Noise2(X + 1, Y, seed ),
		Noise2(X + 1, Y + 1, seed ),
		Noise2(X, Y + 1, seed )
	};

	unsigned int dx=  x - (X <<k );
	unsigned int dy=  y - (Y <<k );
	unsigned int interp_x[2]=
	{
		dy * noise[3] + (step - dy) * noise[0],
		dy * noise[2] + (step - dy) * noise[1]
	};
	return (unsigned short)(
		(interp_x[1] * dx + interp_x[0] * (step - dx) )>>(k+k)
		);
}

static unsigned short FinalNoise( unsigned int x, unsigned int y, int seed )
{
	unsigned short r= 0;
	for( int i= 0; i<= 6; i++ )
		r+= InterpolatedNoise( x, y, 7 - i, seed )>>(1+i);
	return r;
}

mf_Level::mf_Level( unsigned int seed )
	: seed_(seed)
{
	terrain_size_[0]= 1280;
	terrain_size_[1]= 8192;
	terrain_amplitude_= 144.0f * 1.3f;
	terrain_cell_size_= 2.0f;
	terrain_water_level_= terrain_amplitude_ / 9.0f;

	forcefield_radius_= terrain_amplitude_ * 4.0f;
	forcefield_z_pos_= - terrain_amplitude_ * 2.0f;

	terrain_heightmap_data_= new unsigned short[ terrain_size_[0] * terrain_size_[1] ];
	terrain_normal_textures_map_= new char[ terrain_size_[0] * terrain_size_[1] * 4 ];

	valley_y_params_= new ValleyYparams[ terrain_size_[1] ];

	valley_way_points_= new ValleyWayPoint[ MF_MAX_VALLEY_WAY_POINTS ];
	valley_way_point_count_= 0;

	static_objects_row_count_= terrain_size_[1] / MF_STATIC_OBJECTS_ROW_SIZE_CL;
	static_objects_rows_= new mf_StaticLevelObjectsRow[ static_objects_row_count_ ];
	/*for( unsigned int i= 0; i< static_objects_row_count_; i++ )
	{
		static_objects_rows_[i].objects_count= 16;
		static_objects_rows_[i].objects= new mf_StaticLevelObject[ static_objects_rows_[i].objects_count ];
	}*/

	GenTarrain();
	PlaceStaticObjects();
}

mf_Level::~mf_Level()
{
	delete[] terrain_heightmap_data_;
	delete[] terrain_normal_textures_map_;
	delete[] valley_y_params_;
	delete[] valley_way_points_;

	//for( unsigned int i= 0; i< static_objects_row_count_; i++ )
	//	delete[] static_objects_rows_[i].objects;
	delete[] static_objects_rows_;
}


float mf_Level::GetTerrainHeight( float x, float y ) const
{
	x/= terrain_cell_size_;
	y/= terrain_cell_size_;

	int x_i= int(x);
	int y_i= int(y);
	if( x_i < 0 ) x_i= 0; else if( x_i >= int(terrain_size_[0]-1) ) x_i= terrain_size_[0]-2;
	if( y_i < 0 ) y_i= 0; else if( y_i >= int(terrain_size_[1]-1) ) y_i= terrain_size_[1]-2;

	float dx= x - float(x_i);
	float dy= y - float(y_i);

	float values[4]=
	{
		float(terrain_heightmap_data_[ x_i   +  y_i    * terrain_size_[0] ]) / 65535.0f,
		float(terrain_heightmap_data_[ x_i+1 +  y_i    * terrain_size_[0] ]) / 65535.0f,
		float(terrain_heightmap_data_[ x_i   + (y_i+1) * terrain_size_[0] ]) / 65535.0f,
		float(terrain_heightmap_data_[ x_i+1 + (y_i+1) * terrain_size_[0] ]) / 65535.0f
	};
	float y_interp[2];
	y_interp[0]= values[0] * (1.0f-dy) + dy * values[2];
	y_interp[1]= values[1] * (1.0f-dy) + dy * values[3];

	return terrain_amplitude_ *
		( y_interp[0] * (1.0f-dx) + y_interp[1] * dx );
}

float mf_Level::GetValleyCenterX( float y ) const
{
	y/= terrain_cell_size_;
	return float(valley_y_params_[ int(y) ].x_center ) * terrain_cell_size_;
}

bool mf_Level::SphereIntersectTerrain( const float* pos, float radius ) const
{
	int pos_i[3];
	for( unsigned int i= 0; i< 3; i++ )
		pos_i[i]= int( pos[i] / terrain_cell_size_ );
	int i_radius= int( mf_Math::ceil( radius / terrain_cell_size_ ) );
	float radius2= radius * radius;
	
	int xy_begin[2], xy_end[2];
	for( unsigned int i= 0; i< 2; i++ )
	{
		xy_begin[i]= pos_i[i] - i_radius;
		if( xy_begin[i] < 0 ) xy_begin[i]= 0;
		xy_end[i]= pos_i[i] + i_radius;
		if( xy_end[i] >= int(terrain_size_[i]) ) xy_end[i]= terrain_size_[i] - 1;
	}
	for( int y= xy_begin[1]; y<= xy_end[1]; y++ )
	{
		float dy2= float(y) * terrain_cell_size_ - pos[1];
		dy2= dy2 * dy2;
		for( int x= xy_begin[0]; x<= xy_end[0]; x++ )
		{
			float dx= float(x) * terrain_cell_size_ - pos[0];
			float dz2= radius2 - dx*dx - dy2;
			if( dz2 <= 0.0f ) continue;
			float terrain_height= float( terrain_heightmap_data_[ x + y * terrain_size_[0] ] ) * terrain_amplitude_ / 65535.0f;
			float sphere_low_pos= pos[2] - mf_Math::sqrt(dz2);
			if( sphere_low_pos < terrain_height ) return true;
		}
	}

	return false;
}

bool mf_Level::BeamIntersectTerrain( const float* in_pos, const float* dir, float max_distance, bool need_accuracy, float* out_pos_opt )
{
	float pos[3];
	float step[3];
	VEC3_CPY( pos, in_pos );
	Vec3Normalize( dir, step );
	Vec3Mul( step, 1.0f / terrain_cell_size_ );

	bool is_intersect= false;

	float xy_max[2]=
	{
		float(terrain_size_[0]) * terrain_cell_size_,
		float(terrain_size_[1]) * terrain_cell_size_,
	};
	float dist= 0.0f;
	while(pos[2] <= terrain_amplitude_ && pos[2] >= 0 && dist <= max_distance )
	{
		if( pos[0] < 0.0f || pos[1] < 0.0f || pos[0] > xy_max[0] || pos[1] > xy_max[1] )
			break;
		if( GetTerrainHeight( pos[0], pos[1] ) > pos[2] )
		{
			is_intersect= true;
			break;
		}
		Vec3Add( pos, step );
		dist+= 1.0f; // length of step is 1
	}

	//TODO - make hight precisionresult
	(void)need_accuracy;

	if( is_intersect && out_pos_opt != NULL )
	{
		VEC3_CPY( out_pos_opt, pos );
	}
	return is_intersect;
}

void mf_Level::GenTarrain()
{
	mf_MainLoop::Instance()->DrawLoadingFrame( "generating terrain" );

	unsigned short* primary_terrain_data= new unsigned short[ terrain_size_[0] * terrain_size_[1] ];

	// first gen
	for( unsigned int y= 0; y< terrain_size_[1]; y++ )
		for( unsigned int x= 0; x< terrain_size_[0]; x++ )
		{
			unsigned int noise= FinalNoise( x,y, seed_ );
			noise= ( noise * noise ) >> 16;
			primary_terrain_data[ x + y * terrain_size_[0] ]= (unsigned short)noise;
		}

	// Make terrain smooth.
	for( unsigned int y= 1; y< terrain_size_[1] - 1; y++ )
		for( unsigned int x= 1; x< terrain_size_[0] - 1; x++ )
		{
			unsigned int r;
			r=  primary_terrain_data[  x     + y * terrain_size_[0] ]<<2;
			r+= primary_terrain_data[ (x+1)  + y * terrain_size_[0] ]<<1;
			r+= primary_terrain_data[ (x-1)  + y * terrain_size_[0] ]<<1;
			r+= primary_terrain_data[  x + (y+1) * terrain_size_[0] ]<<1;
			r+= primary_terrain_data[  x + (y-1) * terrain_size_[0] ]<<1;

			r+= primary_terrain_data[ (x-1) + (y-1) * terrain_size_[0] ];
			r+= primary_terrain_data[ (x-1) + (y+1) * terrain_size_[0] ];
			r+= primary_terrain_data[ (x+1) + (y-1) * terrain_size_[0] ];
			r+= primary_terrain_data[ (x+1) + (y+1) * terrain_size_[0] ];

			terrain_heightmap_data_[ x + y * terrain_size_[0] ]= (unsigned short)(r>>4);
		}
	delete[] primary_terrain_data;

	GenValleyWayPoints();

	// Gen Normals
	float terr_k= terrain_amplitude_ / float(0xFFFF);
	for( unsigned int y= 1; y< terrain_size_[1] - 1; y++ )
		for( unsigned int x= 1; x< terrain_size_[0] - 1; x++ )
		{
			float val[9];
			for( unsigned int j= 0; j< 3; j++ )
			{
				val[j*3  ]= float(terrain_heightmap_data_[ (x-1) + (y-1+j) * terrain_size_[0] ]) * terr_k;
				val[j*3+1]= float(terrain_heightmap_data_[  x    + (y-1+j) * terrain_size_[0] ]) * terr_k;
				val[j*3+2]= float(terrain_heightmap_data_[ (x+1) + (y-1+j) * terrain_size_[0] ]) * terr_k;
			}
			float normal[]= 
			{
				-( val[2] + val[5] + val[8] - val[0] - val[3] - val[6] ) * ( 0.3333333f * 0.5f ),
				-( val[6] + val[7] + val[8] - val[0] - val[1] - val[2] ) * ( 0.3333333f * 0.5f ),
				terrain_cell_size_
			};
			/*float normal[]=
			{
				-( val[5] - val[3] ) * 0.5f,
				-( val[7] - val[2] ) * 0.5f,
				terrain_ceil_size_
			};*/

			Vec3Normalize(normal);
			for( unsigned int j= 0; j< 3; j++ )
				terrain_normal_textures_map_[ (x + y * terrain_size_[0]) * 4 + j ]= (char)( 126.9f * normal[j] );
		}// for xy

	// set borderes
	for( unsigned int x= 1; x < terrain_size_[0] - 1; x++ )
	{
		unsigned int i0dst= x;
		unsigned int i0src= x + terrain_size_[0];
		unsigned int i1dst= x + (terrain_size_[1]-1) * terrain_size_[0];
		unsigned int i1src= x + (terrain_size_[1]-2) * terrain_size_[0];
		terrain_heightmap_data_[ i0dst ]= terrain_heightmap_data_[ i0src ];
		terrain_heightmap_data_[ i1dst ]= terrain_heightmap_data_[ i1src ];

		for( unsigned int j= 0; j< 4; j++ )
		{
			terrain_normal_textures_map_[ i0dst * 4 + j ]= terrain_normal_textures_map_[ i0src * 4 + j ];
			terrain_normal_textures_map_[ i1dst * 4 + j ]= terrain_normal_textures_map_[ i1src * 4 + j ];
		}
	} // for x
	for( unsigned int y= 1; y < terrain_size_[1] - 1; y++ )
	{
		unsigned int i0dst= y * terrain_size_[0];
		unsigned int i0src= 1 + y * terrain_size_[0];
		unsigned int i1dst= terrain_size_[0] - 2 + y * terrain_size_[0];
		unsigned int i1src= terrain_size_[0] - 1 + y * terrain_size_[0];
		terrain_heightmap_data_[ i0dst ]= terrain_heightmap_data_[ i0src ];
		terrain_heightmap_data_[ i1dst ]= terrain_heightmap_data_[ i1src ];

		for( unsigned int j= 0; j< 4; j++ )
		{
			terrain_normal_textures_map_[ i0dst * 4 + j ]= terrain_normal_textures_map_[ i0src * 4 + j ];
			terrain_normal_textures_map_[ i1dst * 4 + j ]= terrain_normal_textures_map_[ i1src * 4 + j ];
		}
	} // for y

	// set corners
	terrain_heightmap_data_[0]= terrain_heightmap_data_[ 1 + terrain_size_[0] ];
	terrain_heightmap_data_[ terrain_size_[0] - 1 ]= terrain_heightmap_data_[ terrain_size_[0] - 2 ];
	for( unsigned int j= 0; j< 4; j++ )
	{
		terrain_normal_textures_map_[j]=
			terrain_normal_textures_map_[ (1 + terrain_size_[0]) * 4 + j ];
		terrain_normal_textures_map_[ (terrain_size_[0] - 1) * 4 + j ]=
			terrain_normal_textures_map_[ (terrain_size_[0] - 2) * 4 + j ];
	}
	unsigned int i0dst= ( terrain_size_[1] - 1 ) * terrain_size_[0];
	unsigned int i0src= 1 + ( terrain_size_[1] - 2 ) * terrain_size_[0] ;
	unsigned int i1dst= terrain_size_[0] - 1 + ( terrain_size_[1] - 1 ) * terrain_size_[0];
	unsigned int i1src= terrain_size_[0] - 2 + ( terrain_size_[1] - 2 ) * terrain_size_[0];
	terrain_heightmap_data_[ i0dst ]= terrain_heightmap_data_[ i0src ];
	terrain_heightmap_data_[ i1dst ]= terrain_heightmap_data_[ i1src ];
	for( unsigned int j= 0; j< 4; j++ )
	{
		terrain_normal_textures_map_[ i0dst * 4 + j ]= terrain_normal_textures_map_[ i0src * 4 + j ];
		terrain_normal_textures_map_[ i1dst * 4 + j ]= terrain_normal_textures_map_[ i1src * 4 + j ];
	}

	PlaceTextures();
}

void mf_Level::GenValleyWayPoints()
{
	mf_MainLoop::Instance()->DrawLoadingFrame( "generating river" );
	mf_Rand randomizer( seed_ );

	const float y_range[]= {96.0f * 2.0f, 128.0f * 2.5f };
	const float x_amplitude= 144.0f;

	int y= -512;

	float x_center= float(terrain_size_[0]/2);
	while( y < int(terrain_size_[1]) + 512 )
	{
		float x= x_center + randomizer.RandF( -x_amplitude, x_amplitude );
		float dy= randomizer.RandF( y_range[0], y_range[1] );

		y+= int(dy);
		if( y >= int(terrain_size_[1]) + 512 ) break;

		valley_way_points_[ valley_way_point_count_ ].x= int(x);
		valley_way_points_[ valley_way_point_count_ ].y= y;
		valley_way_points_[ valley_way_point_count_ ].h= randomizer.RandI( 0,0xFFFF/12 );
		valley_way_point_count_++;
	}

	for( unsigned int y= 0; y< terrain_size_[1]; y++ )
		valley_y_params_[y].x_center= (unsigned short)( terrain_size_[0]/2 );

	for( unsigned int i= 1; i< valley_way_point_count_ - 3; i++ )
	{
		// search cubic spline for smoothing x(y) river function
		double abcd[4];
		{
			double xyzw[4];
			double mat[16];
			double invert_mat[16];

			mat[12]= 1.0;
			mat[8]= double(valley_way_points_[i].y); // y0
			mat[4]= mat[8] * mat[8]; // y0^2
			mat[0]= mat[4] * mat[8]; // y0^3

			mat[13]= 1.0;
			mat[9]= double(valley_way_points_[i+1].y); // y1
			mat[5]= mat[9] * mat[9]; // y1^2
			mat[1]= mat[5] * mat[9]; // y1^3

			double der0= double( valley_way_points_[i].x - valley_way_points_[i-1].x ) /
				double( valley_way_points_[i].y - valley_way_points_[i-1].y );

			double der1= double( valley_way_points_[i+1].x - valley_way_points_[i].x ) /
				float( valley_way_points_[i+1].y - valley_way_points_[i].y );

			double der2= double( valley_way_points_[i+2].x - valley_way_points_[i+1].x ) /
				float( valley_way_points_[i+2].y - valley_way_points_[i+1].y );

			xyzw[0]= double(valley_way_points_[i  ].x);
			xyzw[1]= double(valley_way_points_[i+1].x);
			xyzw[2]= 0.5 * ( der0 + der1 );
			xyzw[3]= 0.5 * ( der1 + der2 );

			mat[14]= 0.0;
			mat[10]= 1.0;
			mat[6 ]= 2.0 * double(valley_way_points_[i].y);
			mat[2 ]= double(valley_way_points_[i].y) * double(valley_way_points_[i].y) * 3.0;

			mat[15]= 0.0;
			mat[11]= 1.0;
			mat[7 ]= 2.0 * double(valley_way_points_[i+1].y);
			mat[3 ]= double(valley_way_points_[i+1].y) * double(valley_way_points_[i+1].y) * 3.0;

			DoubleMat4Invert( mat, invert_mat );
			for( unsigned int i= 0; i< 4; i++ )
			{
				abcd[i]=
					xyzw[0] * invert_mat[i  ] + xyzw[1] * invert_mat[i+4] +
					xyzw[2] * invert_mat[i+8] + xyzw[3] * invert_mat[i+12];
			}
		}

		int dy= valley_way_points_[i+1].y - valley_way_points_[i].y;
		int dh= valley_way_points_[i+1].h - valley_way_points_[i].h;
		double h_d= double(valley_way_points_[i].h);
		double dh_d= double(dh) / double(dy);

		y= valley_way_points_[i].y;
		if( y < 0 ) { y= 0; h_d+= dh_d * double(-y); }
		for( ; y< int(valley_way_points_[i+1].y) && y < int(terrain_size_[1]); y++, h_d+= dh_d )
		{
			double y_d= double(y) + 0.5;
			double x_center_d= (y_d * abcd[0] * y_d * y_d) + (y_d * abcd[1] * y_d) + (y_d * abcd[2]) + (abcd[3]);
			int x_center= int(x_center_d);
			valley_y_params_[y].x_center= (unsigned short) x_center;

			static const double init_width2= 48.0 * 2.0f;
			double der= 3.0 * y_d * y_d * abcd[0] + 2.0 * y_d * abcd[1] + abcd[2];
			double inv_cos_a= mf_Math::sqrt( 1.0f + float(der * der) );
			double width_d= init_width2 * inv_cos_a ;
			int width_2= int( width_d );

			for( int x= x_center - width_2; x< x_center + width_2; x++ )
			{
				int ind= x + y * terrain_size_[0];

				double dw= mf_Math::fabs( float( x_center_d - double(x) ) ) / width_d;
				double height_k= dw * dw * ( 3.0 - 2.0 * dw );
				double final_h= double(terrain_heightmap_data_[ ind ]) * height_k + (1.0 - height_k ) * h_d;
				terrain_heightmap_data_[ ind ]= (unsigned short) final_h;
			} // for x
		} // for y
	} // for segments of way
}

void mf_Level::PlaceTextures()
{
	unsigned short water_level_s= (unsigned short)( float(0xFFFF) * 1.05 * terrain_water_level_ / terrain_amplitude_ );
	char rock_normal_z= 100;

	unsigned int ind= 0;
	for( unsigned int y= 0; y< terrain_size_[1]; y++ )
		for( unsigned int x= 0; x< terrain_size_[0]; x++, ind++ )
		{
			if( terrain_heightmap_data_[ ind ] < water_level_s )
				terrain_normal_textures_map_[ ind * 4 + 3 ]= TextureSand;
			else if( terrain_normal_textures_map_[ ind * 4 + 2 ] < rock_normal_z )
				terrain_normal_textures_map_[ ind * 4 + 3 ]= TextureRock;
			else
				terrain_normal_textures_map_[ ind * 4 + 3 ]= TextureDirtWithGrass;

		}

	for( unsigned int i= 0; i< LastTerrainTexture; i++ )
	{
		for( unsigned int y= i*16; y< i*16 + 16; y++ )
			for( unsigned int x= 0; x< 16; x++ )
				terrain_normal_textures_map_[ (x + y * terrain_size_[0]) * 4 + 3 ]= (char)i;
	}
}

struct mf_GridCell
{
	mf_GridCell()
	{
		xy[0]= -1.0f;
	}

	bool Initialized() const { return xy[0] != -1.0f; }

	float xy[2];
	float point_radius;
	mf_StaticLevelObject::Type type;
};

void mf_Level::PlaceStaticObjects()
{
	mf_MainLoop::Instance()->DrawLoadingFrame( "planting forest" );
	mf_Rand randomizer( seed_ );

	const unsigned int grid_cell_size= 3;
	const float grid_cell_size_f= float(grid_cell_size);
	const unsigned int max_radius_scaler= 9;
	const float max_radius_scaler_f= float(max_radius_scaler);
	
	const unsigned int neighbor_k= 16;

	const float radius_eps= 0.01f;
	const float min_object_radius_cl= MF_SQRT_2 * 0.5f * ( 1.0f + radius_eps );

	unsigned int grid_size[2];
	for( unsigned int i= 0; i< 2; i++ )
	{
		grid_size[i]= terrain_size_[i] /grid_cell_size;
		if( grid_size[i] * grid_cell_size < terrain_size_[i] ) grid_size[i]++;
	}

	const unsigned int target_point_count= grid_size[0] * grid_size[1];
	unsigned int point_count= 0;

	mf_GridCell* grid= new mf_GridCell[ grid_size[0] * grid_size[1] ];
	mf_GridCell** processing_stack= new mf_GridCell*[ target_point_count ];
	mf_GridCell** final_points= new mf_GridCell*[ target_point_count ];

	float init_xy[2];
	init_xy[0]= randomizer.RandF( float(grid_size[0]/2) );
	init_xy[1]= randomizer.RandF( float(grid_size[1]/2) );
	processing_stack[0]= &grid[ int(init_xy[0]) + int(init_xy[1]) * grid_size[0] ];
	processing_stack[0]->xy[0]= init_xy[0];
	processing_stack[0]->xy[1]= init_xy[1];
	processing_stack[0]->point_radius= min_object_radius_cl;
	final_points[ point_count ]= processing_stack[0];
	point_count++;

	unsigned int processing_stack_pos= 1;

	while( point_count < target_point_count )
	{
		if( processing_stack_pos == 0 ) break;

		// pop point from processing stack
		const mf_GridCell* current_point= processing_stack[ processing_stack_pos - 1 ];
		MF_ASSERT( current_point->Initialized() );
		processing_stack_pos--;

		float current_point_terrain_space_xy[2];
		current_point_terrain_space_xy[0]= current_point->xy[0] * grid_cell_size_f;
		current_point_terrain_space_xy[1]= current_point->xy[1] * grid_cell_size_f;
		int current_point_terrain_space_xy_i[2];
		current_point_terrain_space_xy_i[0]= int(current_point_terrain_space_xy[0]);
		current_point_terrain_space_xy_i[1]= int(current_point_terrain_space_xy[1]);
		float radius_scaler= mf_Math::fabs(float(
			current_point_terrain_space_xy_i[0]
			- int(valley_y_params_[current_point_terrain_space_xy_i[1]].x_center)))
			/ float(terrain_size_[0]/2);
		radius_scaler= mf_Math::clamp( 0.0f, 1.0f, radius_scaler );
		radius_scaler= max_radius_scaler_f * radius_scaler + 1.0f * ( 1.0f - radius_scaler );
		if( radius_scaler > 6.0f ) radius_scaler= 9.0f;

		// try place neighbor points
		for( unsigned int i= 0; i< neighbor_k; i++ )
		{
			float point_radius= min_object_radius_cl * radius_scaler;

			float angle= randomizer.RandF( 0.0f, MF_2PI );
			float r= randomizer.RandF( point_radius * 2.0f, point_radius * 4.0f );
			float xy[2]=
			{
				mf_Math::sin(angle) * r + current_point->xy[0],
				mf_Math::cos(angle) * r + current_point->xy[1]
			};
			int xy_int[2]= { int(mf_Math::floor(xy[0])), int(mf_Math::floor(xy[1])) };
			bool is_outside_rect= false;
			for( unsigned int k= 0; k< 2; k++ )
			{
				if( xy_int[k] < 1 ) is_outside_rect= true;
				else if( xy_int[k] >= int(grid_size[k]) - 1 ) is_outside_rect= true;
			}
			if (is_outside_rect) continue;

			float terrain_space_xy[2];
			terrain_space_xy[0]= xy[0] * grid_cell_size_f;
			terrain_space_xy[1]= xy[1] * grid_cell_size_f;
			if( float(terrain_heightmap_data_[ int(terrain_space_xy[0]) + int(terrain_space_xy[1]) * terrain_size_[0] ])
				* terrain_amplitude_
				/ 65535.0f < terrain_water_level_ ) continue;

			int loop_coord_begin[2];
			int loop_coord_end[2];
			for( unsigned int i= 0; i< 2; i++ )
			{
				loop_coord_begin[i]= xy_int[i] - 2 * max_radius_scaler;
				if( loop_coord_begin[i] < 0 ) loop_coord_begin[i]= 0;
				loop_coord_end[i]= xy_int[i] + 2 * max_radius_scaler;
				if( loop_coord_end[i] >= int(grid_size[i]) ) loop_coord_end[i]= grid_size[i] - 1;
			}
			for( int y= loop_coord_begin[1]; y<= loop_coord_end[1]; y++ )
				for( int x= loop_coord_begin[0]; x<= loop_coord_end[0]; x++ )
				{
					const mf_GridCell* cell= &grid[ x + y * grid_size[0] ];
					if( cell->Initialized() )
					{
						float dx_dy[2]= { cell->xy[0] - xy[0], cell->xy[1] - xy[1] };
						float radius_sum= point_radius + cell->point_radius;
						if( dx_dy[0] * dx_dy[0] + dx_dy[1] * dx_dy[1] <= radius_sum * radius_sum )
							goto xy_loop_break;
					}
				}// for neighbor grid cells

			{
				mf_GridCell* cell= &grid[ xy_int[0] + xy_int[1] * grid_size[0] ];

				MF_ASSERT( !cell->Initialized() );
				MF_ASSERT( point_count < target_point_count );
				
				cell->xy[0]= xy[0]; cell->xy[1]= xy[1];
				cell->point_radius= point_radius;
				final_points[ point_count ]= cell;
				point_count++;

				processing_stack[ processing_stack_pos ]= cell;
				processing_stack_pos++;
			}
			xy_loop_break:;
		} // for place points
	} // while low points

	// zero rows
	for( unsigned int i= 0; i< static_objects_row_count_; i++ )
	{
		static_objects_rows_[i].objects_count= 0;
		static_objects_rows_[i].last_object_index= 0;
	}
	// calculate objects in row
	for( unsigned int i= 0; i< point_count; i++ )
	{
		int ind= int(final_points[i]->xy[1] * grid_cell_size_f) / MF_STATIC_OBJECTS_ROW_SIZE_CL;
		static_objects_rows_[ind].objects_count++;
	}
	// allocate memory for objects
	for( unsigned int i= 0; i< static_objects_row_count_; i++ )
		static_objects_rows_[i].objects= new mf_StaticLevelObject[ static_objects_rows_[i].objects_count ];

	// place objects in rows
	for( unsigned int i= 0; i< point_count; i++ )
	{
		float terrain_space_xy[2]=
		{
			final_points[i]->xy[0] * grid_cell_size_f,
			final_points[i]->xy[1] * grid_cell_size_f
		};

		mf_StaticLevelObjectsRow* row= &static_objects_rows_[ int(terrain_space_xy[1]) / MF_STATIC_OBJECTS_ROW_SIZE_CL ];
		mf_StaticLevelObject* obj= &row->objects[ row->last_object_index ];

		obj->pos[0]= terrain_space_xy[0] * terrain_cell_size_;
		obj->pos[1]= terrain_space_xy[1] * terrain_cell_size_;

		obj->pos[2]= GetTerrainHeight( obj->pos[0], obj->pos[1] ) - 0.3f;

		obj->type= mf_StaticLevelObject::Type( randomizer.Rand() % mf_StaticLevelObject::LastType );
		obj->scale= randomizer.RandF( 0.8f, 1.2f );
		obj->z_angle= randomizer.RandF( MF_2PI );

		row->last_object_index++;
	}

	for( unsigned int i= 0; i< static_objects_row_count_; i++ )
		SortStaticObjectsRow( &static_objects_rows_[i] );
	
	delete[] grid;
	delete[] processing_stack;
	delete[] final_points;

	MF_DEBUG_INFO_STR_I( "static objects on level: ", point_count );
}

void mf_Level::SortStaticObjectsRow( mf_StaticLevelObjectsRow* row )
{
	unsigned int count_by_type[ mf_StaticLevelObject::LastType ];
	unsigned int offset_by_type[ mf_StaticLevelObject::LastType ];

	for( unsigned int i= 0; i< mf_StaticLevelObject::LastType ; i++ )
		count_by_type[i]= 0;

	for( unsigned int i= 0; i< row->objects_count; i++ )
		count_by_type[row->objects[i].type]++;

	unsigned int offset= 0;
	for( unsigned int i= 0; i< mf_StaticLevelObject::LastType; i++ )
	{
		offset_by_type[i]= offset;
		offset+= count_by_type[i];
	}

	mf_StaticLevelObject* new_objects= new mf_StaticLevelObject[ row->objects_count ];

	for( unsigned int i= 0; i< row->objects_count; i++ )
	{
		mf_StaticLevelObject* obj= &row->objects[i];
		new_objects[ offset_by_type[obj->type] ]= *obj;
		offset_by_type[obj->type]++;
	}

	delete[] row->objects;
	row->objects= new_objects;
}