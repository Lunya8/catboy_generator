#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "jansson.h"

struct memory_block {
    size_t size;
    char* memory;
};

#define NULL_MEMORY_BLOCK {.size=0, .memory=malloc(1)}

CURL* curl_object;

void init_curl_api() {
    curl_object = curl_easy_init();
    if (!curl_object) {
        fprintf(stderr, "libcurl failed to init!");
        exit(1);
    }
}

size_t write_callback_func(void* buffer, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    struct memory_block* block = (struct memory_block*)userp;

    block->memory = realloc(block->memory, block->size + real_size);
    if (block->memory == NULL)
        return 0;

    memcpy(&block->memory[block->size], buffer, real_size);
    block->size += real_size;

    return real_size;
}

struct memory_block curl_http_get(char* url) {
    struct memory_block n = NULL_MEMORY_BLOCK;

    if (!curl_object) {
        fprintf(stderr, "Cannot send GET to %s:\nCURL not initialized!\n", url);
        return n;
    }

    struct memory_block response_buffer = NULL_MEMORY_BLOCK;

    curl_easy_setopt(curl_object, CURLOPT_URL, url);
    curl_easy_setopt(curl_object, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    curl_easy_setopt(curl_object, CURLOPT_WRITEDATA, &response_buffer);
    curl_easy_setopt(curl_object, CURLOPT_WRITEFUNCTION, write_callback_func);

    CURLcode result = curl_easy_perform(curl_object);

    if (result != CURLE_OK) {
        fprintf(stderr, "Cannot send GET to %s:\n%s", url, curl_easy_strerror(result));
        return n;
    }

    return response_buffer;
}

void shutdown_curl_api() {
    curl_easy_cleanup(curl_object);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG|IMG_INIT_WEBP);
    init_curl_api();
    char* url;

    do {
        struct memory_block block = curl_http_get("https://api.catboys.com/img");
        json_t *root = json_loads(block.memory, 0, NULL);
        url = json_string_value(json_object_get(root, "url"));
    } while (!url);
    printf("URL: %s\n", url);

    struct memory_block picture_data = curl_http_get(url);

    const int MAX_HEIGHT = 600;
    const int MAX_WIDTH = 800;

    SDL_RWops* rwop;
    rwop = SDL_RWFromConstMem(picture_data.memory, (int)picture_data.size);
    SDL_Surface* texture_surface = IMG_Load_RW(rwop, 0);
    SDL_Rect window_size;
    window_size.x = 0;
    window_size.y = 0;
    window_size.w = texture_surface->w;
    window_size.h = texture_surface->h;
    float scaling_factor;
    if (window_size.h > MAX_HEIGHT) {
        scaling_factor = (float)MAX_HEIGHT / (float)window_size.h;
        window_size.h = (int)((float)window_size.h * scaling_factor);
        window_size.w = (int)((float)window_size.w * scaling_factor);
    }
    if (window_size.w > MAX_WIDTH) {
        scaling_factor = (float)MAX_WIDTH / (float)window_size.w;
        window_size.h = (int)((float)window_size.h * scaling_factor);
        window_size.w = (int)((float)window_size.w * scaling_factor);
    }

    SDL_Window* window = SDL_CreateWindow("Catboy", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          window_size.w, window_size.h, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, texture_surface);
    SDL_FreeSurface(texture_surface);

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
    SDL_RenderCopy(renderer, texture, NULL, &window_size);
    SDL_RenderPresent(renderer);

    int running = 1;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                running = 0;
                break;
            }
        }
    }

    SDL_DestroyTexture(texture);
    SDL_FreeRW(rwop);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    shutdown_curl_api();
}