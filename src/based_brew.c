#include <string.h>
#include <switch.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>

#define MAX_DAB 6

Result based_brew_init(void) {
    int sdl_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    int mix_flags = MIX_INIT_MP3;

    Result rc = SDL_Init(sdl_flags);

    if (R_SUCCEEDED(rc))
        rc = img_flags != IMG_Init(img_flags);

    // We have to call Mix_OpenAudio before Mix_Init, as per:
    // https://discourse.libsdl.org/t/bug-sdl2-mixer-init-error-format-support-not-available/23705/2
    if (R_SUCCEEDED(rc))
        rc = Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 0x10000);

    if (R_SUCCEEDED(rc))
        rc = mix_flags != Mix_Init(mix_flags);

    if (R_FAILED(rc))
        SDL_Log("Error while initializing SDL: %u (%s).\n", rc, SDL_GetError());

    if (R_SUCCEEDED(rc))
        rc = romfsInit();
    
    return rc;
}

void based_brew_exit(void) {
    SDL_Quit();
    IMG_Quit();
    Mix_CloseAudio();
    Mix_Quit();
    romfsExit();
}

int main(int argc, char **argv) {
    int texture_index = 0;
    char path[50];
    u64 keys_down = 0;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    Mix_Music *music = NULL;
    SDL_Texture **textures = NULL;

    if (R_FAILED(based_brew_init())) {
        goto error;
    }
    
    if (!(window = SDL_CreateWindow(NULL, 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN))) {
        SDL_Log("Could not create window\n");
        goto error;
    }
    
    if (!(renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC))) {
        SDL_Log("Could not create renderer\n");
        goto error;
    }
   
    music = Mix_LoadMUS("romfs:/LMFAO - Party Rock Anthem.mp3");
    if (!(music)) {
        SDL_Log("Could not load MP3\n");
        goto error;
    }

    textures = malloc(MAX_DAB * sizeof(SDL_Texture *));
    for (int i=0; i<MAX_DAB+1; i++) {
        sprintf(path, "romfs:/mario_dab%d.jpg", i);
        textures[i] = IMG_LoadTexture(renderer, path);
        if (textures[i] == NULL)
            SDL_Log("Could not load %s: %s\n", path, IMG_GetError());
    }

    Mix_PlayMusic(music, -1);
    while (appletMainLoop()) {
        hidScanInput();
        keys_down = hidKeysDown(CONTROLLER_P1_AUTO);
        
        if (keys_down & KEY_PLUS) 
            break;

        if (keys_down & KEY_A) {
            texture_index = (texture_index + 1) % (MAX_DAB + 1);
        }

        SDL_RenderCopy(renderer, textures[texture_index], NULL, NULL);
        SDL_RenderPresent(renderer);
    }
    Mix_HaltMusic();

    for (int i=0; i<MAX_DAB+1; i++) {
        SDL_DestroyTexture(textures[i]);
    }
    free(textures);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_FreeMusic(music);
error:    
    based_brew_exit();
    return 0;
}
