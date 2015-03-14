#include <new.h>

#include "micro-f.h"
#include "main_loop.h"
#include "sound_engine.h"
#include "music_engine.h"
#include "gui.h"

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

		unsigned int tick= clock();
		if( prev_game_tick_ == 0 ) prev_game_tick_= tick;
		prev_tick_dt_= float( tick - prev_game_tick_ ) / float(CLOCKS_PER_SEC );
		if( prev_tick_dt_ > 0.001f )
		{
			prev_game_tick_= tick;

			if( mouse_captured_ )
			{
				POINT new_cursor_pos;
				GetCursorPos(&new_cursor_pos);
				player_.RotateZ( float( prev_cursor_pos_.x - new_cursor_pos.x ) * mouse_speed_x_ );
				player_.RotateX( float( prev_cursor_pos_.y - new_cursor_pos.y ) * mouse_speed_y_ );
				SetCursorPos( prev_cursor_pos_.x, prev_cursor_pos_.y );
			}
			player_.Tick(prev_tick_dt_);
			game_time_= float(prev_game_tick_) / float(CLOCKS_PER_SEC);
		}// if normal dt

		{ // sound setup
			const mf_Aircraft* aircraft= player_.GetAircraft();

			sound_engine_->SetListenerOrinetation( player_.Pos(), player_.Angle(), aircraft->Velocity() );
			if (test_sound_ != NULL )
			{
				extern float ThrottleToEngineSoundPitch( float throttle );
				extern float ThrottleToEngineSoundVolumeScaler( float throttle );

				test_sound_->SetOrientation( aircraft->Pos(), aircraft->Velocity() );
				test_sound_->SetVolume( 40.0f * ThrottleToEngineSoundVolumeScaler( aircraft->Throttle() ) );
				test_sound_->SetPitch( ThrottleToEngineSoundPitch( aircraft->Throttle() ) );
			}
		}

		text_->SetViewport( viewport_width_, viewport_height_ );
		renderer_->DrawFrame();

		{
			char fps_str[32];
			sprintf( fps_str, "fps: %d", fps_calc_.frame_count_to_show );
			text_->AddText( 0, 0, 1, mf_Text::default_color, fps_str );
		}

		gui_->Draw();
		text_->Draw();

		SwapBuffers( hdc_ );
		CalculateFPS();
	} // while !quit
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
	, prev_cursor_pos_(), mouse_captured_(false)
	, renderer_(NULL), sound_engine_(NULL), music_engine_(NULL), gui_(NULL)
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

	text_= new mf_Text();
	renderer_= new mf_Renderer( &player_, &level_, text_ );
	gui_ = new mf_Gui( text_, &player_ );

	/*player_.SetPos(
		float(level_.TerrainSizeX()/2) * level_.TerrainCellSize(),
		float(level_.TerrainSizeY()/2) * level_.TerrainCellSize(),
		level_.TerrainAmplitude() * 0.5f );*/
	/*player_.SetPos(
		384.0f,
		384.0f,
		level_.TerrainAmplitude() * 0.5f );*/

	// Intial GL state
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

#ifdef MF_DEBUG
	int max_unifroms= 0;
	glGetIntegerv( GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_unifroms );
	MF_DEBUG_INFO_STR_I( "max uniform components: ", max_unifroms );
#endif

	prev_game_tick_= 0;
	game_time_= float(prev_game_tick_) / float(CLOCKS_PER_SEC);

	fps_calc_.prev_calc_time= clock();
	fps_calc_.frame_count_to_show= 0;
	fps_calc_.current_calc_frame_count= 0;

	sound_engine_= new mf_SoundEngine(hwnd_);
	test_sound_= sound_engine_->CreateSoundSource( SoundTurbojetEngine );

	music_engine_= new mf_MusicEngine();
	//music_engine_->Play( mf_MusicEngine::MelodyAviatorsMarch );
}

mf_MainLoop::~mf_MainLoop()
{
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
	case WM_LBUTTONUP:
		instance->gui_->MouseClick( lParam&65535, lParam>>16 );
	case WM_MOUSEMOVE:
		instance->gui_->MouseHover( lParam&65535, lParam>>16 );
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