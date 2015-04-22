#pragma once
#include "micro-f.h"
#include "aircraft.h"
#include "autopilot.h"
#include "game_logic.h"

class mf_Player
{
public:

	enum ControlMode
	{
		ModeDebugFreeFlight,
		ModeAircraftControl
	};

	enum ViewMode
	{
		ViewThirdperson,
		ViewInsideCockpit
	};

	mf_Player();
	~mf_Player();

	void Tick( float dt );

	const float* Pos() const;
	const float* Angle() const;
	const float Fov() const;

	float GetMachinegunCircleRadius() const;

	void SetControlMode( ControlMode mode );
	ControlMode GetControlMode() const;
	void SetViewMode( ViewMode mode );
	void ToggleViewMode();
	ViewMode GetViewMode() const;

	void ScreenPointToWorldSpaceVec( unsigned int x, unsigned int y, float* out_vec ) const;

	void RotateX( float pixel_delta );
	void RotateZ( float pixel_delta );
	void SetPos( float x, float y, float z );

	const mf_Aircraft* GetAircraft() const;
	mf_Aircraft* GetAircraft();

	unsigned int Score() const;
	void AddScorePoints( unsigned int points );
	void ResetScore();

	void AddEnemyAircraft( mf_Aircraft* aircraft );
	void RemoveEnemyAircraft( mf_Aircraft* aircraft );
	const mf_Aircraft* const* GetEnemiesAircrafts() const;
	unsigned int EnemiesAircraftsCount() const;
	void SetTargetAircraft( mf_Aircraft* aircraft );
	const mf_Aircraft* TargetAircraft() const;

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

	void RotateClockwisePressed();
	void RotateAnticlockwisePressed();

	void RotateClockwiseReleased();
	void RotateAnticlockwiseReleased();

	void ZoomIn();
	void ZoomOut();

private:
	void CalculateCamRadius();

private:
	ControlMode control_mode_;
	ViewMode view_mode_;
	mf_Aircraft aircraft_;
	mf_Autopilot autopilot_;

	float pos_[3];

	// 0 - pitch / tangaž
	// 1 - yaw / ryskanije
	// 2 - roll / kren
	float angle_[3];

	float cam_radius_;
	float aspect_;
	float fov_;
	float target_fov_;

	bool forward_pressed_, backward_pressed_, left_pressed_, right_pressed_;
	bool up_pressed_, down_pressed_;
	bool rotate_up_pressed_, rotate_down_pressed_, rotate_left_pressed_, rotate_right_pressed_;
	bool rotate_clockwise_pressed_, rotate_anticlockwise_pressed_;

	unsigned int score_;

	mf_Aircraft* enemies_aircrafts_[ MF_MAX_ENEMIES ];
	unsigned int enemies_count_;

	mf_Aircraft* target_aircraft_;
};

inline const float* mf_Player::Pos() const
{
	return pos_;
}

inline const mf_Aircraft* mf_Player::GetAircraft() const
{
	return &aircraft_;
}

inline mf_Aircraft* mf_Player::GetAircraft()
{
	return &aircraft_;
}

inline unsigned int mf_Player::Score() const
{
	return score_;
}

inline void mf_Player::AddScorePoints( unsigned int points )
{
	score_+= points;
}

inline void mf_Player::ResetScore()
{
	score_= 0;
}

inline const mf_Aircraft* const* mf_Player::GetEnemiesAircrafts() const
{
	return enemies_aircrafts_;
}

inline unsigned int mf_Player::EnemiesAircraftsCount() const
{
	return enemies_count_;
}

inline const float* mf_Player::Angle() const
{
	return angle_;
}

inline const float mf_Player::Fov() const
{
	return fov_;
}

inline void mf_Player::SetControlMode( ControlMode mode )
{
	control_mode_= mode;
}

inline mf_Player::ControlMode mf_Player::GetControlMode() const
{
	return control_mode_;
}

inline void mf_Player::ToggleViewMode()
{
	if( view_mode_ == ViewThirdperson ) view_mode_= ViewInsideCockpit;
	else view_mode_= ViewThirdperson;
}

inline void mf_Player::SetViewMode( ViewMode mode )
{
	view_mode_= mode;
}

inline mf_Player::ViewMode mf_Player::GetViewMode() const
{
	return view_mode_;
}

inline void mf_Player::SetPos( float x, float y, float z )
{
	pos_[0]= x; pos_[1]= y; pos_[2]= z;
}

inline void mf_Player::SetTargetAircraft( mf_Aircraft* aircraft )
{
	target_aircraft_= aircraft;
}

inline const mf_Aircraft* mf_Player::TargetAircraft() const
{
	return target_aircraft_;
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

inline void mf_Player::RotateClockwisePressed()
{
	rotate_clockwise_pressed_= true;
}

inline void mf_Player::RotateAnticlockwisePressed()
{
	rotate_anticlockwise_pressed_= true;
}

inline void mf_Player::RotateClockwiseReleased()
{
	rotate_clockwise_pressed_= false;
}

inline void mf_Player::RotateAnticlockwiseReleased()
{
	rotate_anticlockwise_pressed_= false;
}