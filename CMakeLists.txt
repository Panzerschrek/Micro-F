cmake_minimum_required(VERSION 2.8)

project(Micro-F)

set( HEADERS
	src/aircraft.h
	src/autopilot.h
	src/drawing_model.h
	src/enemy.h
	src/game_logic.h
	src/glsl_program.h
	src/gui.h
	src/level.h
	src/main_loop.h
	src/mf_math.h
	src/mf_model.h
	src/micro-f.h
	src/models_generation.h
	src/music_engine.h
	src/particles_manager.h
	src/player.h
	src/renderer.h
	src/settings.h
	src/shaders.h
	src/sound_engine_sdl.h
	src/sounds_generation.h
	src/text.h
	src/texture.h
)

set( SOURCES
	src/aircraft.cpp
	src/autopilot.cpp
	src/drawing_model.cpp
	src/enemy.cpp
	src/game_logic.cpp
	src/glsl_program.cpp
	src/gui.cpp
	src/level.cpp
	src/main.cpp
	src/main_loop.cpp
	src/mf_math.cpp
	src/models_generation.cpp
	src/music_engine.cpp
	src/particles_manager.cpp
	src/player.cpp
	src/renderer.cpp
	src/shaders.cpp
	src/sound_engine_sdl.cpp
	src/sounds_generation.cpp
	src/text.cpp
	src/texture.cpp
	src/textures_generation.cpp
	src/vertex_buffer.cpp
)

add_executable( MicroF ${SOURCES} ${HEADERS} )

if( WIN32 )
	target_compile_definitions( MicroF PRIVATE MF_PLATFORM_WIN )
else()
	target_compile_definitions( MicroF PRIVATE MF_PLATFORM_SDL )
endif()
