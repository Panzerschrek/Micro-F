#include "micro-f.h"
#include "texture.h"

#include "text.h"
#include "mf_math.h"


void mfMonochromeImageTo8Bit( const unsigned char* in_data, unsigned char* out_data, unsigned int out_data_size )
{
	for( unsigned int i= 0; i< out_data_size / 8; i++ )
		for( unsigned int j= 0; j< 8; j++ )
			out_data[ (i<<3) + (7-j) ]= ( (in_data[i] & (1<<j)) >> j ) * 255;
}

mf_Texture::mf_Texture( unsigned int size_x_log2, unsigned int size_y_log2 )
	: data_( new float[ 1<<(size_x_log2 + size_y_log2 + 2) ] )
	, normalized_data_( NULL )
{
	size_log2_[0]= size_x_log2;
	size_log2_[1]= size_y_log2;
	size_[0]= 1 << size_x_log2;
	size_[1]= 1 << size_y_log2;
}

mf_Texture::~mf_Texture()
{
	delete[] data_;
	if( normalized_data_ != NULL )
		delete[] normalized_data_;
}

void mf_Texture::Noise( unsigned int octave_count )
{
	float* d= data_;
	for( unsigned int y= 0; y< size_[1]; y++ )
		for( unsigned int x= 0; x< size_[0]; x++, d+= 4 )
		{
			d[0]= d[1]= d[2]= d[3]=
				float( FinalNoise( x, y, octave_count ) ) / float(0xFFFF);
		}
}

void mf_Texture::PoissonDiskPoints( unsigned int min_distanse_div_sqrt2, unsigned int rand_seed  )
{
	mf_Rand randomizer(rand_seed);

	const int neighbor_k= 20;
	const float min_dst= float(min_distanse_div_sqrt2) * MF_SQRT_2 + 0.01f;
	const float min_dst2= min_dst * min_dst;

	int size_minus_1[2];
	size_minus_1[0]= size_[0] - 1;
	size_minus_1[1]= size_[1] - 1;

	int grid_size[2];
	int grid_cell_size_to_tex_size_k[2];
	for( unsigned int i= 0; i< 2; i++ )
	{
		grid_size[i]= size_[i] / min_distanse_div_sqrt2;
		if( grid_size[i] * min_distanse_div_sqrt2 < size_[i] ) grid_size[i]++;
		grid_cell_size_to_tex_size_k[i]= grid_size[i] * min_distanse_div_sqrt2 - size_[i];
	}

	// coord int grid - in pixels
	int max_point_count= grid_size[0] * grid_size[1];
	int* grid= new int[ max_point_count * 2 ];
	for( int i= 0; i< max_point_count; i++ )
		grid[i*2]= -1;

	int** processing_stack= new int*[ max_point_count ];

	int init_pos[2];
	init_pos[0]= randomizer.RandI( size_[0] - 1 );
	init_pos[1]= randomizer.RandI( size_[1] - 1 );
	int init_pos_grid_pos[2];
	init_pos_grid_pos[0]= init_pos[0] / min_distanse_div_sqrt2;
	init_pos_grid_pos[1]= init_pos[1] / min_distanse_div_sqrt2;
	processing_stack[0]= &grid[ (init_pos_grid_pos[0] + init_pos_grid_pos[1] * grid_size[0]) * 2 ];
	processing_stack[0][0]= init_pos[0];
	processing_stack[0][1]= init_pos[1];

	int processing_stack_pos= 1;
	while( processing_stack_pos != 0 )
	{
		int* current_point= processing_stack[ processing_stack_pos - 1 ];
		MF_ASSERT( current_point[0] != -1 );
		processing_stack_pos--;

		for( int n= 0; n< neighbor_k; n++ )
		{
			int pos[2];
			int grid_pos[2];

			float angle= randomizer.RandF( MF_2PI );
			float r= randomizer.RandF( min_dst, min_dst*2.0f );

			pos[0]= current_point[0] + int(r * mf_Math::cos(angle));
			pos[1]= current_point[1] + int(r * mf_Math::sin(angle));
			pos[0]= (pos[0] + size_[0]) & size_minus_1[0];
			pos[1]= (pos[1] + size_[1]) & size_minus_1[1];
			grid_pos[0]= pos[0] / min_distanse_div_sqrt2;
			grid_pos[1]= pos[1] / min_distanse_div_sqrt2;

			for( int y= grid_pos[1] - 4; y<= grid_pos[1] + 3; y++ )
				for( int x= grid_pos[0] - 4; x<= grid_pos[0] + 3; x++ )
				{
					int wrap_xy[2];
					wrap_xy[0]= (x + grid_size[0] ) % grid_size[0];
					wrap_xy[1]= (y + grid_size[1] ) % grid_size[1];

					int* cell= &grid[ (wrap_xy[0] + wrap_xy[1] * grid_size[0]) * 2 ];
					if( cell[0] != -1 )
					{
						int d_dst[2];
						d_dst[0]= pos[0] - cell[0] + ( wrap_xy[0] - x ) * min_distanse_div_sqrt2;
						d_dst[1]= pos[1] - cell[1] + ( wrap_xy[1] - y ) * min_distanse_div_sqrt2;

						if( wrap_xy[0] < x ) d_dst[0]+= grid_cell_size_to_tex_size_k[0];
						else if( wrap_xy[0] > x ) d_dst[0]-= grid_cell_size_to_tex_size_k[0];
						if( wrap_xy[1] < y ) d_dst[1]+= grid_cell_size_to_tex_size_k[1];
						else if( wrap_xy[1] > y ) d_dst[1]-= grid_cell_size_to_tex_size_k[1];

						if( float( d_dst[0] * d_dst[0] + d_dst[1] * d_dst[1] ) < min_dst2 )
							goto xy_loop_break;
					}
				}
			
			int* cell= &grid[ (grid_pos[0] + grid_pos[1] * grid_size[0]) * 2 ];
			MF_ASSERT( cell[0] == -1 );
			cell[0]= pos[0];
			cell[1]= pos[1];
			MF_ASSERT( processing_stack_pos < max_point_count );
			processing_stack[ processing_stack_pos++ ]= cell;

			xy_loop_break:;
		} // try place points near
	} // while 1

	float intencity_multipler= 1.0f / float(min_distanse_div_sqrt2 * 3);

	float* d= data_;
	for( int y= 0; y< int(size_[1]); y++ )
	{
		int grid_pos[2];
		grid_pos[1]= y / min_distanse_div_sqrt2;
		for( int x= 0; x< int(size_[0]); x++, d+= 4 )
		{
			grid_pos[0]= x / min_distanse_div_sqrt2;

			int nearest_point_dst2[2]= { 0xfffffff, 0xfffffff };

			for( int v= grid_pos[1] - 3; v<= grid_pos[1] + 3; v++ )
			{
				int grid_uv[2];
				grid_uv[1]= ( v + grid_size[1] ) % grid_size[1];
				for( int u= grid_pos[0] - 3; u <= grid_pos[0] + 3; u++ )
				{
					grid_uv[0]= ( u + grid_size[0] ) % grid_size[0];
					int* cell= &grid[ (grid_uv[0] + grid_uv[1] * grid_size[0]) * 2 ];
					if( cell[0] != -1 )
					{
						int d_dst[2];
						d_dst[0]= cell[0] - x + ( u - grid_uv[0] ) * min_distanse_div_sqrt2;
						d_dst[1]= cell[1] - y + ( v - grid_uv[1] ) * min_distanse_div_sqrt2;

						if( grid_uv[0] > u ) d_dst[0]+= grid_cell_size_to_tex_size_k[0];
						else if( grid_uv[0] < u ) d_dst[0]-= grid_cell_size_to_tex_size_k[0];
						if( grid_uv[1] > v ) d_dst[1]+= grid_cell_size_to_tex_size_k[1];
						else if( grid_uv[1] < v ) d_dst[1]-= grid_cell_size_to_tex_size_k[1];

						int dst2= d_dst[0] * d_dst[0] + d_dst[1] * d_dst[1];
						if( dst2 < nearest_point_dst2[0] )
						{
							nearest_point_dst2[1]= nearest_point_dst2[0];
							nearest_point_dst2[0]= dst2;
						}
						else if( dst2 < nearest_point_dst2[1] )
							nearest_point_dst2[1]= dst2;
					}
				} // for grid xy
			} // for grid v

			d[0]= mf_Math::sqrt(float(nearest_point_dst2[0])) * intencity_multipler;
			d[1]= ( mf_Math::sqrt(float(nearest_point_dst2[1])) - mf_Math::sqrt(float(nearest_point_dst2[0])) )
				* intencity_multipler;
			d[2]= mf_Math::sqrt(float(nearest_point_dst2[1])) * intencity_multipler;
			d[3]= 1.0f;
			{
				int* cell= &grid[ (grid_pos[0] + grid_pos[1] * grid_size[0]) * 2 ];
				if( cell[0] == x && cell[1] == y ) d[3]= 0.0f;
			}
		} // for x
	} // for y

	delete[] processing_stack;
	delete[] grid;
}

void mf_Texture::GenNormalMap()
{
	unsigned int size_x1= size_[0] - 1;
	unsigned int size_y1= size_[1] - 1;

	float* d= data_;
	for( unsigned int y= 0; y< size_[1]; y++ )
		for( unsigned int x= 0; x< size_[0]; x++, d+= 4 )
		{
			float val[8];
			unsigned int ys= ((y-1)&(size_y1)) << size_log2_[0];
			val[0]= data_[ 3+ ( ( ((x-1)&size_x1) + ys ) << 2 ) ];
			val[1]= data_[ 3+ ( (   x             + ys ) << 2 ) ];
			val[2]= data_[ 3+ ( ( ((x+1)&size_x1) + ys ) << 2 ) ];

			ys= y << size_log2_[0];
			val[3]= data_[ 3+ ( ( ((x-1)&size_x1) + ys ) << 2 ) ];
			//val[4]= data_[ 3+ ( (   x             + ys ) << 2 ) ];
			val[5]= data_[ 3+ ( ( ((x+1)&size_x1) + ys ) << 2 ) ];

			ys= ys= ((y+1)&(size_y1)) << size_log2_[0];
			val[6]= data_[ 3+ ( ( ((x-1)&size_x1) + ys ) << 2 ) ];
			val[7]= data_[ 3+ ( (   x             + ys ) << 2 ) ];
			val[8]= data_[ 3+ ( ( ((x+1)&size_x1) + ys ) << 2 ) ];

			float dx= ( val[2] + val[5] + val[8] - val[0] - val[3] - val[6] ) * 0.3333333f;
			float dy= ( val[6] + val[7] + val[8] - val[0] - val[1] - val[2] ) * 0.3333333f;

			d[0]= dx;
			d[1]= dy;
		}
}

void mf_Texture::Gradient( unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, const float* color0, const float* color1 )
{
	float plane_normal[]= { float(x1-x0), float(y1-y0), 0.0f };
	Vec3Normalize( plane_normal );

	float plane_k;
	float end_point[]= { float(x1), float(y1), 0.0f };

	plane_k= -Vec3Dot( plane_normal, plane_normal );
	float end_point_inv_len= 1.0f / ( Vec3Dot( plane_normal, end_point ) + plane_k );

	float* d= data_;
	for( unsigned int y= 0; y< size_[1]; y++ )
		for( unsigned int x= 0; x< size_[0]; x++, d+= 4 )
		{
			float vec[]= { float(x), float(y), 0.0f };
			float k= ( Vec3Dot( vec, plane_normal ) + plane_k ) * end_point_inv_len;
			if( k > 1.0f ) k= 1.0f; else if( k < 0.0f ) k = 0.0f;
			float k1= 1.0f - k;
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color0[j] * k + color1[j] * k1;
		}
}

void mf_Texture::RadialGradient( int center_x, int center_y, int radius, const float* color0, const float* color1 )
{
	int size_x1= size_[0] - 1;
	int size_y1= size_[1] - 1;
	float inv_radius_f= 1.0f / float(radius);

	for( int y= center_y - radius, y_end= center_y + radius; y<= y_end; y++ )
	{
		int dy= y - center_y;
		float dy2= float( dy * dy );
		int y_ind= y & size_y1;
		for( int x= center_x - radius, x_end= center_x + radius; x <= x_end; x++ )
		{
			int ind= ( (x&size_x1) + (y_ind<<size_log2_[0]) ) << 2;

			int dx= x - center_x;
			float r= inv_radius_f * mf_Math::sqrt( float(dx*dx) + dy2 );
			if( r > 1.0f ) r= 1.0f;
			float inv_r= 1.0f - r;
			for( unsigned int j= 0; j < 4; j++ )
				data_[ind+j]= color0[j] * inv_r + color1[j] * r;
		} // for x
	} // for y
}

void mf_Texture::Fill( const float* color )
{
	FillRect( 0, 0, size_[0], size_[1], color );
}

void mf_Texture::FillRect( unsigned int x, unsigned int y, unsigned int width, unsigned int height, const float* color )
{
	for( unsigned int j= y; j< y + height; j++ )
	{
		float* d= data_ + (x + (j<<size_log2_[0])) * 4;
		for( unsigned int i= x; i< x + width; i++, d+= 4 )
		{
			d[0]= color[0];
			d[1]= color[1];
			d[2]= color[2];
			d[3]= color[3];
		}
	}
}

void mf_Texture::FillEllipse( int center_x, int center_y, int radius, const float* color, float scale_x, float scale_y )
{
	int size_x1= size_[0] - 1;
	int size_y1= size_[1] - 1;
	float radius2= float(radius * radius);
	float scale_x2= scale_x * scale_x;
	float scale_y2= scale_y * scale_y;

	for( int y= center_y - radius, y_end= center_y + radius; y<= y_end; y++ )
	{
		int dy= y - center_y;
		float dy2= float(dy * dy) * scale_y2;
		int y_ind= y & size_y1;
		for( int x= center_x - radius, x_end= center_x + radius; x <= x_end; x++ )
		{
			int dx= x - center_x;
			if( float(dx*dx) * scale_x2 + dy2<= radius2 )
			{
				int ind= ( (x&size_x1) + (y_ind<<size_log2_[0]) ) << 2;
				data_[ind  ]= color[0];
				data_[ind+1]= color[1];
				data_[ind+2]= color[2];
				data_[ind+3]= color[3];
			}
		} // for x
	} // for y
}

void mf_Texture::DrawLine( unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, const float* color )
{
	unsigned int size_x1= size_[0] - 1;
	unsigned int size_y1= size_[1] - 1;

	if( abs( int(x0 - x1) ) > abs( int(y0 - y1) ) )
	{
		if( x1 < x0 )
		{
			unsigned int tmp= x1;
			x1= x0;
			x0= tmp;
			tmp= y1;
			y1= y0;
			y0= tmp;
		}

		int y_f= int(y0)<<16;
		int dy_f= ( ( int(y1) - int(y0) )<<16 ) / ( int(x1) - int(x0) );
		while( x0 < x1 )
		{
			int y_i= y_f>>16;
			float* d= data_ + (( (x0&size_x1) + ((y_i&size_y1)<<size_log2_[0]) )<<2);
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color[j];
			y_f+= dy_f;
			x0++;
		};
		/*float y_f= float(y0);
		float dy_f= float( int(y1) - int(y0) ) / float( int(x1) - int(x0) );
		while( x0 < x1 )
		{
			int y_i= int(y_f);
			float* d= data_ + (( (x0&size_x1) + ((y_i&size_y1)<<size_log2_[0]) )<<2);
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color[j];
			y_f+= dy_f;
			x0++;
		};*/
	}
	else
	{
		if( y1 < y0 )
		{
			unsigned int tmp= x1;
			x1= x0;
			x0= tmp;
			tmp= y1;
			y1= y0;
			y0= tmp;
		}

		int x_f= int(x0)<<16;
		int dx_f= ( ( int(x1) - int(x0) )<<16 ) / ( int(y1) - int(y0) );
		while( y0 < y1 )
		{
			int x_i= x_f>>16;
			float* d= data_ + (( (x_i&size_x1) + ((y0&size_y1)<<size_log2_[0]) )<<2);
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color[j];
			x_f+= dx_f;
			y0++;
		};
		/*float x_f= float(x0);
		float dx_f= float( int(x1) - int(x0) ) / float( int(y1) - int(y0) );
		while( y0 < y1 )
		{
			int x_i= int(x_f);
			float* d= data_ + (( (x_i&size_x1) + ((y0&size_y1)<<size_log2_[0]) )<<2);
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color[j];
			x_f+= dx_f;
			y0++;
		};*/
	}
}

void mf_Texture::Grayscale()
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		float g= ( d[0] + d[1] + d[2] ) * 0.3333333f;
		d[0]= d[1]= d[2]= g;
	}
}

void mf_Texture::Abs()
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		d[0]= fabsf(d[0]);
		d[1]= fabsf(d[1]);
		d[2]= fabsf(d[2]);
		d[3]= fabsf(d[3]);
	}
}

void mf_Texture::SinWaveDeformX( float amplitude, float freq, float phase )
{
	float* new_data= new float[ 1<<( size_log2_[0] + size_log2_[1] + 2) ];

	unsigned int size_y1= size_[1] - 1;

	float omega= freq * MF_2PI;
	for( unsigned int x= 0; x< size_[0]; x++ )
	{
		int dy= int( amplitude * mf_Math::sin( omega * float(x) + phase ) );

		for( unsigned int y= 0; y< size_[1]; y++ )
		{
			unsigned int k= x + ( ( (y+dy) & size_y1 ) << size_log2_[0] );
			k*= 4;
			unsigned int k_dst= x + ( y << size_log2_[0] );
			k_dst*= 4;
			for( unsigned int j= 0; j< 4; j++ )
				new_data[k_dst+j]= data_[k+j];
		}
	}

	delete[] data_;
	data_= new_data;
}

void mf_Texture::SinWaveDeformY( float amplitude, float freq, float phase )
{
	float* new_data= new float[ 1<<( size_log2_[0] + size_log2_[1] + 2) ];
	float* dst= new_data;

	unsigned int size_x1 = size_[0] - 1;

	float omega= freq * MF_2PI;
	for( unsigned int y= 0; y< size_[1]; y++ )
	{
		int dx= int( amplitude * mf_Math::sin( omega * float(y) + phase ) );

		for( unsigned int x= 0; x< size_[0]; x++, dst+= 4 )
		{
			unsigned int k= ( (x+dx) & size_x1 ) + (y<<size_log2_[0]);
			k*= 4;
			for( unsigned int j= 0; j< 4; j++ )
				dst[j]= data_[k+j];
		}
	}

	delete[] data_;
	data_= new_data;
}

void mf_Texture::DownscaleX()
{
	unsigned int size_x_minus_1= size_[0] - 1;

	float* new_data= new float[ size_[0] * size_[1] * 4 ];

	for( unsigned int y= 0; y< size_[1]; y++ )
	{
		float* data_src= data_ + (y << size_log2_[0]) * 4;
		float* data_dst= new_data + (y << size_log2_[0]) * 4;

		for( unsigned int x= 0; x< size_[0]; x++, data_dst+= 4 )
		{
			unsigned int x_src= ( (x<<1) & size_x_minus_1 ) * 4;
			for( unsigned int j= 0; j< 4; j++ )
				data_dst[j]= (data_src[x_src+j] + data_src[x_src+4+j]) * 0.5f;
		}
	}

	delete[] data_;
	data_= new_data;
}

void mf_Texture::DownscaleY()
{
}

void mf_Texture::Copy( const mf_Texture* t )
{
	memcpy( data_, t->data_, ( 1<<( size_log2_[0] + size_log2_[1] + 2 ) ) * sizeof(float) );
}

void mf_Texture::CopyRect( const mf_Texture* src, unsigned int width, unsigned int height, unsigned int x_dst, unsigned int y_dst, unsigned int x_src, unsigned int y_src )
{
	for( unsigned int v= 0; v< height; v++ )
	{
		const float* src_data= src->GetData() + ( x_src + ((v + y_src) << src->SizeXLog2()) ) * 4;
		float* dst_data= data_+ ( x_dst + ((v + y_dst) << size_log2_[0]) ) * 4;
		memcpy( dst_data, src_data, width * sizeof(float) * 4 );
	}
}

void mf_Texture::Rotate( float deg )
{
	float* new_data= new float[ 1<<(size_log2_[0] + size_log2_[1] + 2) ];
	float* d= new_data;
	unsigned int size_x1= (1 << size_log2_[0]) - 1;
	unsigned int size_y1= (1 << size_log2_[1]) - 1;

	float c= mf_Math::cos( deg * MF_DEG2RAD );
	float s= mf_Math::sin( deg * MF_DEG2RAD );
	float xc= float(1<<size_log2_[0]) * 0.5f;
	float yc= float(1<<size_log2_[1]) * 0.5f;

	for( unsigned int y= 0; y< size_[1]; y++ )
		for( unsigned int x= 0; x< size_[0]; x++, d+= 4 )
		{
			float mx= float(x) - xc;
			float my= float(y) - yc;
			float fx= mx * c - my * s + xc;
			float fy= mx * s + my * c + yc;
			
			const float* d_src= data_ + ( (((unsigned int)(fx))&size_x1) + (((unsigned int)(fy))&size_y1) * size_[0] ) * 4;
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= d_src[j];
		}

	delete[] data_;
	data_= new_data;
}

void mf_Texture::Shift( unsigned int dx, unsigned int dy )
{
	unsigned int size_x1= size_[0] - 1;
	unsigned int size_y1= size_[1] - 1;
	
	float* new_data= new float[ 1<<(size_log2_[0] + size_log2_[1] + 2) ];
	float* d= new_data;

	for( unsigned int y= 0; y< size_[1]; y++ )
	{
		unsigned int y1= (y+dy) & size_y1;
		for( unsigned int x= 0; x< size_[0]; x++, d+= 4 )
		{
			unsigned int k= ((x+dx)&size_x1) + ( y1 << size_log2_[0] );
			k*= 4;
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= data_[k+j];
		}
	}

	delete[] data_;
	data_= new_data;
}

void mf_Texture::Invert( const float* add_color )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		d[0]= add_color[0] - d[0];
		d[1]= add_color[1] - d[1];
		d[2]= add_color[2] - d[2];
		d[3]= add_color[3] - d[3];
	}
}

void mf_Texture::Add( const mf_Texture* t )
{
	const float* d1= t->data_;
	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		d0[0]+= d1[0];
		d0[1]+= d1[1];
		d0[2]+= d1[2];
		d0[3]+= d1[3];
	}
}

void mf_Texture::Sub( const mf_Texture* t )
{
	const float* d1= t->data_;
	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		d0[0]-= d1[0];
		d0[1]-= d1[1];
		d0[2]-= d1[2];
		d0[3]-= d1[3];
	}
}

void mf_Texture::Mul( const mf_Texture* t )
{
	const float* d1= t->data_;
	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		d0[0]*= d1[0];
		d0[1]*= d1[1];
		d0[2]*= d1[2];
		d0[3]*= d1[3];
	}
}

void mf_Texture::Max( const mf_Texture* t )
{
	const float* d1= t->data_;
	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
			d0[j]= (d0[j] > d1[j]) ? d0[j] : d1[j];
	}
}

void mf_Texture::Min( const mf_Texture* t )
{
	const float* d1= t->data_;
	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
			d0[j]= (d0[j] < d1[j]) ? d0[j] : d1[j];
	}
}

void mf_Texture::Add( const float* color )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		d[0]+= color[0];
		d[1]+= color[1];
		d[2]+= color[2];
		d[3]+= color[3];
	}
}

void mf_Texture::Sub( const float* color )
{
	float mcolor[]= { -color[0], -color[1], -color[2], -color[3] };
	Add( mcolor );
}

void mf_Texture::Mul( const float* color )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		d[0]*= color[0];
		d[1]*= color[1];
		d[2]*= color[2];
		d[3]*= color[3];
	}
}

void mf_Texture::Max( const float* color )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
			d[j]= (d[j] > color[j]) ? d[j] : color[j];
	}
}

void mf_Texture::Min( const float* color )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
			d[j]= (d[j] < color[j]) ? d[j] : color[j];
	}
}

void mf_Texture::Pow( float p )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		d[0]= mf_Math::pow( d[0], p );
		d[1]= mf_Math::pow( d[1], p );
		d[2]= mf_Math::pow( d[2], p );
		d[3]= mf_Math::pow( d[3], p );
	}
}

void mf_Texture::Mix( const float* color0, const float* color1, const float* sub_color )
{
	float* d= data_;
	float* d_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	for( ; d< d_end; d+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
			d[j]= color0[j] * d[j] + color1[j] * (sub_color[j] - d[j]);
	}
}

void mf_Texture::DrawText( unsigned int x, unsigned int y, unsigned int size, const float* color, const char* text )
{
	unsigned int size_x1= size_[0] - 1;
	unsigned int size_y1= size_[1] - 1;

	unsigned int d;
	unsigned int  x0= x;
	const unsigned char* text_data= mf_Text::GetFontData();
	while( *text != 0 )
	{
		if( *text == '\n' )
		{
			y-= MF_LETTER_HEIGHT * size;
			x=x0;
			text++;
			continue;
		}

		for( unsigned int j= 0; j< MF_LETTER_HEIGHT * size; j++ )
			for( unsigned int i= 0; i< MF_LETTER_WIDTH * size; i++ )
			{
				 d= i/size + (*text - 32) * MF_LETTER_WIDTH  + j / size * MF_FONT_BITMAP_WIDTH;
				if( text_data[d] != 0 )
				{
					d= (((i+x)&size_x1) + (((j+y)&size_y1)<<size_log2_[0]))<<2;
					for( unsigned int k= 0; k< 3; k++ )
						data_[d+k]= color[k];
				}
			}
		text++;
		x+= MF_LETTER_WIDTH * size;
	}
}

void mf_Texture::LinearNormalization( float k )
{
	AllocateNormalizedData();

	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	unsigned char* d1= normalized_data_;

	float mk= 255.0f * k;
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
		{
			int c= int(d0[j] * mk );
			if( c < 0 ) c= 0; else if ( c > 255 ) c= 255;
			d1[j]= (unsigned char)c;
		}
	}
}

void mf_Texture::ExpNormalization( float k )
{
	AllocateNormalizedData();

	float* d0= data_;
	float* d0_end= data_ + (1<<( size_log2_[0] + size_log2_[1] + 2));
	unsigned char* d1= normalized_data_;

	float mk= -k;
	for( ; d0< d0_end; d0+= 4, d1+= 4 )
	{
		for( unsigned int j= 0; j< 4; j++ )
		{
			int c= int( 255.0f * ( 1.0f - mf_Math::exp( d0[j] * mk ) ) );
			if( c < 0 ) c= 0; else if ( c > 255 ) c= 255;
			d1[j]= (unsigned char)c;
		}
	}
}

unsigned short mf_Texture::Noise2( int x, int y, unsigned int mask )
{
	x&= mask;
	y&= mask;

	int n = x + y * 57;
	n = (n << 13) ^ n;
	return (unsigned short)(
		((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 0x7fff
		);

}

unsigned short mf_Texture::InterpolatedNoise( unsigned int x, unsigned int y, unsigned int k )
{
	unsigned int step= 1<<k;
	unsigned int mask= ((1<<size_log2_[1])>>k)-1;//DO NOT TOUCH! This value make noise tilebale!

	unsigned int X= x >> k;
	unsigned int Y= y >> k;

	unsigned int noise[4]=
	{
		Noise2(X, Y, mask),
		Noise2(X + 1, Y, mask),
		Noise2(X + 1, Y + 1, mask),
		Noise2(X, Y + 1, mask)
	};

	unsigned int dx= x - (X <<k );
	unsigned int dy= y - (Y <<k );

	unsigned int interp_x[2]=
	{
		dy * noise[3] + (step - dy) * noise[0],
		dy * noise[2] + (step - dy) * noise[1]
	};
	return (unsigned short)(
		(interp_x[1] * dx + interp_x[0] * (step - dx) )>>(k+k)
		);
}

unsigned short mf_Texture::FinalNoise( unsigned int x, unsigned int y, unsigned int octave_count )
{
	unsigned short r=0;

	int od= 8 - octave_count;
	for( int i= 1 + od, j= 7 - od; i< 8; i++, j-- )
		r+= InterpolatedNoise(x,y,i)>>j;
	return r;
}

void mf_Texture::AllocateNormalizedData()
{
	if( normalized_data_ == NULL )
		normalized_data_= new unsigned char[ 1<<(size_log2_[0] + size_log2_[1] + 2) ];
}