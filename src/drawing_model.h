#pragma once
#include "micro-f.h"

// 32-byte GPU vertex
struct mf_DrawingModelVertex
{
	float pos[3];
	float normal[3];
	float tex_coord[2];
};

class mf_DrawingModel
{
public:
	mf_DrawingModel();
	~mf_DrawingModel();

	const mf_DrawingModelVertex* GetVertexData() const;
	const unsigned short* GetIndexData() const;
	unsigned int VertexCount() const;
	unsigned int IndexCount() const;

	void SetVertexData( mf_DrawingModelVertex* vertices, unsigned int vertex_count );
	void SetIndexData( unsigned short* indeces, unsigned int index_count );

	void Copy( const mf_DrawingModel* m );

	void LoadFromMFMD( const unsigned char* model_data );

	void Add( const mf_DrawingModel* m );

	void Scale( const float* scale_vec );
	void Scale( float s );
	void Shift( const float* shift_vec );
	void Rotate( const float* normal_mat );

	void ScaleTexCoord( const float* scale_vec );
	void ShiftTexCoord( const float* shift_vec );
	void TransformTexCoord( const float* transform_mat3x3 );

	void FlipTriangles();

private:
	mf_DrawingModelVertex* vertices_;
	unsigned short* indeces_;
	unsigned int vertex_count_;
	unsigned int index_count_;
};

inline const mf_DrawingModelVertex* mf_DrawingModel::GetVertexData() const
{
	return vertices_;
}

inline const unsigned short* mf_DrawingModel::GetIndexData() const
{
	return indeces_;
}

inline unsigned int mf_DrawingModel::VertexCount() const
{
	return vertex_count_;
}

inline unsigned int mf_DrawingModel::IndexCount() const
{
	return index_count_;
}