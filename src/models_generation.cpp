#include "micro-f.h"
#include "models_generation.h"

#include "drawing_model.h"
#include "mf_math.h"



void GenGeosphere( mf_DrawingModel* model, unsigned int segments, unsigned int partition )
{
	unsigned int vertex_count= (segments + 1 ) * (partition + 1 );
	unsigned int index_count= segments * partition * 6;
	mf_DrawingModelVertex* v= new mf_DrawingModelVertex[ vertex_count ];
	unsigned short* ind= new unsigned short[ index_count ];

	float inv_segments= 1.0f / float(segments);
	float inv_partition= 1.0f / float(partition);

	float a, da= MF_2PI * inv_segments;
	float phi= -MF_PI2, d_phi= MF_PI * inv_partition;
	unsigned int i, j, k;
	for( j= 0, k=0; j<= partition; j++, phi+= d_phi )
	{
		float xy= mf_Math::cos(phi);
		float z= mf_Math::sin(phi);
		float tex_coord_y= float(j) * inv_partition;
		for( i= 0, a= 0.0f; i<= segments; i++, a+=da, k++ )
		{
			v[k].pos[0]= xy * cos(a);
			v[k].pos[1]= xy * sin(a);
			v[k].pos[2]= z;
			v[k].normal[0]= v[k].pos[0];
			v[k].normal[1]= v[k].pos[1];
			v[k].normal[2]= v[k].pos[2];

			v[k].tex_coord[0]= float(i) * inv_segments;
			v[k].tex_coord[1]= tex_coord_y;
		}
	}

	for( j= 0, k=0; j< partition; j++  )
		for( i= 0; i< segments; i++, k+=6 )
		{
			ind[k  ]= (unsigned short)( i + j * (segments + 1 ) );
			ind[k+2]= (unsigned short)( ind[k] + segments + 1 );
			ind[k+1]= (unsigned short)( ind[k+2] + 1 );

			ind[k+5]= (unsigned short)( ind[k] );
			ind[k+4]= (unsigned short)( ind[k+1] );
			ind[k+3]= (unsigned short)( ind[k]+1 );
		}

	model->SetVertexData( v, vertex_count );
	model->SetIndexData( ind, index_count );
}