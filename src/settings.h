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

	enum CloudsIntensity
	{
		CloudsDisabled,
		CloudsLow,
		CloudsMedium,
		CloudsHeight,
		LastCloudsQuality
	};

	Quality shadows_quality;
	Quality terrain_quality;
	Quality sky_quality;
	CloudsIntensity clouds_intensity;
	bool use_hdr;
};