#pragma once
#include "micro-f.h"

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

private:
	void GenTarrain();
	void GenValleyWayPoints();
	void PlaceTextures();

private:
	struct ValleyWayPoint
	{
		unsigned int x, y; // in terrain units
		unsigned int h; // height in terrain scale
	};

	unsigned short* terrain_heightmap_data_;
	 // combined data of terrain normals and textures. Format : n.x, n.y, n.z, texture_number
	char* terrain_normal_textures_map_;
	float terrain_amplitude_; // in meters
	float terrain_ceil_size_; // in meters
	float terrain_water_level_;

	unsigned int terrain_size_[2];

	ValleyWayPoint* valley_way_points_;
	unsigned int valley_way_point_count_;
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
	return terrain_ceil_size_;
}

inline float mf_Level::TerrainWaterLevel() const
{
	return terrain_water_level_;
}