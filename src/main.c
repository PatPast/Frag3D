
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <stdlib.h>
#include <math.h>
#include <common.h>
#include <stdbool.h>

//coucou
//test 2
/*
// Vertex Shader source code
const char* vertexShaderSource = "#version 450 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
//Fragment Shader source code
const	 char* fragmentShaderSource = "#version 450 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"}\n\0";


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
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, 800, 800);

	// Create Vertex Shader Object and get its reference
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	// Attach Vertex Shader source to the Vertex Shader Object
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(vertexShader);

	// Create Fragment Shader Object and get its reference
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Attach Fragment Shader source to the Fragment Shader Object
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// Compile the Vertex Shader into machine code
	glCompileShader(fragmentShader);

	// Create Shader Program Object and get its reference
	GLuint shaderProgram = glCreateProgram();
	// Attach the Vertex and Fragment Shaders to the Shader Program
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	// Wrap-up/Link all the shaders together into the Shader Program
	glLinkProgram(shaderProgram);

	// Delete the now useless Vertex and Fragment Shader objects
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);



	// Vertices coordinates
	GLfloat vertices[] =
	{
		-0.5f, -0.5f * (float)sqrt(3) / 3, 0.0f, // Lower left corner
		0.5f, -0.5f * (float)sqrt(3) / 3, 0.0f, // Lower right corner
		0.0f, 0.5f * (float)sqrt(3) * 2 / 3, 0.0f, // Upper corner
		-0.5f / 2, 0.5f * (float)sqrt(3) / 6, 0.0f, // Inner left
		0.5f / 2, 0.5f * (float)sqrt(3) / 6, 0.0f, // Inner right
		0.0f, -0.5f * (float)sqrt(3) / 3, 0.0f // Inner down
	};

	// Indices for vertices order
	GLuint indices[] =
	{
		0, 3, 5, // Lower left triangle
		3, 2, 4, // Lower right triangle
		5, 4, 1 // Upper triangle
	};

	// Create reference containers for the Vartex Array Object, the Vertex Buffer Object, and the Element Buffer Object
	GLuint VAO, VBO, EBO;

	// Generate the VAO, VBO, and EBO with only 1 object each
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Make the VAO the current Vertex Array Object by binding it
	glBindVertexArray(VAO);

	// Bind the VBO specifying it's a GL_ARRAY_BUFFER
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Introduce the vertices into the VBO
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Bind the EBO specifying it's a GL_ELEMENT_ARRAY_BUFFER
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	// Introduce the indices into the EBO
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Configure the Vertex Attribute so that OpenGL knows how to read the VBO
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	// Enable the Vertex Attribute so that OpenGL knows to use it
	glEnableVertexAttribArray(0);

	// Bind both the VBO and VAO to 0 so that we don't accidentally modify the VAO and VBO we created
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	// Bind the EBO to 0 so that we don't accidentally modify it
	// MAKE SURE TO UNBIND IT AFTER UNBINDING THE VAO, as the EBO is linked in the VAO
	// This does not apply to the VBO because the VBO is already linked to the VAO during glVertexAttribPointer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	SDL_Event event;
	while (1)
	{
		// Specify the color of the background
		glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
		// Clean the back buffer and assign the new color to it
		glClear(GL_COLOR_BUFFER_BIT);

		// Tell OpenGL which Shader Program we want to use
		glUseProgram(shaderProgram);
		// Bind the VAO so OpenGL knows to use it
		glBindVertexArray(VAO);
		// Draw primitives, number of indices, datatype of indices, index of indices
		glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_INT, 0);

		if (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) break;
			if (event.type == SDL_KEYUP &&
			event.key.keysym.sym == SDLK_ESCAPE) break;
		}

		SDL_GL_SwapWindow(window);
	}
	

	// Delete all the objects we've created
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);
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
#include <unistd.h>
#include <config.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>


// TODO @NEXT: Player start pos
// TODO @NEXT: Shoot footage


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



 

 

 // TODO @CLEANUP: To make it run from Unity

 

 //chdir("../");

 //char s[256]; printf("%s\n", getcwd(s, 100));

 //TODO, faire en sorte que le chdir marche



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



 float prev_time = platform_get_time() ;

 world->fly_move_enabled = 1;

 while (!platform_should_window_close(platform)) {

 const float now_time = platform_get_time();

 const float dt = now_time - prev_time;

 prev_time = now_time;

 //if(1000.0/30 > dt) SDL_Delay(1000.0/30-dt); 



 platform_read_input(platform);

 world_player_tick(world, platform, dt);

 renderer_render(renderer, world_get_view_matrix(world), dt);




 platform_end_frame(platform);

 bool platform_was_mouse_clicked(platform_t* platform) {
	static bool was_clicked = 0;
	bool is_clicked = 0;

	if (platform->mouse_state & SDL_BUTTON(SDL_BUTTON_LEFT)) {
		if (!was_clicked) {
			is_clicked = 1;
			}
		was_clicked = 1;
		} else {
			was_clicked = 0;
	}

		return is_clicked;
}

 while (!platform_should_window_close(platform)) {
  const float now_time = platform_get_time();
  const float dt = now_time - prev_time;
  prev_time = now_time;

  platform_read_input(platform);

  // Vérifier si un clic de souris a été effectué
  if (platform_was_mouse_clicked(platform)) {
    // Faire quelque chose en réponse au clic de souris
    printf("Un clic de souris a été effectué !\n");
  }

  world_player_tick(world, platform, dt);
  renderer_render(renderer, world_get_view_matrix(world), dt);

  platform_end_frame(platform);
}



 }



 exit(EXIT_SUCCESS);

}


void retour(){
	
    // Initialisation de la bibliothèque SDL_ttf
    TTF_Init();

    // Chargement de la police de caractères
    TTF_Font* font = TTF_OpenFont("assets/menu/contrast.ttf", 32);
	TTF_Font* font2 = TTF_OpenFont("assets/menu/contrast2.ttf", 32);
    if (font == NULL || font2 == NULL) {
        fprintf(stderr, "Erreur : impossible de charger la police de caractères\n");
        return 1;
    }

    // Définition des dimensions de la fenêtre
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    // Initialisation de la SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Menu du jeu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    // Création du renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Color textColor = { 255, 255, 255, 255 };
	SDL_Color textColor2 = {  255, 195, 0, 0 };
	SDL_Color textColor3 = {  255, 50, 50, 0 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font2, "Fag 3D", textColor2);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = { (SCREEN_WIDTH - textSurface->w) / 2, (SCREEN_HEIGHT - textSurface->h) / 2 - 100, textSurface->w, textSurface->h };

    SDL_Surface* textSurface1 = TTF_RenderText_Solid(font, "Jouer", textColor);
    SDL_Texture* textTexture1 = SDL_CreateTextureFromSurface(renderer, textSurface1);
    SDL_Rect textRect1 = { (SCREEN_WIDTH - textSurface1->w) / 2, (SCREEN_HEIGHT - textSurface1->h) / 2 - 50, textSurface1->w, textSurface1->h };

    SDL_Surface* textSurface2 = TTF_RenderText_Solid(font, "Options", textColor);
    SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
    SDL_Rect textRect2 = { (SCREEN_WIDTH - textSurface2->w) / 2, (SCREEN_HEIGHT - textSurface2->h) / 2, textSurface2->w, textSurface2->h };

    SDL_Surface* textSurface3 = TTF_RenderText_Solid(font, "Quitter", textColor3);
    SDL_Texture* textTexture3 = SDL_CreateTextureFromSurface(renderer, textSurface3);
    SDL_Rect textRect3 = { (SCREEN_WIDTH - textSurface3->w) / 2, (SCREEN_HEIGHT - textSurface3->h) / 2 + 50, textSurface3->w, textSurface3->h };

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
                            optionReglage();
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
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Affichage des textures
        SDL_RenderCopy(renderer, textTexture, NULL, NULL);
        SDL_RenderCopy(renderer, textTexture1, NULL, &textRect1);
        SDL_RenderCopy(renderer, textTexture2, NULL, &textRect2);
        SDL_RenderCopy(renderer, textTexture3, NULL, &textRect3);

        // Mise à jour de l'écran
        SDL_RenderPresent(renderer);
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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Fermeture de la bibliothèque SDL_ttf
    TTF_Quit();

    // Fermeture de la SDL
    SDL_Quit();

    return 0;
}


//Fonction pour le menu option

int optionReglage(){
	
    // Initialisation de la bibliothèque SDL_ttf
    TTF_Init();

    // Chargement de la police de caractères
    TTF_Font* font = TTF_OpenFont("assets/menu/contrast.ttf", 32);
    if (font == NULL) {
        fprintf(stderr, "Erreur : impossible de charger la police de caractères\n");
        return 1;
    }

    // Définition des dimensions de la fenêtre
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    // Initialisation de la SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Menu du jeu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    // Création du renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    // Sous-menu des options
	SDL_Color textColor = { 255, 255, 255, 255 };
	SDL_Color textColor3 = { 255, 50, 50, 0 };
    SDL_Surface* textSurface4 = TTF_RenderText_Solid(font, "Reglage du volume", textColor);
    SDL_Texture* textTexture4 = SDL_CreateTextureFromSurface(renderer, textSurface4);
    SDL_Rect textRect4 = { (SCREEN_WIDTH - textSurface4->w) / 2, (SCREEN_HEIGHT - textSurface4->h) / 2 - 100, textSurface4->w, textSurface4->h };

    SDL_Surface* textSurface5 = TTF_RenderText_Solid(font, "Changement de langue", textColor);
    SDL_Texture* textTexture5 = SDL_CreateTextureFromSurface(renderer, textSurface5);
    SDL_Rect textRect5 = { (SCREEN_WIDTH - textSurface5->w) / 2, (SCREEN_HEIGHT - textSurface5->h) / 2 - 50, textSurface5->w, textSurface5->h };

    SDL_Surface* textSurface6 = TTF_RenderText_Solid(font, "Reglage de l'affichage", textColor);
    SDL_Texture* textTexture6 = SDL_CreateTextureFromSurface(renderer, textSurface6);
    SDL_Rect textRect6 = { (SCREEN_WIDTH - textSurface6->w) / 2, (SCREEN_HEIGHT - textSurface6->h) / 2, textSurface6->w, textSurface6->h };

    SDL_Surface* textSurface7 = TTF_RenderText_Solid(font, "Retour", textColor3);
    SDL_Texture* textTexture7 = SDL_CreateTextureFromSurface(renderer, textSurface7);
    SDL_Rect textRect7 = { (SCREEN_WIDTH - textSurface7->w) / 2, (SCREEN_HEIGHT - textSurface7->h) / 2 + 50, textSurface7->w, textSurface7->h };

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
                        if (x >= textRect4.x && x <= textRect4.x + textRect4.w && y >= textRect4.y && y <= textRect4.y + textRect4.h) {
							
                            // Action du bouton volume
                        } else if (x >= textRect5.x && x <= textRect5.x + textRect5.w && y >= textRect5.y && y <= textRect5.y + textRect5.h) {
                     
							// Action du bouton langue
                        } else if (x >= textRect6.x && x <= textRect6.x + textRect6.w && y >= textRect6.y && y <= textRect6.y + textRect6.h) {
                            
                        }
							// Action du bouton retour
						else if (x >= textRect7.x && x <= textRect7.x + textRect7.w && y >= textRect7.y && y <= textRect7.y + textRect7.h) {
                            retour();
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        // Effacement de l'écran
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Affichage des textures
   
        SDL_RenderCopy(renderer, textTexture4, NULL, &textRect4);
        SDL_RenderCopy(renderer, textTexture5, NULL, &textRect5);
        SDL_RenderCopy(renderer, textTexture6, NULL, &textRect6);
		SDL_RenderCopy(renderer, textTexture7, NULL, &textRect7);

        // Mise à jour de l'écran
        SDL_RenderPresent(renderer);
    }

    // Libération de la mémoire
    
    SDL_DestroyTexture(textTexture4);
    SDL_FreeSurface(textSurface4);
    SDL_DestroyTexture(textTexture5);
    SDL_FreeSurface(textSurface5);
    SDL_DestroyTexture(textTexture6);
    SDL_FreeSurface(textSurface6);
	SDL_DestroyTexture(textTexture7);
    SDL_FreeSurface(textSurface7);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Fermeture de la bibliothèque SDL_ttf
    TTF_Quit();

    // Fermeture de la SDL
    SDL_Quit();

    return 0;
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

    // Définition des dimensions de la fenêtre
    const int SCREEN_WIDTH = 640;
    const int SCREEN_HEIGHT = 480;

    // Initialisation de la SDL
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Menu du jeu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    // Création du renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

    SDL_Color textColor = { 255, 255, 255, 255 };
	SDL_Color textColor2 = {  255, 195, 0, 0 };
	SDL_Color textColor3 = {  255, 50, 50, 0 };
    SDL_Surface* textSurface = TTF_RenderText_Solid(font2, "Frag 3D", textColor2);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = { (SCREEN_WIDTH - textSurface->w) / 2, (SCREEN_HEIGHT - textSurface->h) / 2 - 100, textSurface->w, textSurface->h };

    SDL_Surface* textSurface1 = TTF_RenderText_Solid(font, "Jouer", textColor);
    SDL_Texture* textTexture1 = SDL_CreateTextureFromSurface(renderer, textSurface1);
    SDL_Rect textRect1 = { (SCREEN_WIDTH - textSurface1->w) / 2, (SCREEN_HEIGHT - textSurface1->h) / 2 - 50, textSurface1->w, textSurface1->h };

    SDL_Surface* textSurface2 = TTF_RenderText_Solid(font, "Options", textColor);
    SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
    SDL_Rect textRect2 = { (SCREEN_WIDTH - textSurface2->w) / 2, (SCREEN_HEIGHT - textSurface2->h) / 2, textSurface2->w, textSurface2->h };

    SDL_Surface* textSurface3 = TTF_RenderText_Solid(font, "Quitter", textColor3);
    SDL_Texture* textTexture3 = SDL_CreateTextureFromSurface(renderer, textSurface3);
    SDL_Rect textRect3 = { (SCREEN_WIDTH - textSurface3->w) / 2, (SCREEN_HEIGHT - textSurface3->h) / 2 + 50, textSurface3->w, textSurface3->h };

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
                            optionReglage();
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
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Affichage des textures
        SDL_RenderCopy(renderer, textTexture, NULL, NULL);
        SDL_RenderCopy(renderer, textTexture1, NULL, &textRect1);
        SDL_RenderCopy(renderer, textTexture2, NULL, &textRect2);
        SDL_RenderCopy(renderer, textTexture3, NULL, &textRect3);

        // Mise à jour de l'écran
        SDL_RenderPresent(renderer);
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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // Fermeture de la bibliothèque SDL_ttf
    TTF_Quit();

    // Fermeture de la SDL
    SDL_Quit();

    return 0;
}

