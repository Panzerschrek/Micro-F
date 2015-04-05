#pragma once
#include "micro-f.h"

extern short* (* const sound_gen_func[])(unsigned int sample_rate, unsigned int* out_samples_count);