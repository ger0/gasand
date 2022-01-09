#include <SDL2/SDL.h>
#include <cstdio>

#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   800

bool isRunning = true;

int main() {
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      printf("Failed to initialize the SDL2 library\n");
      return -1;
   }

   SDL_Window *window = SDL_CreateWindow(
         "gasand",
         SDL_WINDOWPOS_CENTERED,
         SDL_WINDOWPOS_CENTERED,
         SCREEN_WIDTH, SCREEN_HEIGHT,
         0
   );
   SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

   if (!window) {
      printf("Failed to create window\n");
      return -2;
   }

   SDL_Surface *window_surface = SDL_GetWindowSurface(window);
   if (!window_surface) {
      printf("Failed to create window surface");
      return -3;
   }

   while (isRunning) {
      // handle events
      SDL_Event e;
      while (SDL_PollEvent(&e) > 0) {
         switch(e.type) {
            case SDL_QUIT:
               isRunning = false;
               break;
            case SDL_KEYDOWN:
               isRunning = false;
               break;
         }
      }
      // handle updates
      // handle rendering
renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
      if (renderer) {
         SDL_SetRenderDrawColor(renderer, 38, 38, 38, 255);
            printf("Created renderer\n")
      }
      SDL_Rect rect = {30 * 2, 30 * 2, 30, 30};
      SDL_RenderFillRect(renderer, &rect);
      SDL_UpdateWindowSurface(window);
      SDL_Delay(16);
   }
   // clean up 
   return 0;
}

