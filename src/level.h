#pragma once
#include "micro-f.h"

#define MF_STATIC_OBJECTS_ROW_SIZE_CL 64

struct mf_StaticLevelObject
{
	enum Type
	{
		Palm= 0,
		Oak,
		Spruce,
		Birch,
		LastType
	};
	
	Type type;
	float pos[3]; // world space position
	float scale;
	float z_angle;
};

struct mf_StaticLevelObjectsRow
{
	mf_StaticLevelObject* objects;
	unsigned int objects_count;
	unsigned int last_object_index; // for generation
};

class mf_Level
{
public:
	mf_Level();
	~mf_Level();

	const unsigned short* GetTerrianHeightmapData() const;
	const char* GetTerrainNormalTextureMap() const;
	unsigned int TerrainSizeX() const;
	unsigned int TerrainSizeY() const;
	float TerrainAmplitude() const;
	float TerrainCellSize() const;
	float TerrainWaterLevel() const;

	float ForcefieldRadius() const;
	float ForcefieldZPos() const;

	const mf_StaticLevelObjectsRow* GetStaticObjectsRows() const;
	unsigned int GetStaticObjectsRowCount() const;

	// All inputs - in world space
	float GetTerrainHeight( float x, float y ) const;
	float GetValleyCenterX( float y ) const;
	bool SphereIntersectTerrain( const float* pos, float radius ) const;

private:
	void GenTarrain();
	void GenValleyWayPoints();
	void PlaceTextures();
	void PlaceStaticObjects();

	void SortStaticObjectsRow( mf_StaticLevelObjectsRow* row );

private:
	struct ValleyWayPoint
	{
		unsigned int x, y; // in terrain units
		unsigned int h; // height in terrain scale
	};

	struct ValleyYparams
	{
		unsigned short x_begin, x_end, x_center;
	};

	unsigned short* terrain_heightmap_data_;
	 // combined data of terrain normals and textures. Format : n.x, n.y, n.z, texture_number
	char* terrain_normal_textures_map_;
	float terrain_amplitude_; // in meters
	float terrain_cell_size_; // in meters
	float terrain_water_level_;

	unsigned int terrain_size_[2];

	float forcefield_radius_;
	float forcefield_z_pos_;

	ValleyWayPoint* valley_way_points_;
	unsigned int valley_way_point_count_;
	ValleyYparams* valley_y_params_;

	mf_StaticLevelObjectsRow* static_objects_rows_;
	unsigned int static_objects_row_count_;
};

inline const unsigned short* mf_Level::GetTerrianHeightmapData() const
{
	return terrain_heightmap_data_;
}

inline const char* mf_Level::GetTerrainNormalTextureMap() const
{
	return terrain_normal_textures_map_;
}

inline unsigned int mf_Level::TerrainSizeX() const
{
	return terrain_size_[0];
}

inline unsigned int mf_Level::TerrainSizeY() const
{
	return terrain_size_[1];
}

inline float mf_Level::TerrainAmplitude() const
{
	return terrain_amplitude_;
}

inline float mf_Level::TerrainCellSize() const
{
	return terrain_cell_size_;
}

inline float mf_Level::TerrainWaterLevel() const
{
	return terrain_water_level_;
}

inline float mf_Level::ForcefieldRadius() const
{
	return forcefield_radius_;
}

inline float mf_Level::ForcefieldZPos() const
{
	return forcefield_z_pos_;
}

inline const mf_StaticLevelObjectsRow* mf_Level::GetStaticObjectsRows() const
{
	return static_objects_rows_;
}

inline unsigned int mf_Level::GetStaticObjectsRowCount() const
{
	return static_objects_row_count_;
}