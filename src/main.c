#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <render.h>
#include <assets.h>
#include <platform.h>
#include <world.h>
#include <unistd.h>
#include <config.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>



static platform_t* platform;
static renderer_t* renderer;
static world_t* world;
static scene_t* scene;

void platform_exit(){
	platform_destroy(&platform);
}
void renderer_exit(){
	renderer_destroy(&renderer);
}
void world_exit(){
	world_destroy(&world);
}
void scene_exit(){
	scene_destroy(&scene);
}




void jeu(){

    platform = platform_init(); 
    atexit(platform_exit);

    renderer = renderer_init();
    atexit(renderer_exit);

    world = world_init();
    atexit(world_exit);

    scene = scene_read_scene("assets/test_frag3d.txt");
    world_register_scene(world, scene);

    renderer_register_scene(renderer, scene);
    scene_exit();

    float prev_time = platform_get_time();
    world->fly_move_enabled = 1;

    while (!platform_should_window_close(platform)) {
    const float now_time = platform_get_time();
    const float dt = now_time - prev_time;
    prev_time = now_time;

    platform_read_input(platform);

    world_player_tick(world, platform, dt);
    renderer_render(renderer, world_get_view_matrix(world), dt);

    platform_end_frame(platform);
}

 exit(EXIT_SUCCESS);

}


int main(int argc, char *argv[]){
	
    // Initialisation de la bibliothèque SDL_ttf
    TTF_Init();

    // Chargement de la police de caractères
    TTF_Font* font = TTF_OpenFont("assets/menu/contrast.ttf", 32);
	TTF_Font* font2 = TTF_OpenFont("assets/menu/contrast2.ttf", 32);
    if (font == NULL || font2 == NULL) {
        fprintf(stderr, "Erreur : impossible de charger la police de caractères\n");
        return 1;
    }

	/*
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    fprintf(stderr, "Erreur : impossible d'initialiser SDL_mixer\n");
    return 1;
	}
	Mix_Music* music = Mix_LoadMUS("assets/audio/audio.mp3");
	if (music == NULL) {
    fprintf(stderr, "Erreur : impossible de charger le fichier audio\n");
    return 1;
	}
	Mix_PlayMusic(music, -1);
	Mix_FreeMusic(music);
	Mix_CloseAudio();

	*/

    // Initialisation de la SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Menu du jeu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN);
    // Création du renderer du menu
    SDL_Renderer* menuRenderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Color textColor = { 255, 255, 255, 255 };
	SDL_Color textColor2 = {  255, 195, 0, 0 };
	SDL_Color textColor3 = {  255, 50, 50, 0 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font2, "Frag 3D", textColor2);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(menuRenderer, textSurface);
    SDL_Rect textRect = { (window_width - textSurface->w) / 2, (window_height - textSurface->h) / 2 - 100, textSurface->w, textSurface->h };

    SDL_Surface* textSurface1 = TTF_RenderText_Solid(font, "Jouer", textColor);
    SDL_Texture* textTexture1 = SDL_CreateTextureFromSurface(menuRenderer, textSurface1);
    SDL_Rect textRect1 = { (window_width - textSurface1->w) / 2, (window_height - textSurface1->h) / 2 - 50, textSurface1->w, textSurface1->h };

    SDL_Surface* textSurface2 = TTF_RenderText_Solid(font, "Options", textColor);
    SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(menuRenderer, textSurface2);
    SDL_Rect textRect2 = { (window_width - textSurface2->w) / 2, (window_height - textSurface2->h) / 2, textSurface2->w, textSurface2->h };

    SDL_Surface* textSurface3 = TTF_RenderText_Solid(font, "Quitter", textColor3);
    SDL_Texture* textTexture3 = SDL_CreateTextureFromSurface(menuRenderer, textSurface3);
    SDL_Rect textRect3 = { (window_width - textSurface3->w) / 2, (window_height - textSurface3->h) / 2 + 50, textSurface3->w, textSurface3->h };

    // Boucle principale du menu
    int quit = 0;
    while (!quit) {
        // Gestion des événements
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        quit = 1;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        int x = event.button.x;
                        int y = event.button.y;
                        if (x >= textRect1.x && x <= textRect1.x + textRect1.w && y >= textRect1.y && y <= textRect1.y + textRect1.h) {
							jeu();
                            // Action du bouton Jouer
                        } else if (x >= textRect2.x && x <= textRect2.x + textRect2.w && y >= textRect2.y && y <= textRect2.y + textRect2.h) {
                            //optionReglage(font, menuRenderer);
							// Action du bouton Options
                        } else if (x >= textRect3.x && x <= textRect3.x + textRect3.w && y >= textRect3.y && y <= textRect3.y + textRect3.h) {
                            quit = 1;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        // Effacement de l'écran
        SDL_SetRenderDrawColor(menuRenderer, 0, 0, 0, 255);
        SDL_RenderClear(menuRenderer);

        // Affichage des textures
        SDL_RenderCopy(menuRenderer, textTexture, NULL, NULL);
        SDL_RenderCopy(menuRenderer, textTexture1, NULL, &textRect1);
        SDL_RenderCopy(menuRenderer, textTexture2, NULL, &textRect2);
        SDL_RenderCopy(menuRenderer, textTexture3, NULL, &textRect3);

        // Mise à jour de l'écran
        SDL_RenderPresent(menuRenderer);
    }

    // Libération de la mémoire
    SDL_DestroyTexture(textTexture);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture1);
    SDL_FreeSurface(textSurface1);
    SDL_DestroyTexture(textTexture2);
    SDL_FreeSurface(textSurface2);
    SDL_DestroyTexture(textTexture3);
    SDL_FreeSurface(textSurface3);
    SDL_DestroyRenderer(menuRenderer);

    // Fermeture de la bibliothèque SDL_ttf
    TTF_Quit();

    // Fermeture de la SDL
    SDL_Quit();

    return 0;
}

