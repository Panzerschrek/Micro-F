#pragma once
#include "micro-f.h"

struct mf_Settings
{
	enum Quality
	{
		QualityLow,
		QualityMedium,
		QualityHeight,
		LastQuality
	};

	Quality shadows_quality;
	Quality terrain_quality;
	Quality sky_quality;
	bool use_hdr;
};