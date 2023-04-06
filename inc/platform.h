#ifndef _RENDER_DEBUG_H_
#define _RENDER_DEBUG_H_


#include <SDL2/SDL.h> // TODO à checker si juste pas faire des prototype de struct comme "struct GLFWwindow;"
#include <list.h>
#include <vector3.h>

#define NB_KEYCODE 13

typedef enum keycode_e {
    k_Forward,
    k_Back,
    k_Left,
    k_Right,
    k_Up,
    k_Down,
    k_Jump,
    k_Crounch,
    k_Shoot,
    k_Zoom, //reduire le champ de vision
    k_Killself, //respawn
    k_ToggleFly,
    k_Null //pour l'initialisation
}keycode_t;

typedef struct platform_s {
    SDL_Window* window;
    SDL_GLContext context;
    SDL_Event event;
    int running;

    int prev_events[NB_KEYCODE]; //table de bool
    int current_events[NB_KEYCODE]; 
    int prev_mouse_x;
    int prev_mouse_y;
    int mouse_dx;
    int mouse_dy;
    int mouse_state;

}platform_t;

platform_t* platform_init();
void platform_freealloc(platform_t* pf);
void platform_destroy(platform_t** pf);


void platform_read_input(platform_t* pf);
int platform_get_key(platform_t* pf, keycode_t key_code);
int platform_get_key_down(platform_t* pf, keycode_t key_code);
int platform_get_key_up(platform_t* pf, keycode_t key_code);
vector2_t platform_get_mouse_delta(platform_t* pf);

float platform_get_time();
int platform_should_window_close(platform_t* pf);
void platform_end_frame(platform_t* pf);

#endif