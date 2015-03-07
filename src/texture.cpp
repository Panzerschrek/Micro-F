#include "micro-f.h"
#include "texture.h"

#include "text.h"
#include "mf_math.h"

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

void mf_Texture::RandomPoints()
{
	/*unsigned int cell_size= 8;
	unsigned int max_points_in_cell= 8;
	unsigned int tex_size[]= { 1 << size_log2_[0],  1 << size_log2_[1] };
	unsigned int grid_size[2];

	for( unsigned int i= 0; i< 2; i++ )
	{
		grid_size[i]= tex_size[i] / cell_size;
		if( grid_size[i] * cell_size < tex_size[i] ) grid_size[i]++;
	}

	unsigned short* grid= new unsigned short[ grid_size[0] * grid_size[1] * 2 * max_points_in_cell ];

	float point[2];
	point[0]= RandF(float(tex_size[0]-1));
	point[1]= RandF(float(tex_size[1]-1));*/
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
		int dy_f= ( int( y1 - y0 )<<16 ) / ( x1 - x0 );
		while( x0 < x1 )
		{
			int y_i= y_f>>16;
			float* d= data_ + (( (x0&size_x1) + ((y_i&size_y1)<<size_log2_[0]) )<<2);
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color[j];
			y_f+= dy_f;
			x0++;
		};
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
		int dx_f= ( int( x1 - x0 )<<16 ) / ( y1 - y0 );
		while( y0 < y1 )
		{
			int x_i= x_f>>16;
			float* d= data_ + (( (x_i&size_x1) + ((y0&size_y1)<<size_log2_[0]) )<<2);
			for( unsigned int j= 0; j< 4; j++ )
				d[j]= color[j];
			x_f+= dx_f;
			y0++;
		};
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

void mf_Texture::Copy( const mf_Texture* t )
{
	memcpy( data_, t->data_, ( 1<<( size_log2_[0] + size_log2_[1] + 2 ) ) * sizeof(float) );
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