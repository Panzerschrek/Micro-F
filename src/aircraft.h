#pragma once
#include "micro-f.h"

class mf_Aircraft
{
public:
	mf_Aircraft();
	~mf_Aircraft();

	const float* Pos() const;
	const float* Angle() const;
	const float* Velocity() const;

private:
	float pos_[3];
	float angle_[3];
	float velocity_[3];

	unsigned int hp_;

};