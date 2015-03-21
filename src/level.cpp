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
	terrain_size_[0]= 768;
	terrain_size_[1]= 2048;
	terrain_amplitude_= 144.0f;
	terrain_cell_size_= 2.0f;
	terrain_water_level_= terrain_amplitude_ / 9.0f;

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
			valley_y_params_[y].x_center= (unsigned short) x_center;

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
	mf_Rand randomizer;

	const unsigned int grid_cell_size= 3;
	const float grid_cell_size_f= float(grid_cell_size);
	const unsigned int max_radius_scaler= 8;
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
		current_point_terrain_space_xy_i[1]= int(current_point_terrain_space_xy[1]);
		float radius_scaler= mf_Math::fabs(float(
			current_point_terrain_space_xy[0]
			- int(valley_y_params_[current_point_terrain_space_xy_i[1]].x_center)))
			/ float(terrain_size_[0]/2);
		radius_scaler= mf_Math::clamp( 0.0f, 1.0f, radius_scaler );
		radius_scaler= max_radius_scaler_f * radius_scaler + 1.0f * ( 1.0f - radius_scaler );

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

		unsigned short h= terrain_heightmap_data_[ int(terrain_space_xy[0]) + int(terrain_space_xy[1]) * terrain_size_[0] ];
		obj->pos[2] = float(h) * terrain_amplitude_ / 65535.0f;

		obj->type= mf_StaticLevelObject::Palm;
		obj->scale= 0.25f * (
			randomizer.RandF( 0.8f, 1.2f ) +
			randomizer.RandF( 0.8f, 1.2f ) +
			randomizer.RandF( 0.8f, 1.2f ) +
			randomizer.RandF( 0.8f, 1.2f ) );
		obj->z_angle= randomizer.RandF( 0.0f, MF_2PI );

		row->last_object_index++;
	}
	
	delete[] grid;
	delete[] processing_stack;
	delete[] final_points;

	MF_DEBUG_INFO_STR_I( "static objects on level: ", point_count );
}