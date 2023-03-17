#include <platform.h>
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <time.h>
#include <config.h>
#include <vector3.h>


platform_t* platform_init() {

    platform_t* pf = malloc(sizeof(platform_t));

    // TODO @CLEANUP: Feels weird to only define a Platform variable doing this sort of thing
    srand(time(NULL));
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    int window_x = (1920 / 2) - (window_width / 2);
    int window_y = (1080 / 2) - (window_height / 2);

    pf->window = SDL_CreateWindow("Frag3D", window_x, window_y, window_width, window_height, SDL_WINDOW_OPENGL | SDL_WINDOW_MINIMIZED);
	pf->context = SDL_GL_CreateContext(pf->window);

    for(int i = 0; i < NB_KEYCODE; i++){ 
        pf->prev_events[i] = pf->current_events[i] = 0;
    }

    pf->running = 1;
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide cursor

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    pf->prev_mouse_x = mouse_x;
    pf->prev_mouse_y = mouse_y;
    pf->mouse_dx = 0;
    pf->mouse_dy = 0;

    
    return pf;
}
void platform_freealloc(platform_t* pf){
    SDL_GL_DeleteContext(pf->context);
    SDL_DestroyWindow(pf->window);
	SDL_Quit();
}

void platform_destroy(platform_t** pf){
    platform_freealloc(*pf);
	free(*pf);
    *pf = NULL;
}


int platform_get_key(platform_t* pf, keycode_t key_code){
    return pf->current_events[key_code];
}

int platform_get_key_down(platform_t* pf, keycode_t key_code){
    return pf->current_events[key_code]
        && !pf->prev_events[key_code];
}

int platform_get_key_up(platform_t* pf, keycode_t key_code){
    return !pf->current_events[key_code]
        && pf->prev_events[key_code];
}

vector2_t platform_get_mouse_delta(platform_t* pf){
    return vector2_init(pf->mouse_dx, pf->mouse_dy);
}

void platform_read_input(platform_t* pf){

    keycode_t key = k_Null;

    for(int i = 0; i < NB_KEYCODE; i++){ // current copié dans prev et réinitialisé
        pf->prev_events[i] = pf->current_events[i];
        pf->current_events[i] = 0;
    }

    while (SDL_PollEvent(&pf->event)){   
        
        if ((pf->event.type == SDL_QUIT) || (pf->event.type == SDL_KEYUP &&
                pf->event.key.keysym.sym == SDLK_ESCAPE)) {
            pf->running = 0;
        }
        else if(pf->event.type == SDL_KEYDOWN || pf->event.type == SDL_KEYUP){ // TODO 
            switch (pf->event.key.keysym.sym){ 
                case SDLK_z : // TODO faire un fichier de config qui fait varier chaque 'case' selon les touches choisis
                    key = k_Forward; break;
                case SDLK_q : 
                    key = k_Left; break;
                case SDLK_s : 
                    key = k_Back; break;
                case SDLK_d : 
                    key = k_Right; break;
                case SDLK_SPACE : 
                    key = k_Up; 
                    key = k_Jump; break;
                case SDLK_LCTRL : 
                    key = k_Down; 
                    key = k_Crounch; break;
                case SDLK_k : 
                    key = k_ToggleFly; break;
                default : break;           
            }
            if (pf->event.type == SDL_KEYDOWN) pf->current_events[key] = 1;
            else pf->current_events[key] = 0;
        }
        else if (pf->event.type == SDL_MOUSEBUTTONDOWN || pf->event.type == SDL_MOUSEBUTTONUP){
            switch (pf->event.button.button){
                case SDL_BUTTON_LEFT:
                        key = k_Shoot; break;
                case SDL_BUTTON_RIGHT:
                        key = k_Zoom; break;
                case SDL_BUTTON_MIDDLE:
                        key = k_Killself; break;
                default: break;
            }
            if (pf->event.type == SDL_MOUSEBUTTONDOWN) pf->current_events[key] = 1;
            else pf->current_events[key] = 0;
        }
    }

    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    pf->mouse_dx = mouse_x - pf->prev_mouse_x;
    pf->mouse_dy = mouse_y - pf->prev_mouse_y;
    pf->prev_mouse_x = mouse_x;
    pf->prev_mouse_y = mouse_y;
}

float platform_get_time() {
    return (float)SDL_GetTicks();
}

int platform_should_window_close(platform_t* pf){
    return !pf->running;
}

void platform_end_frame(platform_t* pf){
    SDL_GL_SwapWindow(pf->window);
    
}