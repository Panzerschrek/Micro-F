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

	enum Daytime
	{
		DaytimeSunrise,
		DaytimeMorning,
		DaytimeMidday,
		DaytimeEvening,
		DaytimeSunset,
		DaytimeNight,
		LastDaytime
	};

	Quality shadows_quality;
	Quality terrain_quality;
	Quality sky_quality;
	CloudsIntensity clouds_intensity;
	bool use_hdr;
	Daytime daytime;
};