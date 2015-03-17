#include "micro-f.h"
#include "level.h"

#include "mf_math.h"
#include "textures_generation.h"

#define MF_MAX_VALLEY_WAY_POINTS 1024

static unsigned short Noise2( int x,  int y ) 
{
	int n= x + y * 57;
	n= (n << 13) ^ n;
	return (unsigned short)(
		((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) /0x7fff
		);
}

static unsigned short InterpolatedNoise( unsigned int x, unsigned int y, unsigned int k )
{
	unsigned int step= 1<<k;
	unsigned int X= x >> k;
	unsigned int Y= y >> k;

	unsigned int noise[4]=
	{
		Noise2(X, Y ),
		Noise2(X + 1, Y ),
		Noise2(X + 1, Y + 1 ),
		Noise2(X, Y + 1 )
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

static unsigned short FinalNoise(unsigned int x, unsigned int y)
{
	unsigned short r= 0;
	for( int i= 0; i<= 6; i++ )
		r+= InterpolatedNoise( x, y, 7 - i )>>(1+i);
	return r;
}

mf_Level::mf_Level()
{
	terrain_size_[0]= 512;
	terrain_size_[1]= 1024;
	terrain_amplitude_= 144.0f;
	terrain_cell_size_= 2.0f;
	terrain_water_level_= terrain_amplitude_ / 9.0f;

	terrain_heightmap_data_= new unsigned short[ terrain_size_[0] * terrain_size_[1] ];
	terrain_normal_textures_map_= new char[ terrain_size_[0] * terrain_size_[1] * 4 ];

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
	delete[] valley_way_points_;

	//for( unsigned int i= 0; i< static_objects_row_count_; i++ )
	//	delete[] static_objects_rows_[i].objects;
	delete[] static_objects_rows_;
}

void mf_Level::GenTarrain()
{
	unsigned short* primary_terrain_data= new unsigned short[ terrain_size_[0] * terrain_size_[1] ];

	// first gen
	for( unsigned int y= 0; y< terrain_size_[1]; y++ )
		for( unsigned int x= 0; x< terrain_size_[0]; x++ )
		{
			unsigned int noise= FinalNoise(x,y);
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
	mf_Rand randomizer;

	const float y_range[]= {96.0f, 128.0f };
	const float x_amplitude= 144.0f;

	unsigned int y= 16;

	float x_center= float(terrain_size_[0]/2);
	while( y < terrain_size_[1] - 32 )
	{
		float x= x_center + randomizer.RandF( -x_amplitude, x_amplitude );
		float dy= randomizer.RandF( y_range[0], y_range[1] );

		y+= (unsigned int)(dy);
		if( y >= terrain_size_[1] -32)
			break;

		valley_way_points_[ valley_way_point_count_ ].x= (unsigned int)(x);
		valley_way_points_[ valley_way_point_count_ ].y= y;
		valley_way_points_[ valley_way_point_count_ ].h= randomizer.RandI( 0,0xFFFF/12 );
		valley_way_point_count_++;
	}

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

			double der0= double( int(valley_way_points_[i].x) - int(valley_way_points_[i-1].x) ) /
				double( int(valley_way_points_[i].y) - int(valley_way_points_[i-1].y) );

			double der1= double( int(valley_way_points_[i+1].x) - int(valley_way_points_[i].x) ) /
				float( int(valley_way_points_[i+1].y) - int(valley_way_points_[i].y) );

			double der2= double( int(valley_way_points_[i+2].x) - int(valley_way_points_[i+1].x) ) /
				float( int(valley_way_points_[i+2].y) - int(valley_way_points_[i+1].y) );

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

		int dy= int(valley_way_points_[i+1].y - valley_way_points_[i].y);
		int dh= int(valley_way_points_[i+1].h - valley_way_points_[i].h);
		double h_d= double(valley_way_points_[i].h);
		double dh_d= double(dh) / double(dy);
		for( int y= valley_way_points_[i].y; y< int(valley_way_points_[i+1].y); y++, h_d+= dh_d )
		{
			double y_d= double(y) + 0.5;
			double x_center_d= (y_d * abcd[0] * y_d * y_d) + (y_d * abcd[1] * y_d) + (y_d * abcd[2]) + (abcd[3]);
			int x_center= int(x_center_d);

			static const double init_width2= 48.0;
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
		: initialized(false)
	{
	}

	float xy[2];
	bool initialized;
};

void mf_Level::PlaceStaticObjects()
{
	mf_Rand randomizer;

	const float eps= 0.05f;
	const float min_objects_distance_cl= 4.0f;
	const unsigned int neighbor_k= 16;
	const float radius_min= 1.0f + eps;
	const float radius_max= 2.0f - eps;

	unsigned int grid_size[2]= { 64, 72 };

	mf_GridCell* grid= new mf_GridCell[ grid_size[0] * grid_size[1] ];

	const unsigned int target_point_count= grid_size[0] * grid_size[1];
	unsigned int point_count= 0;

	mf_GridCell* processing_stack= new mf_GridCell[ grid_size[0] * grid_size[1] ];
	unsigned int processing_stack_pos= 1;

	mf_GridCell* final_points= new mf_GridCell[ target_point_count ];

	mf_GridCell* current_point;
	current_point= &processing_stack[0];
	current_point->xy[0]= randomizer.RandF( float(grid_size[0]/2) );
	current_point->xy[1]= randomizer.RandF( float(grid_size[1]/2) );
	current_point->initialized= true;
	grid[ int(current_point->xy[0]) + int(current_point->xy[1]) * grid_size[0] ]= *current_point;
	final_points[ point_count ]= *current_point;
	point_count++;

	while( point_count < target_point_count )
	{
		if( processing_stack_pos == 0 ) break;

		// pop point from processing stack
		current_point= &processing_stack[ processing_stack_pos - 1 ];
		MF_ASSERT(current_point->initialized);
		processing_stack_pos--;

		// try place neighbor points
		for( unsigned int i= 0; i< neighbor_k; i++ )
		{
			float angle= randomizer.RandF( 0.0f, MF_2PI );
			float r= randomizer.RandF( radius_min, radius_max );
			float xy[2]=
			{
				mf_Math::sin(angle) * r + current_point->xy[0],
				mf_Math::cos(angle) * r + current_point->xy[1]
			};
			int xy_int[2]= { int(mf_Math::ceil(xy[0])), int(mf_Math::ceil(xy[1])) };
			bool is_outside_rect= false;
			for( unsigned int k= 0; k< 2; k++ )
			{
				if( xy_int[k] < 0 ) is_outside_rect= true;
				else if( xy_int[k] >= int(grid_size[k]) ) is_outside_rect= true;
			}
			if (is_outside_rect) continue;

			bool space_around_is_free= true;
			for( unsigned int j= 0; j< 8 * 8; j++ )
			{
				static const char deltas_xy[]=
				{
					-4, -4,  -3,-4,  -2,-4,  -1,-4,  0,-4,  1,-4,  2,-4,  3, -4,
					-4, -3,  -3,-3,  -2,-3,  -1,-3,  0,-3,  1,-3,  2,-3,  3, -3,
					-4, -2,  -3,-2,  -2,-2,  -1,-2,  0,-2,  1,-2,  2,-2,  3, -2,
					-4, -1,  -3,-1,  -2,-1,  -1,-1,  0,-1,  1,-1,  2,-1,  3, -1,
					-4,  0,  -3, 0,  -2, 0,  -1, 0,  0, 0,  1, 0,  2, 0,  3,  0,
					-4,  1,  -3, 1,  -2, 1,  -1, 1,  0, 1,  1, 1,  2, 1,  3,  1,
					-4,  2,  -3, 2,  -2, 2,  -1, 2,  0, 2,  1, 2,  2, 2,  3,  2,
					-4,  3,  -3, 3,  -2, 3,  -1, 3,  0, 3,  1, 3,  2, 3,  3,  3
				};
				int neighbor_cell_xy[2]=
				{
					xy_int[0] + int(deltas_xy[j*2  ]), 
					xy_int[1] + int(deltas_xy[j*2+1])
				};
				for( unsigned int k= 0; k< 2; k++ )
				{
					if( neighbor_cell_xy[k] < 0 ) neighbor_cell_xy[k]= 0;
					else if( neighbor_cell_xy[k] >= int(grid_size[k]) ) neighbor_cell_xy[k]= grid_size[k] - 1;
				}
				const mf_GridCell* cell= &grid[ neighbor_cell_xy[0] + neighbor_cell_xy[1] * grid_size[0] ];
				if( cell->initialized )
				{
					float dx_dy[2]= { cell->xy[0] - xy[0], cell->xy[1] - xy[1] };
					if( dx_dy[0] * dx_dy[0] + dx_dy[1] * dx_dy[1] <= 2.01f/*MF_INV_SQRT_2 * MF_INV_SQRT_2*/ )
					{
						space_around_is_free= false;
						break;
					}
				}
			}

			mf_GridCell* cell= &grid[ xy_int[0] + xy_int[1] * grid_size[0] ];
			if( space_around_is_free )
			{
				MF_ASSERT(!cell->initialized);
				if( processing_stack_pos < target_point_count - 1 )
				{
					cell->xy[0]= xy[0]; cell->xy[1]= xy[1];
					cell->initialized= true;

					final_points[ point_count ]= *cell;
					point_count++;

					processing_stack[ processing_stack_pos ]= *cell;
					processing_stack_pos++;
				}
			}
		} // for place points
	} // while low points

	static_objects_rows_[0].objects_count= point_count;
	static_objects_rows_[0].objects= new mf_StaticLevelObject[ static_objects_rows_[0].objects_count ];
	/*for( unsigned int i= 0; i< static_objects_rows_[0].objects_count; i++ )
	{
		mf_StaticLevelObject* obj= &static_objects_rows_[0].objects[i];
		obj->pos[0]= final_points[i].xy[0] * min_objects_distance_cl * MF_INV_SQRT_2;
		obj->pos[1]= final_points[i].xy[1] * min_objects_distance_cl * MF_INV_SQRT_2;
		obj->pos[2] = terrain_amplitude_;

		obj->type= mf_StaticLevelObject::Palm;
		obj->scale= 1.0f;
		obj->z_angle= randomizer.RandF( 0.0f, MF_2PI );
	}*/
	mf_StaticLevelObject* obj= &static_objects_rows_[0].objects[0];
	unsigned int obj_p= 0;
	for( unsigned int y= 0; y< grid_size[1]; y++ )
		for( unsigned int x= 0; x< grid_size[0]; x++ )
		{
			const mf_GridCell* cell= &grid[ x + y * grid_size[0] ];
			if( cell->initialized)
			{
				obj->pos[0]= cell->xy[0] * min_objects_distance_cl * MF_INV_SQRT_2;
				obj->pos[1]= cell->xy[1] * min_objects_distance_cl * MF_INV_SQRT_2;
				obj->pos[2] = terrain_amplitude_;

				obj->type= mf_StaticLevelObject::Palm;
				obj->scale= 1.0f;
				obj->z_angle= randomizer.RandF( 0.0f, MF_2PI );
				obj++;
				obj_p++;
				//MF_ASSERT(obj_p< static_objects_rows_[0].objects_count );
			}
		}
}