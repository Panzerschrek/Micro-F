#pragma once
#include "micro-f.h"

#include "renderer.h"

#include "text.h"

class mf_Player;
class mf_GameLogic;

class mf_SoundEngine;
class mf_SoundSource;
class mf_MusicEngine;
class mf_Gui;

class mf_MainLoop
{
public:
	static void CreateInstance(
		unsigned int viewport_width, unsigned int viewport_height,
		bool fullscreen, bool vsync,
		bool invert_mouse_y,
		float mouse_speed_x, float mouse_speed_y );
	static mf_MainLoop* Instance();
	static void DeleteInstance();

	float CurrentTime() const;
	unsigned int FPS() const;
	void Loop();
	unsigned int ViewportWidth();
	unsigned int ViewportHeight();

	void Play( const mf_Settings* settings );
	void Quit();

private:
	mf_MainLoop(
		unsigned int viewport_width, unsigned int viewport_height,
		bool fullscreen, bool vsync,
		bool invert_mouse_y,
		float mouse_speed_x, float mouse_speed_y );
	~mf_MainLoop();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Resize();
	void FocusChange( bool focus_in );
	void CaptureMouse();
	void Shot( unsigned int x, unsigned int y );

	void CalculateFPS();

private:

	unsigned int viewport_width_, viewport_height_;
	unsigned int min_viewport_width_, min_viewport_height_;
	float mouse_speed_x_, mouse_speed_y_;
	bool fullscreen_;
	bool vsync_;
	bool quit_;

	HWND hwnd_;
	HDC hdc_;
	HGLRC hrc_;
	WNDCLASSEX window_class_;

	POINT prev_cursor_pos_;
	bool mouse_captured_;

	mf_Text* text_;

	mf_Player player_;
	mf_GameLogic* game_logic_;
	mf_Renderer* renderer_;
	mf_SoundEngine* sound_engine_;
	mf_MusicEngine* music_engine_;
	mf_Gui* gui_;

	unsigned int prev_game_tick_;
	float prev_tick_dt_;
	float game_time_;

	struct
	{
		unsigned int prev_calc_time;
		unsigned int frame_count_to_show;
		unsigned int current_calc_frame_count;
	}fps_calc_;

	mf_SoundSource* test_sound_;

	enum
	{
		ModeMainMenu,
		ModeSettingsMenu,
		ModeGame,
		ModeIngameMenu,
		ModeGameOver,
		ModeGameEnd,
		ModeCredits

	} mode_;

};

inline float mf_MainLoop::CurrentTime() const
{
	return game_time_;
}

inline unsigned int mf_MainLoop::FPS() const
{
	return fps_calc_.frame_count_to_show;
}

inline unsigned int mf_MainLoop::ViewportWidth()
{
	return viewport_width_;
}

inline unsigned int mf_MainLoop::ViewportHeight()
{
	return viewport_height_;
}

inline void mf_MainLoop::Quit()
{
	quit_= true;
}