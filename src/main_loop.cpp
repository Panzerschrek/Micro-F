#include <cstring>
#include <new>

#include "micro-f.h"
#include "main_loop.h"
#ifdef MF_PLATFORM_WIN
#include "sound_engine.h"
#else
#include "sound_engine_sdl.h"
#endif
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
#ifdef MF_PLATFORM_WIN
	return wglGetProcAddress( addr );
#else
	return SDL_GL_GetProcAddress( addr );
#endif
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
	#ifdef MF_PLATFORM_WIN
		MSG msg;
		while ( PeekMessage(&msg,NULL,0,0,PM_REMOVE) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	#else
		ProcessEvents();
	#endif

		text_->SetViewport( viewport_width_, viewport_height_ );

		if( mode_ == ModeGame || mode_ == ModeIngameMenu )
		{
			unsigned int tick= clock();
			if( prev_game_tick_ == 0 ) prev_game_tick_= tick;
			prev_tick_dt_= float( tick - prev_game_tick_ ) / float(CLOCKS_PER_SEC );
			if( prev_tick_dt_ > 0.001f )
			{
				prev_game_tick_= tick;

				if( mouse_captured_ )
				{
				#ifdef MF_PLATFORM_WIN
					POINT new_cursor_pos;
					GetCursorPos(&new_cursor_pos);
					player_.RotateZ( float( prev_cursor_pos_.x - new_cursor_pos.x ) * mouse_speed_x_ );
					player_.RotateX( float( prev_cursor_pos_.y - new_cursor_pos.y ) * mouse_speed_y_ );
					SetCursorPos( prev_cursor_pos_.x, prev_cursor_pos_.y );
				#endif
				}
				player_.Tick(prev_tick_dt_);
				game_time_= float(prev_game_tick_) / float(CLOCKS_PER_SEC);

				if( shot_button_pressed_ )
				{
					float dir[3];
					player_.ScreenPointToWorldSpaceVec( viewport_width_ / 2, viewport_height_ / 2, dir );
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
		else if( mode_ == ModeGameEnd )
		{
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

#ifdef MF_PLATFORM_WIN
		SwapBuffers( hdc_ );
#else
		SDL_GL_SwapWindow(window_);
#endif

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
		center_column - std::strlen(c_loading_text),
		center_row - 3,
		2, mf_Text::default_color, c_loading_text );
	text_->AddText(
		center_column - std::strlen(text),
		center_row,
		2, mf_Text::default_color, text );

	text_->Draw();
#ifdef MF_PLATFORM_WIN
	SwapBuffers( hdc_ );
#else
	SDL_GL_SwapWindow( window_ );
#endif
}

void mf_MainLoop::Play( const mf_Settings* settings )
{
	if( game_logic_ == NULL )
	{
		game_logic_= new mf_GameLogic( &player_, settings->seed );
		renderer_= new mf_Renderer( &player_, game_logic_, text_, settings );
	}
	mode_= ModeChooseAircraft;
	mf_Aircraft* aircraft= player_.GetAircraft();
	float pos[3];
	pos[0]= game_logic_->GetLevel()->TerrainSizeX() * game_logic_->GetLevel()->TerrainCellSize() * 0.5f;
	pos[1]= MF_Y_LEVEL_BORDER;
	pos[2]= game_logic_->GetLevel()->TerrainAmplitude();
	aircraft->SetPos( pos );
	player_.SetViewMode( mf_Player::ViewThirdperson );

	prev_game_tick_= 0;
}

void mf_MainLoop::StartGame()
{
	player_.ChoseAircraft();
	game_logic_->StartGame();
	music_engine_->Stop();
	mode_= ModeGame;

	CaptureMouse( true );
}

void mf_MainLoop::Win( float game_time )
{
	gui_->Win( player_.Score(), game_time );
	RestartGame();
}

void mf_MainLoop::Loose()
{
	gui_->Loose();
	RestartGame();
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
#ifdef MF_PLATFORM_WIN
	, prev_cursor_pos_()
#endif
	, mouse_captured_(false), shot_button_pressed_(false)
	, game_logic_(NULL), renderer_(NULL), sound_engine_(NULL), music_engine_(NULL), gui_(NULL)
	, mode_(ModeMainMenu)
{
#ifdef MF_PLATFORM_WIN
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

#else
	SDL_Init( SDL_INIT_VIDEO );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, OGL_VERSION_MAJOR );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, OGL_VERSION_MINOR );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	window_=
		SDL_CreateWindow(
			WINDOW_NAME,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			viewport_width_, viewport_height_,
			SDL_WINDOW_OPENGL | ( fullscreen_ ? SDL_WINDOW_FULLSCREEN : 0 ) | SDL_WINDOW_SHOWN );

	gl_context_= SDL_GL_CreateContext( window_ );
#endif
	GetGLFunctions( mfGetGLFuncAddress );

#ifdef MF_PLATFORM_WIN
	sound_engine_= new mf_SoundEngine(hwnd_);
#else
	sound_engine_= new mf_SoundEngine();
#endif
	music_engine_= new mf_MusicEngine();
	//music_engine_->Play( mf_MusicEngine::MelodyAviatorsMarch );

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

#ifdef MF_PLATFORM_WIN
	wglMakeCurrent( NULL, NULL );
	wglDeleteContext( hrc_ );

	ReleaseDC( hwnd_, hdc_ );
	DestroyWindow(hwnd_);

	UnregisterClass( WINDOW_CLASS, 0 );
#endif
}

#ifdef MF_PLATFORM_WIN
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
	case WM_SETFOCUS:
		instance->FocusChange( HWND(wParam) != instance->hwnd_ );
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
			if( !instance->player_.IsInRespawn() && instance->game_logic_ != NULL && instance->game_logic_->GameStarted() )
				instance->player_.ToggleViewMode();
			break;
		case VK_ESCAPE:
			instance->quit_= true;
			break;
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

#else
void mf_MainLoop::ProcessEvents()
{
	mf_MainLoop* instance= Instance();

	SDL_Event event;
	while( SDL_PollEvent(&event) )
	{
		switch(event.type)
		{
		case SDL_WINDOWEVENT:
			if( event.window.event == SDL_WINDOWEVENT_CLOSE )
				instance->quit_= true;
			break;

		case SDL_QUIT:
			instance->quit_= true;
			break;

		case SDL_MOUSEBUTTONUP:
			if( event.button.button == SDL_BUTTON_LEFT )
			{
				instance->gui_->MouseClick( event.button.x, event.button.y );
				instance->shot_button_pressed_= false;
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			if( event.button.button == SDL_BUTTON_LEFT )
			{
				instance->Shot( event.button.x, event.button.y, 0 );
				instance->shot_button_pressed_= true;
			}
			else if( event.button.button == SDL_BUTTON_RIGHT )
				instance->Shot( event.button.x, event.button.y, 1 );
			else if( event.button.button == SDL_BUTTON_MIDDLE )
				instance->Shot( event.button.x, event.button.y, 2 );
			break;

		case SDL_MOUSEMOTION:
			instance->gui_->MouseHover( event.motion.x, event.motion.y );
			instance->gui_->SetCursor( event.motion.x, event.motion.y );
			if( instance->mouse_captured_ )
			{
				instance->player_.RotateZ( float( -event.motion.xrel ) * instance->mouse_speed_x_ );
				instance->player_.RotateX( float( -event.motion.yrel ) * instance->mouse_speed_y_ );
			}
			break;

		case SDL_MOUSEWHEEL:
			{
			const int step= event.wheel.y;
				if( step > 0 )
					for( int i= 0; i< step; i++ )
						instance->player_.ZoomIn();
				else
					for( int i= 0; i< abs(step); i++ )
						instance->player_.ZoomOut();
			}
			break;

		case SDL_KEYUP:
			switch(event.key.keysym.scancode)
			{
			case SDL_SCANCODE_W:
				instance->player_.ForwardReleased(); break;
			case SDL_SCANCODE_S:
				instance->player_.BackwardReleased(); break;
			case SDL_SCANCODE_A:
				instance->player_.LeftReleased(); break;
			case SDL_SCANCODE_D:
				instance->player_.RightReleased(); break;
			case SDL_SCANCODE_LSHIFT:
				instance->player_.UpReleased(); break;
			case SDL_SCANCODE_LCTRL:
				instance->player_.DownReleased(); break;
			case SDL_SCANCODE_LEFT:
				instance->player_.RotateLeftReleased(); break;
			case SDL_SCANCODE_RIGHT:
				instance->player_.RotateRightReleased(); break;
			case SDL_SCANCODE_UP:
				instance->player_.RotateUpReleased(); break;
			case SDL_SCANCODE_DOWN:
				instance->player_.RotateDownReleased(); break;
			case SDL_SCANCODE_Q:
				instance->player_.RotateClockwiseReleased(); break;
			case SDL_SCANCODE_E:
				instance->player_.RotateAnticlockwiseReleased(); break;
			case SDL_SCANCODE_F1:
				if( !instance->player_.IsInRespawn() && instance->game_logic_ != NULL && instance->game_logic_->GameStarted() )
					instance->player_.ToggleViewMode();
				break;
			case SDL_SCANCODE_ESCAPE:
				instance->quit_= true;
				break;
			default:
				break;
			};
			break;

		case SDL_KEYDOWN:
			switch(event.key.keysym.scancode)
			{
			case SDL_SCANCODE_W:
				instance->player_.ForwardPressed(); break;
			case SDL_SCANCODE_S:
				instance->player_.BackwardPressed(); break;
			case SDL_SCANCODE_A:
				instance->player_.LeftPressed(); break;
			case SDL_SCANCODE_D:
				instance->player_.RightPressed(); break;
			case SDL_SCANCODE_LSHIFT:
				instance->player_.UpPressed(); break;
			case SDL_SCANCODE_LCTRL:
				instance->player_.DownPressed(); break;
			case SDL_SCANCODE_LEFT:
				instance->player_.RotateLeftPressed(); break;
			case SDL_SCANCODE_RIGHT:
				instance->player_.RotateRightPressed(); break;
			case SDL_SCANCODE_UP:
				instance->player_.RotateUpPressed(); break;
			case SDL_SCANCODE_DOWN:
				instance->player_.RotateDownPressed(); break;
			case SDL_SCANCODE_Q:
				instance->player_.RotateClockwisePressed(); break;
			case SDL_SCANCODE_E:
				instance->player_.RotateAnticlockwisePressed(); break;
			default:
				break;
			};
			break;

		default:
			break;
		};
	} // while has events
}
#endif

void mf_MainLoop::Resize()
{
	if(fullscreen_)
	{
		//MoveWindow( hwnd_, 0, 0, viewport_width_, viewport_height_, true );
		return;
	}

#ifdef MF_PLATFORM_WIN
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
#endif
}

void mf_MainLoop::FocusChange( bool focus_in )
{
	if( !focus_in )
		CaptureMouse( false );
	else
		CaptureMouse( mode_ == ModeGame );
}

void mf_MainLoop::CaptureMouse( bool need_capture )
{
	if( need_capture == mouse_captured_) return;
#ifdef MF_PLATFORM_WIN
	if(mouse_captured_)
	{
		mouse_captured_= false;
		ShowCursor( true );
	}
	else
	{
		mouse_captured_= true;
		ShowCursor( false );
		GetCursorPos( &prev_cursor_pos_ );
	}
#else
	mouse_captured_= need_capture;
	SDL_SetRelativeMouseMode( mouse_captured_ ? SDL_TRUE : SDL_FALSE );
#endif
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

void mf_MainLoop::RestartGame()
{
	mode_= ModeGameEnd;
	game_logic_->StopGame();
	player_.AddScorePoints( - player_.Score() );
	player_.AddLifes( 3 - player_.Lifes() );
	player_.SetupInitialAircraftParams();
	player_.SetControlMode( mf_Player::ModeChooseAircraftType );

	CaptureMouse( false );
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
