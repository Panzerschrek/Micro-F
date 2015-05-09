#include <new.h>

#include "micro-f.h"
#include "main_loop.h"
#include "sound_engine.h"
#include "music_engine.h"
#include "game_logic.h"
#include "gui.h"
#include "settings.h"

#include "mf_math.h"

#define WINDOW_CLASS "Micro-F"
#define WINDOW_NAME "Micro-F"
#define OGL_VERSION_MAJOR 3
#define OGL_VERSION_MINOR 3
#define KEY(x) (65 + x - 'A' )

void* mfGetGLFuncAddress( const char* addr )
{
	return wglGetProcAddress( addr );
}

mf_MainLoop* main_loop_instance= NULL;
char* main_loop_instance_data= NULL;

void mf_MainLoop::CreateInstance(
	unsigned int viewport_width, unsigned int viewport_height,
	bool fullscreen, bool vsync,
	bool invert_mouse_y,
	float mouse_speed_x, float mouse_speed_y )
{
	main_loop_instance_data= new char[ sizeof(mf_MainLoop) ];
	main_loop_instance= (mf_MainLoop*)main_loop_instance_data;

	new (main_loop_instance_data) mf_MainLoop(
		viewport_width, viewport_height,
		fullscreen, vsync,
		invert_mouse_y,
		mouse_speed_x, mouse_speed_y );
}

mf_MainLoop* mf_MainLoop::Instance()
{
	return main_loop_instance;
}

void mf_MainLoop::DeleteInstance()
{
	if( main_loop_instance != NULL )
	{
		main_loop_instance->~mf_MainLoop();
		delete[] main_loop_instance_data;
		main_loop_instance= NULL;
		main_loop_instance_data= NULL;
	}
}

void mf_MainLoop::Loop()
{
	while(!quit_)
	{
		MSG msg;
		while ( PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		text_->SetViewport( viewport_width_, viewport_height_ );

		if( mode_ == ModeGame || mode_ == ModeIngameMenu )
		{
			unsigned int tick= clock();
			if( prev_game_tick_ == 0 ) prev_game_tick_= tick;
			prev_tick_dt_= float( tick - prev_game_tick_ ) / float(CLOCKS_PER_SEC );
			if( prev_tick_dt_ > 0.001f )
			{
				prev_game_tick_= tick;

				unsigned int cursor_xy[2]= { viewport_width_ / 2, viewport_height_ / 2 };
				if( mouse_captured_ )
				{
					POINT new_cursor_pos;
					GetCursorPos(&new_cursor_pos);
					player_.RotateZ( float( prev_cursor_pos_.x - new_cursor_pos.x ) * mouse_speed_x_ );
					player_.RotateX( float( prev_cursor_pos_.y - new_cursor_pos.y ) * mouse_speed_y_ );
					SetCursorPos( prev_cursor_pos_.x, prev_cursor_pos_.y );
				}
				else if( player_.GetViewMode() == mf_Player::ViewInsideCockpit )
				{
					POINT new_cursor_pos;
					GetCursorPos( &new_cursor_pos );
					MapWindowPoints( 0, hwnd_, &new_cursor_pos, 1 );

					int dx= new_cursor_pos.x - viewport_width_  / 2;
					int dy= new_cursor_pos.y - viewport_height_ / 2;
					float r2= float( dx * dx + dy * dy );
					float max_r= player_.GetMachinegunCircleRadius();
					if( r2 > max_r * max_r )
					{
						float k= max_r / mf_Math::sqrt(r2);
						new_cursor_pos.x= int( float(dx) * k ) + viewport_width_  / 2;
						new_cursor_pos.y= int( float(dy) * k ) + viewport_height_ / 2;

						POINT global_point= new_cursor_pos;
						MapWindowPoints( hwnd_, 0, &global_point, 1 );
						SetCursorPos( global_point.x, global_point.y );
					}
					cursor_xy[0]= new_cursor_pos.x;
					cursor_xy[1]= new_cursor_pos.y;
					gui_->SetCursor( new_cursor_pos.x, new_cursor_pos.y );
				}

				player_.Tick(prev_tick_dt_);
				game_time_= float(prev_game_tick_) / float(CLOCKS_PER_SEC);

				if( shot_button_pressed_ )
				{
					float dir[3];
					player_.ScreenPointToWorldSpaceVec( cursor_xy[0], cursor_xy[1], dir );
					game_logic_->ShotContinue( player_.GetAircraft(), dir );
				}
				game_logic_->Tick( prev_tick_dt_ );
			}// if normal dt

			// sound setup
			sound_engine_->SetListenerOrinetation( player_.Pos(), player_.Angle(), player_.GetAircraft()->Velocity() );
			sound_engine_->Tick();

			renderer_->DrawFrame();
		} // if in game or ingame menu
		else if( mode_ == ModeChooseAircraft )
		{
			player_.Tick( 0.0f );
			renderer_->DrawFrame();
		}

		{
			char str[32];
			sprintf( str, "fps: %d", fps_calc_.frame_count_to_show );
			text_->AddText( 0, 0, 1, mf_Text::default_color, str );
			sprintf( str, "fov: %3.1f", player_.Fov() * MF_RAD2DEG );
			text_->AddText( 0, 1, 1, mf_Text::default_color, str );
		}

		if( mouse_captured_ )
			gui_->SetCursor( viewport_width_ / 2, viewport_height_ / 2 );
		gui_->Draw();
		text_->Draw();

		SwapBuffers( hdc_ );
		CalculateFPS();
	} // while !quit
}

void mf_MainLoop::DrawLoadingFrame( const char* text )
{
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClear( GL_COLOR_BUFFER_BIT );

	static const char* const c_loading_text= "---LOADING---";

	unsigned int center_column= viewport_width_ / ( 2 * MF_LETTER_WIDTH );
	unsigned int center_row= viewport_height_ / ( 2 * MF_LETTER_HEIGHT ) + 1;

	text_->AddText(
		center_column - strlen(c_loading_text),
		center_row - 3,
		2, mf_Text::default_color, c_loading_text );
	text_->AddText(
		center_column - strlen(text),
		center_row,
		2, mf_Text::default_color, text );

	text_->Draw();
	SwapBuffers( hdc_ );
}

void mf_MainLoop::Play( const mf_Settings* settings )
{
	game_logic_= new mf_GameLogic( &player_, settings->seed );

	mode_= ModeChooseAircraft;
	renderer_= new mf_Renderer( &player_, game_logic_, text_, settings );

	mf_Aircraft* aircraft= player_.GetAircraft();
	float pos[3];
	pos[0]= game_logic_->GetLevel()->TerrainSizeX() * game_logic_->GetLevel()->TerrainCellSize() * 0.5f;
	pos[1]= 128.0f;
	pos[2]= game_logic_->GetLevel()->TerrainAmplitude();
	aircraft->SetPos( pos );
}

void mf_MainLoop::StartGame()
{
	player_.ChoseAircraft();
	game_logic_->StartGame();
	music_engine_->Stop();
	mode_= ModeGame;
}

mf_MainLoop::mf_MainLoop(
	unsigned int viewport_width, unsigned int viewport_height,
	bool fullscreen, bool vsync,
	bool invert_mouse_y,
	float mouse_speed_x, float mouse_speed_y )
	: viewport_width_(viewport_width), viewport_height_(viewport_height)
	, min_viewport_width_(800), min_viewport_height_(600)
	, mouse_speed_x_( mouse_speed_x )
	, mouse_speed_y_( mouse_speed_y * (invert_mouse_y ? -1.0f :1.0f ) )
	, fullscreen_(fullscreen)
	, vsync_(vsync)
	, quit_(false)
	, prev_cursor_pos_(), mouse_captured_(false), shot_button_pressed_(false)
	, game_logic_(NULL), renderer_(NULL), sound_engine_(NULL), music_engine_(NULL), gui_(NULL)
	, mode_(ModeMainMenu)
{
	int border_size, top_border_size, bottom_border_size;

	window_class_.cbSize= sizeof(WNDCLASSEX);
	window_class_.style= CS_OWNDC;
	window_class_.lpfnWndProc= WindowProc;
	window_class_.cbClsExtra= 0;
	window_class_.cbWndExtra= 0;
	window_class_.hInstance= 0;
	window_class_.hIcon= LoadIcon( 0 , IDI_APPLICATION );
	window_class_.hCursor= LoadCursor( NULL, IDC_ARROW );
	window_class_.hbrBackground= (HBRUSH) GetStockObject(BLACK_BRUSH);
	window_class_.lpszMenuName= NULL;
	window_class_.lpszClassName= WINDOW_CLASS;
	window_class_.hIconSm= LoadIcon( NULL, IDI_APPLICATION );

	RegisterClassEx( &window_class_ );

	border_size= GetSystemMetrics(SM_CXFIXEDFRAME);
	bottom_border_size= GetSystemMetrics(SM_CYFIXEDFRAME);
	top_border_size= bottom_border_size + GetSystemMetrics(SM_CYCAPTION);

	unsigned int window_width= viewport_width_ + border_size * 2 + 2;
	unsigned int window_height= viewport_height_ + top_border_size + bottom_border_size + 2;
	if( fullscreen_ )
	{
		viewport_width_= window_width= GetSystemMetrics(SM_CXSCREEN);
		viewport_height_= window_height= GetSystemMetrics(SM_CYSCREEN);
	}

	hwnd_= CreateWindowEx( 0, WINDOW_CLASS, WINDOW_NAME,
		fullscreen_ ? WS_POPUP|WS_SYSMENU : WS_OVERLAPPEDWINDOW,
		0, 0,
		window_width,
		window_height,
		NULL, NULL, 0, NULL);

	ShowWindow( hwnd_, SW_SHOWNORMAL );
	hdc_= GetDC( hwnd_ );

	wglMakeCurrent( 0, 0 );
	PIXELFORMATDESCRIPTOR pfd;

	ZeroMemory( &pfd, sizeof(pfd) );
	pfd.nVersion= 1;
	pfd.dwFlags= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType= PFD_TYPE_RGBA;
	pfd.cColorBits= 32;
	pfd.cDepthBits= 32;
	pfd.cStencilBits= 8;
	pfd.iLayerType= PFD_MAIN_PLANE;

	int format= ChoosePixelFormat( hdc_, &pfd );
	SetPixelFormat( hdc_, format, &pfd );

	hrc_= wglCreateContext( hdc_ );
	wglMakeCurrent( hdc_, hrc_ );

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB= NULL;
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hrc_ );

	static const int attribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, OGL_VERSION_MAJOR,
		WGL_CONTEXT_MINOR_VERSION_ARB, OGL_VERSION_MINOR,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	hrc_= wglCreateContextAttribsARB( hdc_, 0, attribs );
	wglMakeCurrent( hdc_, hrc_ );

	PFNWGLSWAPINTERVALEXTPROC wglSwapInterval= (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress( "wglSwapIntervalEXT" );
	if( wglSwapInterval != NULL )
		wglSwapInterval( vsync_ ? 1 : 0 );

	GetGLFunctions( mfGetGLFuncAddress );

	sound_engine_= new mf_SoundEngine(hwnd_);
	music_engine_= new mf_MusicEngine();
	music_engine_->Play( mf_MusicEngine::MelodyAviatorsMarch );

	text_= new mf_Text();
	gui_ = new mf_Gui( text_, &player_ );


	// Intial GL state
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glEnable( GL_TEXTURE_CUBE_MAP_SEAMLESS );

#ifdef MF_DEBUG
	{
		MF_DEBUG_INFO_STR( "------------OpenGL limitations------------" );
		int max_unifroms= 0;
		glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_unifroms );
		MF_DEBUG_INFO_STR_I( "max uniform components in vertex shader: ", max_unifroms );

		int uniform_block_size;
		glGetIntegerv( GL_MAX_UNIFORM_BLOCK_SIZE, &uniform_block_size );
		MF_DEBUG_INFO_STR_I( "max uniform block size: ", uniform_block_size );

		int max_vertex_shader_uniform_blocks;
		glGetIntegerv( GL_MAX_VERTEX_UNIFORM_BLOCKS, &max_vertex_shader_uniform_blocks );
		MF_DEBUG_INFO_STR_I( "max vertex shader uniform blocks: ", max_vertex_shader_uniform_blocks );
		MF_DEBUG_INFO_STR( "------------------------------------------" );
	}
#endif

	prev_game_tick_= 0;
	game_time_= float(prev_game_tick_) / float(CLOCKS_PER_SEC);

	fps_calc_.prev_calc_time= clock();
	fps_calc_.frame_count_to_show= 0;
	fps_calc_.current_calc_frame_count= 0;
}

mf_MainLoop::~mf_MainLoop()
{
	delete renderer_;
	delete sound_engine_;
	delete game_logic_;

	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hrc_ );

	ReleaseDC( hwnd_, hdc_ );
	DestroyWindow(hwnd_);

	UnregisterClass( WINDOW_CLASS, 0 );
}

LRESULT CALLBACK mf_MainLoop::WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	mf_MainLoop* instance= Instance();

	switch(uMsg)
	{
	case WM_CLOSE:
	case WM_QUIT:
		instance->quit_= true;
		break;
	case WM_SIZE:
		instance->Resize();
		break;
	case WM_KILLFOCUS:
		instance->FocusChange( HWND(wParam) == instance->hwnd_ );
		break;
	case WM_LBUTTONDOWN:
		instance->Shot( lParam&65535, lParam>>16, 0 );
		instance->shot_button_pressed_= true;
		break;
	case WM_RBUTTONDOWN:
		instance->Shot( lParam&65535, lParam>>16, 1 );
		break;
	case WM_MBUTTONDOWN:
		instance->Shot( lParam&65535, lParam>>16, 2 );
		break;
	case WM_LBUTTONUP:
		instance->gui_->MouseClick( lParam&65535, lParam>>16 );
		instance->shot_button_pressed_= false;
		break;
	case WM_MOUSEMOVE:
		instance->gui_->MouseHover( lParam&65535, lParam>>16 );
		instance->gui_->SetCursor( lParam&65535, lParam>>16 );
		break;
	case WM_MOUSEWHEEL:
		{
			int step= GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			if( step > 0 )
				for( int i= 0; i< step; i++ )
					instance->player_.ZoomIn();
			else
				for( int i= 0; i< abs(step); i++ )
					instance->player_.ZoomOut();
		}
		break;

	case WM_KEYUP:
		switch(wParam)
		{
		case KEY('M'):
			instance->CaptureMouse();
			break;
		case KEY('W'):
			instance->player_.ForwardReleased(); break;
		case KEY('S'):
			instance->player_.BackwardReleased(); break;
		case KEY('A'):
			instance->player_.LeftReleased(); break;
		case KEY('D'):
			instance->player_.RightReleased(); break;
		case VK_SHIFT:
			instance->player_.UpReleased(); break;
		case VK_CONTROL:
			instance->player_.DownReleased(); break;
		case VK_LEFT:
			instance->player_.RotateLeftReleased(); break;
		case VK_RIGHT:
			instance->player_.RotateRightReleased(); break;
		case VK_UP:
			instance->player_.RotateUpReleased(); break;
		case VK_DOWN:
			instance->player_.RotateDownReleased(); break;
		case KEY('Q'):
			instance->player_.RotateClockwiseReleased(); break;
		case KEY('E'):
			instance->player_.RotateAnticlockwiseReleased(); break;
		case VK_F1:
			instance->player_.ToggleViewMode(); break;
		default:
			break;
		}
		break; // WM_KEYUP

	case WM_KEYDOWN:
		switch(wParam)
		{
		case KEY('W'):
			instance->player_.ForwardPressed(); break;
		case KEY('S'):
			instance->player_.BackwardPressed(); break;
		case KEY('A'):
			instance->player_.LeftPressed(); break;
		case KEY('D'):
			instance->player_.RightPressed(); break;
		case VK_SHIFT:
			instance->player_.UpPressed(); break;
		case VK_CONTROL:
			instance->player_.DownPressed(); break;
		case VK_LEFT:
			instance->player_.RotateLeftPressed(); break;
		case VK_RIGHT:
			instance->player_.RotateRightPressed(); break;
		case VK_UP:
			instance->player_.RotateUpPressed(); break;
		case VK_DOWN:
			instance->player_.RotateDownPressed(); break;
		case KEY('Q'):
			instance->player_.RotateClockwisePressed(); break;
		case KEY('E'):
			instance->player_.RotateAnticlockwisePressed(); break;

		default:
			break;
		};
		break; // WM_KEYDOWN

	default:
		break;
	}; // switch(uMsg)

	return DefWindowProc( hwnd, uMsg, wParam, lParam );
}

void mf_MainLoop::Resize()
{
	if(fullscreen_)
	{
		//MoveWindow( hwnd_, 0, 0, viewport_width_, viewport_height_, true );
		return;
	}

	RECT rect;
	GetClientRect( hwnd_, &rect );
	unsigned int new_width= rect.right;
	unsigned int new_height= rect.bottom;

	if( new_width != viewport_width_ || new_height != viewport_height_ )
	{
		if( new_width < min_viewport_width_ || new_height < min_viewport_height_ )
		{
			RECT window_rect;
			GetWindowRect( hwnd_, &window_rect );

			unsigned int window_width= window_rect.right - window_rect.left;
			if( new_width < min_viewport_width_ )
			{
				window_width+= min_viewport_width_ - new_width;
				viewport_width_= min_viewport_width_;
			}
			unsigned int window_height= window_rect.bottom - window_rect.top;
			if( new_height < min_viewport_height_ )
			{
				window_height+= min_viewport_height_ - new_height;
				viewport_height_= min_viewport_height_;
			}

			MoveWindow( hwnd_, window_rect.left, window_rect.top, window_width, window_height, true );
		}
		else
		{
			viewport_width_= new_width;
			viewport_height_= new_height;
		}
		glViewport( 0, 0, viewport_width_, viewport_height_ );
		if(renderer_)
			renderer_->Resize();
		if(gui_)
			gui_->Resize();
	}
}

void mf_MainLoop::FocusChange( bool focus_in )
{
	if( !focus_in )
	{
		if( mouse_captured_ )
			CaptureMouse();
	}
}

void mf_MainLoop::CaptureMouse()
{
	if(mouse_captured_)
	{
		ShowCursor(true);
		mouse_captured_= false;
	}
	else
	{
		ShowCursor(false);
		mouse_captured_= true;
		GetCursorPos( &prev_cursor_pos_ );
	}
}

void mf_MainLoop::Shot( unsigned int x, unsigned int y, unsigned int button )
{
	if( game_logic_ != NULL && game_logic_->GameStarted() && !player_.IsInRespawn() )
	{
		if( button == 0 )
		{
			float dir[3];
			player_.ScreenPointToWorldSpaceVec( x, y, dir );
			game_logic_->ShotBegin( player_.GetAircraft() );
			game_logic_->ShotContinue( player_.GetAircraft(), dir, true );
		}
		else
		{
			float dir[3];
			player_.ScreenPointToWorldSpaceVec( x, y, dir );
			game_logic_->PlayerRocketShot( dir );
		}
	}
}

void mf_MainLoop::CalculateFPS()
{
	const unsigned int fps_calc_interval_ticks= CLOCKS_PER_SEC * 3 / 4;

	fps_calc_.current_calc_frame_count++;

	unsigned int current_time= clock();
	unsigned int dt= current_time - fps_calc_.prev_calc_time;
	if( dt >= fps_calc_interval_ticks )
	{
		fps_calc_.frame_count_to_show= fps_calc_.current_calc_frame_count * CLOCKS_PER_SEC / fps_calc_interval_ticks;
		fps_calc_.current_calc_frame_count= 0;

		fps_calc_.prev_calc_time= current_time;
	}
}