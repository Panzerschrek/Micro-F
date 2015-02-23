#pragma once
#include "micro-f.h"

class mf_Player
{
public:
	mf_Player();
	~mf_Player();

	void Tick( float dt);

	const float* Pos() const;
	const float* Angle() const;

	void RotateX( float pixel_delta );
	void RotateZ( float pixel_delta );
	void SetPos( float x, float y, float z );

	void ForwardPressed();
	void BackwardPressed();
	void LeftPressed();
	void RightPressed();
	void ForwardReleased();
	void BackwardReleased();
	void LeftReleased();
	void RightReleased();

	void UpPressed();
	void DownPressed();
	void UpReleased();
	void DownReleased();

	void RotateUpPressed();
	void RotateDownPressed();
	void RotateLeftPressed();
	void RotateRightPressed();
	void RotateUpReleased();
	void RotateDownReleased();
	void RotateLeftReleased();
	void RotateRightReleased();

private:
	float pos_[3];

	// 0 - pitch / tangaž
	// 1 - yaw / ryskanije
	// 2 - rall / kren
	float angle_[3];

	float aspect_;
	float fov_;

	bool forward_pressed_, backward_pressed_, left_pressed_, right_pressed_;
	bool up_pressed_, down_pressed_;
	bool rotate_up_pressed_, rotate_down_pressed_, rotate_left_pressed_, rotate_right_pressed_;
};


inline const float* mf_Player::Pos() const
{
	return pos_;
}

inline const float* mf_Player::Angle() const
{
	return angle_;
}

inline void mf_Player::RotateX( float pixel_delta )
{
	angle_[0]+= pixel_delta * 0.01f;
}

inline void mf_Player::RotateZ( float pixel_delta )
{
	angle_[2]+= pixel_delta * 0.01f;
}

inline void mf_Player::SetPos( float x, float y, float z )
{
	pos_[0]= x; pos_[1]= y; pos_[2]= z;
}

inline void mf_Player::ForwardPressed()
{
	forward_pressed_= true;
}

inline void mf_Player::BackwardPressed()
{
	backward_pressed_= true;
}

inline void mf_Player::LeftPressed()
{
	left_pressed_= true;
}

inline void mf_Player::RightPressed()
{
	right_pressed_= true;
}

inline void mf_Player::ForwardReleased()
{
	forward_pressed_= false;
}

inline void mf_Player::BackwardReleased()
{
	backward_pressed_= false;
}

inline void mf_Player::LeftReleased()
{
	left_pressed_= false;
}

inline void mf_Player::RightReleased()
{
	right_pressed_= false;
}

inline void mf_Player::UpPressed()
{
	up_pressed_= true;
}

inline void mf_Player::DownPressed()
{
	down_pressed_= true;
}

inline void mf_Player::UpReleased()
{
	up_pressed_= false;
}

inline void mf_Player::DownReleased()
{
	down_pressed_= false;
}

inline void mf_Player::RotateUpPressed()
{
	rotate_up_pressed_= true;
}

inline void mf_Player::RotateDownPressed()
{
	rotate_down_pressed_= true;
}

inline void mf_Player::RotateLeftPressed()
{
	rotate_left_pressed_= true;
}

inline void mf_Player::RotateRightPressed()
{
	rotate_right_pressed_= true;
}

inline void mf_Player::RotateUpReleased()
{
	rotate_up_pressed_= false;
}

inline void mf_Player::RotateDownReleased()
{
	rotate_down_pressed_= false;
}

inline void mf_Player::RotateLeftReleased()
{
	rotate_left_pressed_= false;
}

inline void mf_Player::RotateRightReleased()
{
	rotate_right_pressed_= false;
}