/*#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <math.h>
#include <common.h>

//coucou
//test 2

int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_Window* window = SDL_CreateWindow("OpenGL", 100, 100, 800, 800, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();

	SDL_Event event;
	while (1)
	{
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT);

		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) break;
			if (event.type == SDL_KEYUP &&
			event.key.keysym.sym == SDLK_ESCAPE) break;
		}

		SDL_GL_SwapWindow(window);
	}
	
	SDL_GL_DeleteContext(context);
	SDL_Quit(); 
	return 0;
}
*/

#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <render.h>
#include <assets.h>
#include <platform.h>
#include <world.h>

// TODO @NEXT: Player start pos
// TODO @NEXT: Shoot footage

int main(int argc, char *argv[]){

    // TODO @CLEANUP: To make it run from Unity
    //_chdir(R"(c:\users\atil\code\hellc\)");

    platform_t* platform = platform_init();
    renderer_t* renderer = renderer_init();
    world_t* world = world_init();

    scene_t* scene = scene_read_scene("assets/test_export.txt");

    world_register_scene(world, scene);
    renderer_register_scene(renderer, scene);

    float prev_time = platform_get_time();
    while (!platform_should_window_close(platform)) {
        const float now_time = platform_get_time();
        const float dt = now_time - prev_time;
        prev_time = now_time;

        platform_read_input(platform);
        world_player_tick(world, platform, dt);
        renderer_render(renderer, world_get_view_matrix(world), dt);

        platform_end_frame(platform);
    }


	scene_destroy(&scene);
	world_destroy(&world);
	renderer_destroy(&renderer);
	platform_destroy(&platform);
    return 0;
}