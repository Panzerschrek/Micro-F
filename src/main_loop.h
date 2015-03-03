#pragma once
#include "micro-f.h"

#include "player.h"
#include "level.h"
#include "renderer.h"

#include "glsl_program.h"
#include "text.h"
#include "vertex_buffer.h"

class mf_SoundEngine;
class mf_SoundSource;

struct mf_RenderingConfig
{
	bool fullscreen;
	bool vsync;
	bool invert_mouse_y;

	float mouse_speed_x;
	float mouse_speed_y;
};


class mf_MainLoop
{
public:
	static void SetRenderingConfig( const mf_RenderingConfig* config );
	static mf_MainLoop* Instance();
	static void DeleteInstance();

	void Loop();
	unsigned int ViewportWidth();
	unsigned int ViewportHeight();

private:
	mf_MainLoop();
	~mf_MainLoop();

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Resize();
	void FocusChange( bool focus_in );
	void CaptureMouse();

	void CalculateFPS();

private:
	static mf_RenderingConfig rendering_config_;

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
	mf_Level level_;
	mf_Renderer* renderer_;
	mf_SoundEngine* sound_engine_;

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
};

inline unsigned int mf_MainLoop::ViewportWidth()
{
	return viewport_width_;
}

inline unsigned int mf_MainLoop::ViewportHeight()
{
	return viewport_height_;
}